#pragma once
#include "pch.h"
#include "framework.h"

#include "archivist_interfaces.h"

#include "iMutexies.h"
#include "utilities.h"
#include "lib_exceptions.h"
#include "shared_containers.h"

namespace HYDRA15::Union::archivist
{
    /***************************** 设 计 ****************************
    *
    * 始终在初始化阶段尝试将所有数据加载到内存中，如果内存不足则会收到系统异常
    * 使用单字段索引加速查询，加速查询仅在 excute 接口中有效
    * 
    * 查询优先：有迭代器（entry）存在时可能会阻塞插入操作
    * 
    * 构造要求：
    *   构造一个 loader ，其包含的 version == simple_memort_table::version，
    *   并且其第一个字段为 simple_memory_table::sysfldRowMark
    *   从此 loader 构造 simple_memory_table 对象
    * 
    * 系统字段：
    *   $RowMark：位图模式的行标记
    *       0x1 无效记录标记 当此字段没有数据时默认为此标记
    *       0x2 已删除标记
    * 
    * ****************************************************************/
    class simple_memory_table : public table
    {
    public:
        // 表关联的 entry 容器，拥有迭代和修改数据能力，但是可能会影响全表性能，应限制作用域
        class entry_impl : public entry
        {
        private:
            simple_memory_table& tableRef;
            mutable std::optional<std::shared_lock<labourer::atomic_shared_mutex>> sl;
            mutable ID rowID;

            bool locked = false;
            mutable bool lockShared = false;

        private:
            void transport(ID target) const // 移动到指定行，同时转移锁状态
            {
                if (locked)tableRef.rowMtxs[rowID].unlock();
                if (lockShared)tableRef.rowMtxs[rowID].unlock_shared();
                rowID = target;
                if (locked)tableRef.rowMtxs[rowID].lock();
                if (lockShared)tableRef.rowMtxs[rowID].lock_shared();
            }

            INT get_row_mark() const
            {
                if (auto pint = std::get_if<INT>(&tableRef.tabData[rowID * tableRef.fieldTab.size() + tableRef.fieldNameTab.at(tableRef.sysfldRowMark)]))
                    return *pint;
                return simple_memory_table::row_bit_mark::invalid_bit;
            }

            

            void init() const { if (!sl)sl.emplace(tableRef.tableMtx); }    // 延迟锁表到首次访问时

        public:     // 系统接口
            virtual ~entry_impl() = default;
            virtual std::unique_ptr<entry> clone() override
            {
                return std::make_unique<entry_impl>(tableRef, rowID);
            }

            // 记录信息
            virtual ID id() const override { return rowID; };  // 返回 记录 ID

            virtual const field_specs& fields() const override { return tableRef.fieldTab; } // 返回完整的字段表

            virtual entry& erase() // 删除当前记录，迭代器移动到下一条有效记录
            {
                init();
                if (!(get_row_mark() & (simple_memory_table::row_bit_mark::deleted_bit | simple_memory_table::row_bit_mark::deleted_bit)))
                {
                    std::get<INT>(tableRef.tabData[rowID * tableRef.fieldTab.size() + tableRef.fieldNameTab.at(sysfldRowMark)])
                        |= simple_memory_table::row_bit_mark::deleted_bit;
                    tableRef.modifiedRows.insert(rowID);
                    tableRef.update_index(rowID);
                }
                operator++();
                return *this;
            }

            // 获取、写入记录项
            virtual const field& at(const std::string& fieldName) const override  // 返回 指定字段的数据
            {
                init();
                if (get_row_mark() & (simple_memory_table::row_bit_mark::deleted_bit | simple_memory_table::row_bit_mark::deleted_bit))
                    throw exceptions::common("Record id invalid");
                return tableRef.tabData[rowID * tableRef.fieldTab.size() + tableRef.fieldNameTab.at(fieldName)];
            }

            virtual entry& set(const std::string& fieldName, const field& data) override   // 写入指定字段
            {   
                init();
                if (get_row_mark() & (simple_memory_table::row_bit_mark::deleted_bit | simple_memory_table::row_bit_mark::deleted_bit))
                    throw exceptions::common("Writing data to a invalid row is not allowed");
                tableRef.tabData[rowID * tableRef.fieldTab.size() + tableRef.fieldNameTab.at(fieldName)] = data;
                tableRef.modifiedRows.insert(rowID);
                tableRef.update_index(rowID);
                return *this;
            }

            // 将数据行对象直接作为迭代器使用，需要支持迭代器的操作
            virtual entry& operator++() override    // 滑动到下一条有效记录
            { 
                init();
                ID currentRecordCount = tableRef.recordCount.load(std::memory_order::relaxed);
                if (rowID < currentRecordCount)
                    transport(rowID + 1);
                while (rowID < currentRecordCount &&
                    (get_row_mark() & (simple_memory_table::row_bit_mark::deleted_bit | simple_memory_table::row_bit_mark::invalid_bit)))
                    transport(rowID + 1);
                if (rowID >= currentRecordCount)
                    rowID = tableRef.end()->id();
                return *this; 
            }

