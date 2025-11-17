#pragma once
#include "pch.h"
#include "framework.h"

#include "iMutexies.h"
#include "ThreadLake.h"
#include "utilities.h"
#include "assistant_exception.h"

namespace HYDRA15::Union::assistant
{
    // 二进制文件，对 std::fstream 的重新包装，简化调用
    // 不支持多线程并发，并发调用行为未定义
    // 创建时绑定到文件，不可更改
    class bfstream
    {
    private:
        const std::filesystem::path path;
        mutable std::fstream file;
        bool newFile = false;

    private:
        // 工具函数
        void create()
        {
            std::filesystem::path parentPath = path.parent_path();

            // 创建指定路径
            if (!parentPath.empty() && !std::filesystem::exists(parentPath))
                std::filesystem::create_directories(parentPath);

            // 检查并创建文件
            if (std::filesystem::exists(path))
                return;
            std::ofstream ofs(path);
            if (!ofs)
                throw exceptions::assistant::FileNotAccessable();

            newFile = true;
        }

        void open()
        {
            file.open(path, std::ios::in | std::ios::out | std::ios::binary);
            if (!file.is_open())
                throw exceptions::assistant::FileNotAccessable();
        }

        void close(){ file.close(); }


    public:
        // 字节组读写
        std::vector<byte> read(size_t pos, size_t size) const
        {
            std::vector<byte> data(size, 0);
            file.sync();
            file.seekg(pos);
            file.read(reinterpret_cast<char*>(data.data()), size);
            if (!file)
                throw exceptions::assistant::FileIOError();
            return data;
        }

        void write(size_t pos, const std::vector<byte>& data)
        {
            file.seekp(pos);
            file.write(reinterpret_cast<const char*>(data.data()), data.size());
            file.flush();
            if (!file)
                throw exceptions::assistant::FileIOError();
        }

        // 兼容字节指针读写
        void read(size_t pos, size_t size, byte* pdata) const
        {
            file.sync();
            file.seekg(pos);
            file.read(reinterpret_cast<char*>(pdata), size);
            if (!file)
                throw exceptions::assistant::FileIOError();
        }

        void write(size_t pos, size_t size, const byte* pdata)
        {
            file.seekp(pos);
            file.write(reinterpret_cast<const char*>(pdata), size);
            file.flush();
            if (!file)
                throw exceptions::assistant::FileIOError();
        }

        // 任意类型读写
        template<typename T>
            requires std::is_trivial_v<T>
        std::vector<T> read(size_t pos, size_t count) const
        {
            std::vector<T> res(count, T{});
            file.sync();
            file.seekg(pos);
            file.read(reinterpret_cast<char*>(res.data()), sizeof(T) * count);
            if (!file)
                throw exceptions::assistant::FileIOError();
            return res;
        }

        template<typename T>
            requires std::is_trivial_v<T>
        void write(size_t pos, const std::vector<T>& data)
        {
            file.seekp(pos);
            file.write(reinterpret_cast<const char*>(data.data()), sizeof(T) * data.size());
            if (!file)
                throw exceptions::assistant::FileIOError();
        }

        // 信息和管理接口
        void resize(size_t size)
        {
            file.close();
            std::error_code ec;
            std::filesystem::resize_file(path, size, ec);
            if (ec)
                throw exceptions::assistant::FileIOError(ec);
            file.open(path);
        }

        size_t size() const { file.seekg(0, std::ios::end); return file.tellg(); }
        bool is_new() const { return newFile; }
        bool is_open() const { return file.is_open(); }
        std::filesystem::path file_path() const { return path; }
        operator bool()const { return file.operator bool(); }

        // 只允许从文件名构造
        bfstream() = delete;
        bfstream(const bfstream& oth) :bfstream(oth.path) {}
        bfstream(bfstream&&) noexcept = default;
        bfstream(const std::filesystem::path& p)
            :path(p)
        {
            if (!std::filesystem::exists(path))
                create();
            open();
        }
    };


    // 段分割的文件，多路 IO，自动缓存
    class bsfstream
    {
    private:
        struct segment
        {
            labourer::atomic_shared_mutex asmtx;
            std::atomic<size_t> accessCount = 0;
            std::atomic_bool secondChance = false;
            std::atomic_bool modified = false;
            std::vector<byte> data;
        };

        class async_io_thread_pool : protected labourer::background
        {
        private:
            const std::filesystem::path path;
            labourer::basic_blockable_queue<std::packaged_task<void(bfstream&)>> queue;

        private:
            std::atomic_bool working = true;
            virtual void work(thread_info& info) noexcept override
            {
                std::unique_ptr<bfstream> pbfs;
                while (working.load(std::memory_order_acquire) || !queue.empty())
                {
                    if (!pbfs || !(*pbfs))
                    {
                        try { pbfs = std::make_unique<bfstream>(path); }
                        catch (...) {}
                    }
                    auto tsk = queue.pop();
                    if (tsk.valid())tsk(*pbfs);
                }
            }