            virtual entry& operator--() override // 滑动到上一条有效记录
            { 
                init();
                if (rowID > 0)
                    transport(rowID - 1);
                while (rowID > 0 &&
                    (get_row_mark() & (simple_memory_table::row_bit_mark::deleted_bit | simple_memory_table::row_bit_mark::invalid_bit)))
                    transport(rowID - 1);
                if (rowID == 0 &&
                    (get_row_mark() & (simple_memory_table::row_bit_mark::deleted_bit | simple_memory_table::row_bit_mark::invalid_bit)))
                    rowID = tableRef.end()->id();
                return *this; 
            }

            virtual const entry& operator++() const override
            {
                init();
                ID currentRecordCount = tableRef.recordCount.load(std::memory_order::relaxed);
                if (rowID < currentRecordCount)
                    transport(rowID + 1);
                while (rowID < currentRecordCount &&
                    (get_row_mark() & (simple_memory_table::row_bit_mark::deleted_bit | simple_memory_table::row_bit_mark::invalid_bit)))
                    transport(rowID + 1);
                if (rowID >= currentRecordCount)
                    rowID = tableRef.end()->id();
                return *this;
            }

            virtual const entry& operator--() const override
            {
                init();
                if (rowID > 0)
                    transport(rowID - 1);
                while (rowID > 0 &&
                    (get_row_mark() & (simple_memory_table::row_bit_mark::deleted_bit | simple_memory_table::row_bit_mark::invalid_bit)))
                    transport(rowID - 1);
                if (rowID == 0 &&
                    (get_row_mark() & (simple_memory_table::row_bit_mark::deleted_bit | simple_memory_table::row_bit_mark::invalid_bit)))
                    rowID = tableRef.end()->id();
                return *this;
            }

            virtual std::strong_ordering operator<=>(const entry& oth) const override
            {
                ID othID = oth.id();
                if(rowID < othID)return std::strong_ordering::less;
                else if(rowID > othID)return std::strong_ordering::greater;
                else return std::strong_ordering::equal;
            }

            virtual bool operator==(const entry& oth) const override { return rowID == oth.id(); }

        public:     // 行锁
            virtual void lock() override { init(); tableRef.rowMtxs[rowID].lock(); locked = true; }

            virtual void unlock() override { init(); tableRef.rowMtxs[rowID].unlock(); locked = false; }

            virtual bool try_lock() override { init(); locked = tableRef.rowMtxs[rowID].try_lock(); return locked; }

            virtual void lock_shared() const override { init(); tableRef.rowMtxs[rowID].lock_shared(); lockShared = true; }

            virtual void unlock_shared() const override { init(); tableRef.rowMtxs[rowID].unlock_shared(); lockShared = false; }

            virtual bool try_lock_shared() const override { init(); lockShared = tableRef.rowMtxs[rowID].try_lock_shared(); return lockShared; }

        public:
            simple_memory_table& get_table() const { return tableRef; }

        public:
            entry_impl(simple_memory_table& tab, ID id) : tableRef(tab), rowID(id) {}
        };
        friend class entry_impl;

        // 存储数据的 entry 容器，不关联表，不能修改表中数据，不能迭代，不涉及的接口不实现
        class data_entry_impl : public entry
        {
        private:
            const std::unordered_map<std::string, size_t> fieldNameTab;

            std::vector<field> rowData;


        public:     // 系统接口
            virtual std::unique_ptr<entry> clone() override
            {
                return std::make_unique<data_entry_impl>(*this);
            }

            // 记录信息
            virtual ID id() const override { return std::numeric_limits<ID>::max(); }
            virtual const field_specs& fields() const override                      { throw exceptions::common::UnsupportedInterface("HYDRA15::Union::archivist::entry", "HYDRA15::Union::archivist::simple_memory_table::data_entry_impl", "fields"); }
            virtual entry& erase() override                                         { throw exceptions::common::UnsupportedInterface("HYDRA15::Union::archivist::entry", "HYDRA15::Union::archivist::simple_memory_table::data_entry_impl", "erase"); }

            // 获取、写入记录项
            virtual const field& at(const std::string& fieldName) const override  // 返回 指定字段的数据
            {
                return rowData[fieldNameTab.at(fieldName)];
            }

            virtual entry& set(const std::string& fieldName, const field& data) override
            {
                rowData[fieldNameTab.at(fieldName)] = data;
                return *this;
            }

            // 将数据行对象直接作为迭代器使用，需要支持迭代器的操作
            virtual entry& operator++() override                                    { throw exceptions::common::UnsupportedInterface("HYDRA15::Union::archivist::entry", "HYDRA15::Union::archivist::simple_memory_table::data_entry_impl", "operator++"); }
            virtual entry& operator--() override                                    { throw exceptions::common::UnsupportedInterface("HYDRA15::Union::archivist::entry", "HYDRA15::Union::archivist::simple_memory_table::data_entry_impl", "operator--"); }
            virtual const entry& operator++() const override                        { throw exceptions::common::UnsupportedInterface("HYDRA15::Union::archivist::entry", "HYDRA15::Union::archivist::simple_memory_table::data_entry_impl", "operator++ const"); }
            virtual const entry& operator--() const override                        { throw exceptions::common::UnsupportedInterface("HYDRA15::Union::archivist::entry", "HYDRA15::Union::archivist::simple_memory_table::data_entry_impl", "operator-- const"); }
            virtual std::strong_ordering operator<=>(const entry&) const override   { throw exceptions::common::UnsupportedInterface("HYDRA15::Union::archivist::entry", "HYDRA15::Union::archivist::simple_memory_table::data_entry_impl", "operator<=>"); }
            virtual bool operator==(const entry&) const override                    { throw exceptions::common::UnsupportedInterface("HYDRA15::Union::archivist::entry", "HYDRA15::Union::archivist::simple_memory_table::data_entry_impl", "operator=="); }

            // 行锁
            virtual void lock() override                                            { throw exceptions::common::UnsupportedInterface("HYDRA15::Union::archivist::entry", "HYDRA15::Union::archivist::simple_memory_table::data_entry_impl", "lock"); }
            virtual void unlock() override                                          { throw exceptions::common::UnsupportedInterface("HYDRA15::Union::archivist::entry", "HYDRA15::Union::archivist::simple_memory_table::data_entry_impl", "unlock"); }
            virtual bool try_lock() override                                        { throw exceptions::common::UnsupportedInterface("HYDRA15::Union::archivist::entry", "HYDRA15::Union::archivist::simple_memory_table::data_entry_impl", "try_lock"); }
            virtual void lock_shared() const override                               { throw exceptions::common::UnsupportedInterface("HYDRA15::Union::archivist::entry", "HYDRA15::Union::archivist::simple_memory_table::data_entry_impl", "lock_shared"); }
            virtual void unlock_shared() const override                             { throw exceptions::common::UnsupportedInterface("HYDRA15::Union::archivist::entry", "HYDRA15::Union::archivist::simple_memory_table::data_entry_impl", "unlock_shared"); }
            virtual bool try_lock_shared() const override                           { throw exceptions::common::UnsupportedInterface("HYDRA15::Union::archivist::entry", "HYDRA15::Union::archivist::simple_memory_table::data_entry_impl", "try_lock_shared"); }

        public:
            data_entry_impl(const simple_memory_table& tab, const std::vector<field>& data = {})
                : rowData{ data }, fieldNameTab(tab.fieldNameTab) {
                if (rowData.empty())rowData.resize(tab.fieldTab.size());
            }
            data_entry_impl(const data_entry_impl& oth)
                : rowData{ oth.rowData }, fieldNameTab(oth.fieldNameTab) {
            }
            data_entry_impl(const entry_impl& oth)
                : rowData{}, fieldNameTab(oth.get_table().fieldNameTab) {
                ID rid = oth.id();
                rowData.resize(oth.get_table().fieldTab.size());
                for (size_t i = 0; i < rowData.size(); i++)
                    rowData[i] = oth.get_table().tabData[rid * oth.get_table().fieldTab.size() + i];
            }
        };
        friend class data_entry_impl;

    private:
        // 单字段索引表
        class index_impl
        {
        private:    // 异构比较器
            class comparator
            {
            public:
                using is_transparent = void;

            private:
                const simple_memory_table& tableRef;
                const field_spec& fieldSpec;

            public:
                bool operator()(ID l, ID r) const
                {
                    const field& lfield = tableRef.tabData[l * tableRef.fieldTab.size() + tableRef.fieldNameTab.at(fieldSpec)];
                    const field& rfield = tableRef.tabData[r * tableRef.fieldTab.size() + tableRef.fieldNameTab.at(fieldSpec)];
                    return entry_impl::compare_field_ls(lfield, rfield, fieldSpec);
                }

                bool operator()(ID l, const entry& r) const 
                {
                    const field& lfield = tableRef.tabData[l * tableRef.fieldTab.size() + tableRef.fieldNameTab.at(fieldSpec)];
                    const field& rfield = r.at(fieldSpec);
                    return entry_impl::compare_field_ls(lfield, rfield, fieldSpec);
                }

                bool operator()(const entry& l, ID r) const 
                {
                    const field& lfield = l.at(fieldSpec);
                    const field& rfield = tableRef.tabData[r * tableRef.fieldTab.size() + tableRef.fieldNameTab.at(fieldSpec)];
                    return entry_impl::compare_field_ls(lfield, rfield, fieldSpec);
                }

            public:
                comparator(const simple_memory_table& tab, const field_spec& fs) : tableRef(tab), fieldSpec(fs) {}
            };

        private:
            std::multiset<ID, comparator> data;
            std::unordered_map<ID, std::multiset<ID, comparator>::iterator> id2it;
            labourer::atomic_shared_mutex mtx;

        public:
            const field_spec& fieldSpec;

        public:
            void insert(ID id) { std::unique_lock ul{ mtx }; auto it = data.insert(id); id2it[id] = it; }