        public:
            std::future<void> submit(const std::function<void(bfstream&)>& tsk)
            {
                auto pkgedtsk = std::packaged_task<void(bfstream&)>{ tsk };
                auto fut = pkgedtsk.get_future();
                queue.push(std::move(pkgedtsk));
                return fut;
            }

        public:
            async_io_thread_pool(const std::filesystem::path& p, unsigned int thrs) :path(p), background(thrs) {}
            ~async_io_thread_pool() { working.store(false, std::memory_order_release); queue.notify_exit(); }
            using background::start;
        };

        static constexpr unsigned int asyncIOThreads = 4;
        static constexpr size_t accRecsLmt = 2048;

    private: // 常量数据
        const std::filesystem::path path;
        const size_t segSize;
        const size_t maxSegs;
        const size_t cachedSegsLmt;

    private: // 节列表和缓存相关
        bfstream bfs;
        labourer::atomic_shared_mutex segsMtx;
        std::deque<segment> segs;
        labourer::lockless_queue<size_t, 2 * accRecsLmt> accessRecords;

    private: // 异步 IO 相关
        async_io_thread_pool asyncIOThreadpool{ path, asyncIOThreads };

        void load(size_t ID, bfstream& bfs)
        {
            std::shared_lock sl{ segsMtx };
            auto& seg = segs[ID];
            std::unique_lock ul{ seg.asmtx };
            if (!seg.data.empty())return;   // 重复加载请求，不操作
            seg.data = bfs.read(ID * segSize, segSize);
            seg.secondChance.store(true, std::memory_order::relaxed);
            seg.modified.store(false, std::memory_order::relaxed);
            asyncIOThreadpool.submit([this](bfstream& bfs) {this->unload(bfs); });  // 每次加载就尝试卸载一个节
        }

        void store(size_t ID, bfstream& bfs)
        {
            std::shared_lock sl{ segsMtx };
            auto& seg = segs[ID];
            std::unique_lock ul{ seg.asmtx };
            if (!seg.modified.load(std::memory_order::relaxed))return;  // 未修改，不操作
            bfs.write(ID * segSize, seg.data);
        }

        void unload(bfstream& bfs)  // 每次调用淘汰一个节
        {
            struct seg_info { size_t ID; size_t accessCount;};
            auto scan = [this](const std::list<seg_info>& lst) -> std::optional<size_t>
                {
                    bool expected = true;
                    for (auto& item : lst)
                        if (!segs[item.ID].secondChance.compare_exchange_strong(
                            expected, false,
                            std::memory_order::acquire, std::memory_order_relaxed
                        )) return item.ID;    // 找到没有第二次机会的对象
                    return {};
                };
            auto eliminate = [this, &bfs](size_t id) -> bool
                {
                    segment& seg = segs[id];
                    std::unique_lock ul{ seg.asmtx };
                    if (seg.data.empty())return false;
                    if (seg.modified.load(std::memory_order::relaxed))
                        bfs.write(id * segSize, seg.data);
                    seg.data.clear();
                    return true;
                };
            while (true)    // 反复尝试直到成功淘汰一个缓存为止
            {
                std::list<seg_info> lst;
                std::shared_lock sl{ segsMtx };

                // 统计缓存
                for (size_t i = 0; i < segs.size(); i++)
                {
                    std::shared_lock ssl{ segs[i].asmtx };
                    if (segs[i].data.size() > 0)
                        lst.push_back(
                            { i,segs[i].accessCount.load(std::memory_order::acquire) }
                        );
                }
                if (lst.size() < cachedSegsLmt)return;  // 未达到淘汰缓存的标准，无需操作

                // 寻找淘汰对象
                lst.sort([](const seg_info& s1, const seg_info& s2) {
                    return s1.accessCount < s2.accessCount; 
                    });
                for (size_t i = 0; i < 16; i++) // 重试 16 次，还不成功就从头开始
                {
                    std::optional<size_t> target{};
                    target = scan(lst); // 第一轮扫描
                    if (target)         // 找到结果，尝试卸载
                        if (eliminate(target.value()))return;
                        else break;  // 被其他线程抢先卸载，从头开始重试
                }
            }
            
        }

    public:
        bsfstream() = delete;
        bsfstream(const std::filesystem::path& path, size_t segmentSize, size_t maxSegCount, size_t memoryLimit = std::numeric_limits<size_t>::max())
            :path(path), segSize(segmentSize), maxSegs(maxSegCount), bfs(path), cachedSegsLmt(memoryLimit / segmentSize) {
            size_t segCount = assistant::multiple_m_not_less_than_n(segSize, bfs.size());
            segs.resize(segCount);
            size_t cachedSegCount = std::min(segCount, cachedSegsLmt);
            // 启动预加载缓存
            for (size_t i = segs.size() - cachedSegCount; i < segs.size(); i++)
                asyncIOThreadpool.submit([this, i](bfstream& bfs) {this->load(i, bfs); });
        }
        bsfstream(const bsfstream& oth) :bsfstream(oth.path, oth.segSize, oth.maxSegs, oth.cachedSegsLmt * oth.segSize) {}
        bsfstream(bsfstream&&) = default;