            void erase(ID id)
            {
                std::unique_lock ul{ mtx };
                auto it = id2it.find(id);
                if (it != id2it.end()) { data.erase(it->second); id2it.erase(it); }
                else for(auto itr = data.begin(); itr != data.end(); ++itr)
                    if (*itr == id) { data.erase(itr); break; }
            }

            std::unordered_set<ID> search(const entry& ent, const std::unordered_set<ID>& ref = {})
            {
                std::unordered_set<ID> result;
                std::shared_lock sl{ mtx };
                auto range = data.equal_range(ent);
                for (auto& it = range.first; it != range.second; ++it)
                    if (ref.empty()) result.insert(*it);
                    else if (ref.contains(*it)) result.insert(*it);
                return result;
            }

            std::unordered_set<ID> search_range(const entry& lower, const entry& upper, const std::unordered_set<ID>& ref = {})
            {
                std::unordered_set<ID> result;
                std::shared_lock sl{ mtx };
                auto range = std::make_pair(data.lower_bound(lower), data.upper_bound(upper));
                for (auto& it = range.first; it != range.second; ++it)
                    if (ref.empty()) result.insert(*it);
                    else if (ref.contains(*it)) result.insert(*it);
                return result;
            }

            std::unordered_set<ID> search_ls(const entry& upper, const std::unordered_set<ID>& ref = {})
            {
                std::unordered_set<ID> res;
                std::shared_lock sl{ mtx };
                auto up = data.upper_bound(upper);
                for (auto it = data.begin(); it != up; it++)
                    if (ref.empty()) res.insert(*it);
                    else if (ref.contains(*it)) res.insert(*it);
                return res;
            }

            std::unordered_set<ID> search_gr(const entry& lower, const std::unordered_set<ID>& ref = {})
            {
                std::unordered_set<ID> res;
                std::shared_lock sl{ mtx };
                auto lw = data.upper_bound(lower);
                for (auto it = lw; it != data.end(); it++)
                    if (ref.empty()) res.insert(*it);
                    else if (ref.contains(*it)) res.insert(*it);
                return res;
            }

            bool contains(ID id) { std::shared_lock sl{ mtx }; return id2it.contains(id); }

            void clear() { std::unique_lock ul{ mtx }; data.clear(); id2it.clear(); }

            std::deque<ID> to_deque() { std::shared_lock sl{ mtx }; return std::deque<ID>(data.begin(), data.end()); }

        public:
            bool operator==(const index_impl& oth) const { return fieldSpec == oth.fieldSpec; }
            bool operator<(const index_impl& oth) const { return fieldSpec.name < oth.fieldSpec.name; }
            bool operator==(const std::string& oth) const { return fieldSpec.name == oth; }
            bool operator<(const std::string& oth) const { return fieldSpec.name < oth; }

        public:
            index_impl(const simple_memory_table& tab, const field_spec& fs, const std::deque<ID>& dat)
                : data(comparator(tab, fs)), fieldSpec(fs) {
                for (ID id : dat)
                {
                    auto it = data.insert(data.end(), id);
                    id2it[id] = it;
                }
            }

            index_impl(const simple_memory_table& tab, const field_spec& fs)
                : data(comparator(tab, fs)), fieldSpec(fs) {
            }
        };
        friend class index_table;

    public:
        struct row_bit_mark
        {
            static constexpr INT invalid_bit = 0x1;
            static constexpr INT deleted_bit = 0x2;
        };

        static constexpr std::pair<ID, ID> version{ 0x4172634D656D74,0x0000000100000000 };  // "ArchMemt", 1.0

        static const inline field_spec sysfldRowMark{ "$RowMark","stores the state of data row",field_spec::field_type::INT };

    private:
        // 系统
        labourer::atomic_shared_mutex tableMtx;
        const std::unique_ptr<loader> loader;

        // 字段
        const field_specs fieldTab;
        const std::unordered_map<std::string, size_t> fieldNameTab;

        // 数据
        std::atomic<ID> recordCount = 0;
        std::atomic<ID> fakeRecordCount = 0;
        std::deque<field> tabData;
        mutable std::deque<std::shared_mutex> rowMtxs;
        labourer::basic_shared_set<ID> modifiedRows;

        // 索引
        std::vector<std::unique_ptr<index_impl>> indexTab;

    private:
        std::unique_ptr<entry_impl> get_entry(ID id) { return std::make_unique<entry_impl>(*this, id); }

        std::unique_ptr<const entry_impl> get_const_entry(ID id) { return std::make_unique<const entry_impl>(*this, id); }

        void update_index(ID id)
        {
            for(auto& idx : indexTab)
            {
                if (!idx)continue;
                if (idx->contains(id))
                    idx->erase(id);
                if (!std::holds_alternative<NOTHING>(tabData[id * fieldTab.size() + fieldNameTab.at(idx->fieldSpec)]))
                    idx->insert(id);
            }
        }

        void sync_all()
        {
            // 按页加载数据
            tabData.clear(); rowMtxs.clear();
            ID currentRecordCount = loader->size();
            recordCount.store(currentRecordCount, std::memory_order::relaxed);
            if (recordCount > 0)
            {
                ID pageSize = loader->page_size();
                ID pageCount = (currentRecordCount - 1) / pageSize + 1;
                for (ID i = 0; i < pageCount; ++i)
                    tabData.append_range(loader->rows(i).data);
                tabData.resize(assistant::power_of_2_not_less_than_n(currentRecordCount) * fieldTab.size());
                rowMtxs.resize(assistant::power_of_2_not_less_than_n(currentRecordCount));
            }

            // 加载索引
            indexTab.clear();
            indexTab.resize(loader->indexies().size());
            for (const auto& idxSpec : loader->indexies())
                indexTab[fieldNameTab.at(idxSpec)] = std::make_unique<index_impl>(*this, fieldTab[fieldNameTab.at(idxSpec.fieldSpecs[0])], loader->index_tab(idxSpec).data);
        }

        void flush_all()
        {
            // 按页写入数据
            ID pageSize = loader->page_size();
            ID currentRecordCount = recordCount.load(std::memory_order::relaxed);
            ID pageCount = (currentRecordCount - 1) / pageSize + 1;
            for (ID i = 0; i < pageCount; ++i)
            {
                page pg{ i,i * pageSize,std::min(pageSize, currentRecordCount - i * pageSize) };
                pg.modified = std::unordered_set<ID>{ modifiedRows.lower_bound(pg.start), modifiedRows.upper_bound(pg.start + pg.count) };
                size_t startElem = pg.start * fieldTab.size();
                size_t elemCount = pg.count * fieldTab.size();
                pg.data = std::deque<field>(tabData.begin() + startElem, tabData.begin() + startElem + elemCount);
                loader->rows(pg);
            }

            // 写入索引
            for (auto& idx : indexTab)if (idx)
            {
                index i{ index_spec{idx->fieldSpec.name,"",{idx->fieldSpec}} };
                i.data = idx->to_deque();
                loader->index_tab(i);
            }
        }

        void resize_data_storage(ID newRowSize)
        {
            ID targetSize = assistant::power_of_2_not_less_than_n(newRowSize);
            tabData.resize(targetSize * fieldTab.size());
            rowMtxs.resize(targetSize);
        }

        static std::unordered_map<std::string, size_t> build_field_name_table(const field_specs& ftab)
        {
            std::unordered_map<std::string, size_t> result;
            for (size_t i = 0; i < ftab.size(); ++i)
                result[ftab[i].name] = i;
            return result;
        }

    public:     // 系统接口
        // 表信息与控制
        virtual ID size() const override { return recordCount.load(std::memory_order::relaxed); }        // 返回 记录条数

        virtual ID trim() override             // 优化表记录，返回优化后的记录数
        {
            std::unique_lock ul{ tableMtx };

            {
                std::queue<std::unique_lock<std::shared_mutex>> lockQueue;
                for (auto& mtx : rowMtxs)lockQueue.emplace(mtx);
                ID writePos = 0;
                for (ID readPos = 0; readPos < recordCount.load(std::memory_order::relaxed); ++readPos)
                {
                    if (!(std::get<INT>(tabData[readPos * fieldTab.size() + fieldNameTab.at(sysfldRowMark)])
                        & row_bit_mark::deleted_bit))
                    {
                        if (writePos != readPos)
                            for (size_t fidx = 0; fidx < fieldTab.size(); fidx++)
                                tabData[writePos * fieldTab.size() + fidx] = std::move(tabData[readPos * fieldTab.size() + fidx]);
                        writePos++;
                    }
                }
                recordCount.store(writePos, std::memory_order::relaxed);
            }
            ID currentRecordCount = recordCount.load(std::memory_order::relaxed);
            resize_data_storage(currentRecordCount);

            // 重建索引
            for (auto& idx : indexTab)if(idx)
            {
                idx->clear();
                for (ID id = 0; id < currentRecordCount; ++id)
                {
                    if (!std::holds_alternative<NOTHING>(tabData[id * fieldTab.size() + fieldNameTab.at(idx->fieldSpec)]))
                        idx->insert(id);
                }
            }

            // 更新修改记录
            modifiedRows.clear();
            for(ID i = 0; i < currentRecordCount; ++i)
                modifiedRows.insert(i);

            // 落盘所有数据
            loader->clear();
            flush_all();
            
            return currentRecordCount;
        }

        virtual const field_specs& fields() const override { return fieldTab; } // 返回完整的字段表

        virtual const field_spec& get_field(const std::string& name) const override { return fieldTab[fieldNameTab.at(name)]; }// 通过字段名获取指定的字段信息

        // 行访问接口
        virtual std::unique_ptr<entry> create() override                                                // 增：创建一条记录，返回相关的条目对象
        {
            ID pos;
            {
                std::shared_lock sl{ tableMtx };
                pos = fakeRecordCount.fetch_add(1, std::memory_order::relaxed);

                // 扩展数据存储
                if (tabData.size() < (pos + 1) * fieldTab.size())
                {
                    labourer::upgrade_lock ugl{ tableMtx };
                    if (tabData.size() < (pos + 1) * fieldTab.size())
                        resize_data_storage(pos + 1);
                }

                tabData[pos * fieldTab.size() + fieldNameTab.at(sysfldRowMark)] = INT{ 0 }; // 初始化系统字段
                recordCount.fetch_add(1, std::memory_order::relaxed);
            }
            return get_entry(pos);
        }