    public:
        // 单节读写
        std::vector<byte> read(size_t id)
        {
            auto fetch = [this, id]()->std::optional<std::vector<byte>>
                {
                    std::shared_lock sl{ segsMtx };
                    if (id > segs.size())throw exceptions::assistant::FileInvalidAccess();
                    segment& seg = segs[id];
                    // 处理访问记录
                    seg.accessCount.fetch_add(1, std::memory_order::relaxed);
                    accessRecords.push(id);
                    if (accessRecords.size() > accRecsLmt)
                        segs[accessRecords.pop()].accessCount.fetch_add(-1, std::memory_order::relaxed);
                    std::shared_lock ssl{ seg.asmtx };
                    if (!seg.data.empty())
                        return seg.data;// 已缓存，直接返回数据
                    return {};          // 未缓存，返回空
                };

            while (true)
            {
                auto res = fetch();         // 首次尝试加载
                if (res)return res.value(); // 加载成功，返回数据
                auto fut = asyncIOThreadpool.submit(
                    [this, id](bfstream& bfs) {this->load(id, bfs); }
                );                          // 加载不成功，加载缓存
                fut.get();                  // 等待缓存加载完成
            }
        }

        void write(size_t id, size_t pos, const std::vector<byte>& data)
        {
            auto dump = [this, id, pos, &data] -> bool
                {
                    std::shared_lock sl{ segsMtx };
                    if (id > segs.size())throw exceptions::assistant::FileInvalidAccess();
                    if (pos + data.size() > segSize)throw exceptions::assistant::FileInvalidAccess();
                    segment& seg = segs[id];
                    // 处理访问记录
                    seg.accessCount.fetch_add(1, std::memory_order::relaxed);
                    accessRecords.push(id);
                    if (accessRecords.size() > accRecsLmt)
                        segs[accessRecords.pop()].accessCount.fetch_add(-1, std::memory_order::relaxed);
                    std::unique_lock sul{ seg.asmtx };
                    if (!seg.data.empty())
                    {
                        assistant::memcpy(data.data(), seg.data.data() + pos, data.size());
                        return true;
                    }
                    return false;
                };

            while (true)
            {
                if (dump())return;
                auto fut = asyncIOThreadpool.submit(
                    [this, id](bfstream& bfs) {this->load(id, bfs); }
                );
                fut.get();
            }
        }

        // 批量读写
        std::vector<byte> read(const std::list<size_t>& segIDs)
        {
            pre_load(segIDs);   // 缓存预加载

            std::vector<byte> res(segIDs.size() * segSize, 0);
            size_t i = 0;
            for (const auto& id:segIDs)
            {
                std::vector<byte> d = read(segIDs);
                assistant::memcpy(d.data(), res.data() + i * segSize, d.size());
                i++;
            }

            return res;
        }

        void write(const std::list<size_t>& segIDs, size_t firstSegPos, const std::vector<byte>& data)
        {
            if (data.size() < (segIDs.size() - 1) * segSize - firstSegPos)
                throw exceptions::assistant::FileInvalidAccess();
            if (data.size() > segIDs.size() * segSize - firstSegPos)
                throw exceptions::assistant::FileInvalidAccess();

            pre_load(segIDs); // 缓存预加载

            bool first = true;
            size_t CurrentDataPos = 0;
            size_t i = 0;
            for (const auto& id : segIDs)
            {
                if (first)
                {
                    size_t len = std::min(segSize - firstSegPos, data.size());
                    std::vector<byte> d(len, 0);
                    assistant::memcpy(data.data(), d.data(), len);
                    write(id, firstSegPos, d);
                    first = false;
                    i++;
                }
                else
                {
                    size_t len = std::min(segSize, data.size() - segSize * i);
                    std::vector<byte> d(len, 0);
                    assistant::memcpy(data.data() + segSize * i - firstSegPos, d.data(), len);
                    write(id, 0, d);
                    i++;
                }
            }
        }

        // 管理
        void expand(size_t segIDs)
        {
            std::unique_lock ul{ segsMtx };
            for (size_t i = 0; i < segIDs; i++)
                segs.push_back({});
        }

        void pre_load(const std::list<size_t>& segIDs)
        {
            std::shared_lock sl{ segsMtx };
            for (const auto& id : segIDs)
            {
                if (id > segs.size())throw exceptions::assistant::FileInvalidAccess();
                segment& seg = segs[id];
                std::shared_lock ssl{ seg.asmtx };
                if (seg.data.empty())asyncIOThreadpool.submit(
                    [this, id](bfstream& bfs) {this->load(id, bfs); }
                );
            }
        }

        // 信息
        size_t size() const { return bfs.size(); }
        size_t is_new_file() const { return bfs.is_new(); }
    };
}