        virtual void drop(std::unique_ptr<entry> ety) override                                          // 删：通过条目对象删除记录
        {
            std::unique_lock rul{ *ety };
            ety->erase();
        }

        virtual std::list<ID> at(const std::function<bool(const entry&)>& filter) override // 改、查：通过过滤器查找记录
        {
            std::list<ID> result;
            for (auto& e : *this)
                if (filter(e))
                    result.push_back(e.id());
            return result;
        }

        virtual std::list<ID> excute(const incidents& icdts) override // 执行一系列事件，按照指定字段的排序顺序返回执行结果
        {
            // 字段搜索条件
            struct field_search_condition
            {
                bool impossible = false;
                bool hasDequal = false; std::list<field> dequalVals;
                bool hasEqual = false; field equalVal = NOTHING{};
                bool hasLower = false; field lowerVal = NOTHING{}; // lower is >=
                bool hasUpper = false; field upperVal = NOTHING{}; // upper is <
            };

            // helper：在无索引、或对索引结果做最终过滤时，用于判断某字段值是否满足该字段所有条件（AND）
            const auto eval_all_conditions = [this](const field& fv, const field_search_condition& conds, const std::string& fieldName)->bool {
                using ctype = incident::condition_param::condition_type;
                if (conds.hasEqual)
                    if (!entry::compare_field_eq(fv, conds.equalVal, fieldTab[fieldNameTab.at(fieldName)]))
                        return false;
                if (conds.hasDequal)
                    for (const auto& dv : conds.dequalVals)
                        if (entry::compare_field_eq(fv, dv, fieldTab[fieldNameTab.at(fieldName)]))
                            return false;
                if (conds.hasLower)
                    if (!(entry::compare_field_ls(conds.lowerVal, fv, fieldTab[fieldNameTab.at(fieldName)])
                        || entry::compare_field_eq(conds.lowerVal, fv, fieldTab[fieldNameTab.at(fieldName)])))
                        return false;
                if(conds.hasUpper)
                    if(!entry::compare_field_ls(fv, conds.upperVal, fieldTab[fieldNameTab.at(fieldName)]))
                        return false;
                return true;
                };

            // 结合操作命名空间
            using namespace assistant::set_operation;

            std::unordered_set<ID> finalResult;
            std::unordered_set<ID> currentResult;
            incidents opers = icdts;

            while (!opers.empty())
            {
                auto icdt = std::move(opers.front());
                opers.pop();

                switch (icdt.type)
                {
                case incident::incident_type::separate:
                {
                    finalResult = finalResult + currentResult;
                    currentResult = {};
                    break;
                }
                case incident::incident_type::join:
                {
                    currentResult = finalResult + currentResult;
                    finalResult = {};
                    break;
                }
                case incident::incident_type::ord_lmt:
                {
                    incident::ord_lmt_param param = std::get<incident::ord_lmt_param>(icdt.param);
                    std::vector<ID> ordered(currentResult.begin(),currentResult.end());
                    std::sort(ordered.begin(), ordered.end(), [this, param](ID l, ID r) {
                        auto le = get_const_entry(l); std::shared_lock sll{ *le };
                        auto re = get_const_entry(r); std::shared_lock slr{ *re };
                        return entry::compare_ls(*le, *re, param.targetFields);
                        });

                    ordered.resize(std::min(param.limit,ordered.size()));
                    currentResult.clear();
                    currentResult.insert_range(ordered);
                    break;
                }
                case incident::incident_type::create:
                {
                    entry& e = *std::get<std::shared_ptr<entry>>(icdt.param);
                    auto ne = create();
                    std::unique_lock ulpe{ *ne };
                    for (const auto& field : fieldTab)
                        ne->set(field, e.at(field));
                    break;
                }
                case incident::incident_type::drop:
                {
                    for (const auto& id : currentResult)
                    {
                        auto pe = get_entry(id);
                        std::unique_lock ule{ *pe };
                        pe->erase();
                    }
                    currentResult.clear();
                    break;
                }
                case incident::incident_type::modify:
                {
                    incident::modify_param param = std::get<incident::modify_param>(icdt.param);
                    for (const auto id : currentResult)
                    {
                        auto pe = get_entry(id);
                        std::unique_lock ule{ *pe };
                        pe->set(param.targetField, param.value);
                    }
                    break;
                }
                case incident::incident_type::search:
                {
                    bool impossiable = false;
                    std::unordered_map<std::string, field_search_condition> condByField;

                    {   // 收集连续的 search 事件 并计算全部条件
                        // 收集连续的 search 事件（包含当前这个）
                        std::unordered_map<std::string, std::list<incident::condition_param>> condParamsByField;
                        {
                            incident::condition_param cond = std::get<incident::condition_param>(icdt.param);
                            condParamsByField[cond.targetField].push_back(cond);
                        }
                        while (!opers.empty() && opers.front().type == incident::incident_type::search)
                        {
                            auto nxt = std::move(opers.front()); opers.pop();
                            {
                                incident::condition_param cond = std::get<incident::condition_param>(nxt.param);
                                condParamsByField[cond.targetField].push_back(cond);
                            }
                        }

                        // 计算每个字段的条件
                        for (const auto& [fieldName, conditions] : condParamsByField)
                        {
                            field_search_condition fsc;
                            const field_spec& fs = fieldTab[fieldNameTab.at(fieldName)];
                            for (const auto& cp : conditions)
                            {
                                switch (cp.type)
                                {
                                case incident::condition_param::condition_type::equal:
                                    if (!fsc.hasEqual) { fsc.hasEqual = true; fsc.equalVal = cp.reference; }
                                    else if (!entry::compare_field_eq(fsc.equalVal, cp.reference, fs)) { fsc.impossible = true; }
                                    break;
                                case incident::condition_param::condition_type::dequal:
                                    fsc.hasDequal = true; fsc.dequalVals.push_back(cp.reference);
                                    break;
                                case incident::condition_param::condition_type::less:
                                    if (!fsc.hasUpper) { fsc.hasUpper = true; fsc.upperVal = cp.reference; }
                                    else if (entry::compare_field_ls(cp.reference, fsc.upperVal, fs)) fsc.upperVal = cp.reference; // upper = min(upper, cp.ref)
                                    break;
                                case incident::condition_param::condition_type::greater:
                                    if (!fsc.hasLower) { fsc.hasLower = true; fsc.lowerVal = cp.reference; }
                                    else if (entry::compare_field_ls(fsc.lowerVal, cp.reference, fs)) fsc.lowerVal = cp.reference; // lower = max(lower, cp.ref)
                                    break;
                                default:
                                    break;
                                }
                            }
                            // 判断是否有矛盾条件
                            if (fsc.impossible)
                            {
                                impossiable = true;
                                break;
                            }
                            // equal 与 dequal 冲突
                            if (fsc.hasEqual && fsc.hasDequal)
                                for (const auto& ex : fsc.dequalVals)
                                    if (entry::compare_field_eq(fsc.equalVal, ex, fs))
                                    {
                                        impossiable = true;
                                        fsc.impossible = true;
                                        break;
                                    }
                            // equal 与下界/上界冲突
                            if (fsc.hasEqual)
                            {
                                if (fsc.hasLower)
                                    // equalVal < lowerVal 则不存在满足 >= lower 的等值
                                    if (entry::compare_field_ls(fsc.equalVal, fsc.lowerVal, fs))
                                    {
                                        impossiable = true;
                                        fsc.impossible = true;
                                    }
                                if (fsc.hasUpper)
                                    // 需要 equalVal < upperVal 成立（上界为开区间）
                                    if (!entry::compare_field_ls(fsc.equalVal, fsc.upperVal, fs))
                                    {
                                        impossiable = true;
                                        fsc.impossible = true;
                                    }
                            }
                            else
                                // 下界与上界相互冲突：需要 lowerVal < upperVal，否则区间为空
                                if (fsc.hasLower && fsc.hasUpper)
                                    if (!entry::compare_field_ls(fsc.lowerVal, fsc.upperVal, fs))
                                    {
                                        impossiable = true;
                                        fsc.impossible = true;
                                        break;
                                    }

                            if (impossiable)break;

                            // 如果有 equal 条件，则删除其他所有条件
                            if (fsc.hasEqual)
                            {
                                fsc.hasDequal = false; fsc.dequalVals = {};
                                fsc.hasLower = false; fsc.lowerVal = NOTHING{};
                                fsc.hasUpper = false; fsc.upperVal = NOTHING{};
                            }

                            condByField[fieldName] = fsc;
                        }
                    }

                    // 无解则直接返回空结果
                    if(impossiable)
                    {
                        currentResult.clear();
                        break;
                    }

                    // 开始逐个字段筛选结果
                    bool firstSearch = true;
                    // 先使用有索引的字段进行搜索
                    for (const auto& [fieldName, fsc] : condByField)
                    {
                        if (!indexTab[fieldNameTab.at(fieldName)])continue; // 无索引跳过
                        auto& idx = *indexTab[fieldNameTab.at(fieldName)];

                        if (fsc.hasEqual)
                        {
                            auto pdatent = std::make_unique<data_entry_impl>(*this);
                            pdatent->set(fieldName, fsc.equalVal);
                            currentResult = idx.search(*pdatent, currentResult);
                        }

                        if (fsc.hasLower && fsc.hasUpper)
                        {
                            auto pdatentLower = std::make_unique<data_entry_impl>(*this);
                            auto pdatentUpper = std::make_unique<data_entry_impl>(*this);
                            pdatentLower->set(fieldName, fsc.lowerVal);
                            pdatentUpper->set(fieldName, fsc.upperVal);
                            currentResult = idx.search_range(*pdatentLower, *pdatentUpper, currentResult);
                        }

                        if(fsc.hasLower && !fsc.hasUpper)
                        {
                            auto pdatentLower = std::make_unique<data_entry_impl>(*this);
                            pdatentLower->set(fieldName, fsc.lowerVal);
                            currentResult = idx.search_gr(*pdatentLower, currentResult);
                        }

                        if(fsc.hasUpper && !fsc.hasLower)
                        {
                            auto pdatentUpper = std::make_unique<data_entry_impl>(*this);
                            pdatentUpper->set(fieldName, fsc.upperVal);
                            currentResult = idx.search_ls(*pdatentUpper, currentResult);
                        }

                        if (fsc.hasDequal)
                        {
                            auto pdatent = std::make_unique<data_entry_impl>(*this);
                            for(const auto& dv : fsc.dequalVals)
                            {
                                pdatent->set(fieldName, dv);
                                auto deqres = idx.search(*pdatent, currentResult);
                                currentResult = currentResult - deqres;
                            }
                        }

                        firstSearch = false;
                    }
                    // 再使用无索引的字段进行过滤
                    for (const auto& [fieldName, fsc] : condByField)
                    {
                        if (indexTab[fieldNameTab.at(fieldName)])continue; // 有索引跳过
                        std::shared_lock sl{ tableMtx };
                        if (firstSearch && currentResult.empty())
                        {
                            // 第一次搜索且无索引，需全表扫描
                            for (ID id = 0; id < recordCount.load(std::memory_order::relaxed); ++id)
                            {
                                auto pe = get_const_entry(id);
                                std::shared_lock sle{ *pe };
                                const field& fv = pe->at(fieldName);
                                if (eval_all_conditions(fv, fsc, fieldName))
                                    currentResult.insert(id);
                            }
                            firstSearch = false;
                        }
                        else
                        {
                            // 对现有结果集进行过滤
                            for (auto it = currentResult.begin(); it != currentResult.end(); )
                            {
                                auto pe = get_const_entry(*it);
                                std::shared_lock sle{ *pe };
                                const field& fv = pe->at(fieldName);
                                if (!eval_all_conditions(fv, fsc, fieldName))
                                    it = currentResult.erase(it);
                                else
                                    ++it;
                            }
                        }
                    }

                }
                default:
                    break;
                }
            }

            if (!currentResult.empty())finalResult = finalResult + currentResult;
            std::list<ID> result(finalResult.begin(), finalResult.end());
            result.sort();
            return result;
        }
        
        virtual std::unique_ptr<entry> at(ID id) override                                               // 通过行号访问记录
        {
            if (id >= recordCount.load(std::memory_order::relaxed))
                throw exceptions::common("Record ID out of range");
            return get_entry(id);
        }

        // 索引接口
        virtual void create_index(const std::string&, field_specs fieldSpecs) override// 创建指定名称、基于指定字段的索引
        {
            if(fieldSpecs.size() != 1)
                throw exceptions::common("Only single-field index is supported in simple_memory_table");
            std::unique_lock sl{ tableMtx };
            if(indexTab[fieldNameTab.at(fieldSpecs[0])])
                throw exceptions::common("Index on field '" + fieldSpecs[0].name + "' already exists");

            auto& idx = indexTab[fieldNameTab.at(fieldSpecs[0])];
            idx = std::make_unique<index_impl>(*this, fieldSpecs[0]);
            for(auto& ent : *this)
            {
                if (!std::holds_alternative<NOTHING>(tabData[ent.id() * fieldTab.size() + fieldNameTab.at(fieldSpecs[0])]))
                    idx->insert(ent.id());
            }
        }

        virtual void drop_index(const std::string& name) override 
        {
            std::unique_lock ul{ tableMtx };
            indexTab[fieldNameTab.at(name)].reset();
        }

    private:// 由派生类实现：返回指向首条记录的迭代器 / 尾后迭代器
        virtual std::unique_ptr<entry> begin_impl() { return get_entry(0); }
        virtual std::unique_ptr<entry> end_impl() { return std::make_unique<data_entry_impl>(*this); }

    public:     // 扩展管理接口
        ID reserve(ID recCount)
        {
            ID currentSpace = assistant::power_of_2_not_less_than_n(recordCount.load(std::memory_order::relaxed));
            if(currentSpace >= recCount)
                return currentSpace;
            std::unique_lock ul{ tableMtx };
            resize_data_storage(recCount);
        }

    public: // 表始终从 loader 构造
        simple_memory_table(std::unique_ptr<archivist::loader>&& ld)
            : loader(std::move(ld)), fieldTab(loader->fields()), fieldNameTab(build_field_name_table(fieldTab)){
            if (fieldTab[0] != sysfldRowMark || fieldTab[0].type != sysfldRowMark.type)
                throw exceptions::common("The first field must be the system field '$RowMark'");
            sync_all();
        }

        ~simple_memory_table() noexcept { flush_all(); }
            

    };
}