#pragma once
#include "pch.h"
#include "framework.h"

#include "lib_exceptions.h"
#include "background.h"
#include "shared_containers.h"
#include "utilities.h"



namespace HYDRA15::Union::assistant
{
    // 管理、读写二进制文件
    // 读写任意类型数据数组、修改文件大小、自动处理部分流错误
    // 创建即绑定文件路径
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
                throw exceptions::fstream::FileIOFlowError(path, ofs.rdstate());

            newFile = true;
        }

        void open()
        {
            file.open(path, std::ios::in | std::ios::out | std::ios::binary);
            if (!file.is_open())
                throw exceptions::fstream::FileIOFlowError(path, file.rdstate());
            file.exceptions(std::ios::badbit);
        }

        void close() { file.close(); }

    public:
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
                if (file.rdstate() & std::ios::eofbit)
                {
                    file.clear();
                    res.resize(file.gcount() / sizeof(T));
                }
                else throw exceptions::fstream::FileIOFlowError(path, file.rdstate());
            return res;
        }

        template<typename T>
            requires std::is_trivial_v<T>
        void write(size_t pos, const std::vector<T>& data)
        {
            file.seekp(pos);
            file.write(reinterpret_cast<const char*>(data.data()), sizeof(T) * data.size());
            if (!file)
                throw exceptions::fstream::FileIOFlowError(path, file.rdstate());
        }

        // 兼容任意类型指针读写
        template<typename T>
            requires std::is_trivial_v<T>
        size_t read(size_t pos, size_t count, T* data) const    // 返回实际读取的数据数量
        {
            file.sync();
            file.seekg(pos);
            file.read(reinterpret_cast<char*>(data), sizeof(T) * count);
            if (!file)
                if (file.rdstate() & std::ios::eofbit)file.clear();
                else throw exceptions::fstream::FileIOFlowError(path, file.rdstate());
            return file.gcount() / sizeof(T);
        }

        template<typename T>
            requires std::is_trivial_v<T>
        void write(size_t pos, size_t count, const T* data)
        {
            file.seekp(pos);
            file.write(reinterpret_cast<const char*>(data), sizeof(T) * count);
            if (!file)
                throw exceptions::fstream::FileIOFlowError(path, file.rdstate());
        }

    public:     // 信息和管理接口
        void resize(size_t size)    // 修改文件大小
        {
            file.close();
            std::filesystem::resize_file(path, size);
            file.open(path);
        }

        size_t size() const { file.seekg(0, std::ios::end); return file.tellg(); }

        bool is_new() const { return newFile; }

        std::filesystem::path file_path() const { return path; }

        std::fstream& data() { return file; }

        const std::fstream& data() const { return file; }

        void reopen()
        {
            if (file.is_open())close();
            if (!std::filesystem::exists(path))create(); 
            open();
        }

        operator bool() const { return file.operator bool(); }


        // 只允许从文件名构造
        bfstream() = delete;
        bfstream(const bfstream& oth) :bfstream(oth.path) {}
        bfstream(bfstream&&) noexcept = default;
        bfstream(const std::filesystem::path& p) :path(p) { if (!std::filesystem::exists(path))create(); open(); }
    };


    // 段化文件
    // 多路 IO、批量段读写，依赖 OS 缓存
    // 创建即绑定文件、段大小
    class bsfstream
    {
    private:    
        class async_io_thread_pool : protected labourer::background
        {
        private:
            const size_t segSize;
            const std::filesystem::path path;
            std::atomic<bool> working = true;
            std::exception_ptr gexptr;
            std::mutex gexptrMtx;

        private:
            labourer::basic_blockable_queue<std::packaged_task<std::vector<byte>(bfstream&)>> queue;

        private:
            virtual void work() noexcept override
            {
                try
                {
                    bfstream bfs{ path };
                    while (working.load(std::memory_order_relaxed) || !queue.empty())
                    {
                        auto tsk = queue.pop();
                        if (tsk.valid())tsk(bfs);
                        if (!bfs)bfs.reopen();
                    }
                }
                catch (...) { std::unique_lock ul{ gexptrMtx }; gexptr = std::current_exception(); }
            }

        public:
            std::future<std::vector<byte>> submit(const std::function<std::vector<byte>(bfstream&)>& mis)
            {
                auto tsk = std::packaged_task<std::vector<byte>(bfstream&)>(mis);
                auto fut = tsk.get_future();
                queue.push(std::move(tsk));
                return fut;
            }

            async_io_thread_pool(const std::filesystem::path& path, size_t segSize, unsigned int thrs)
                :path(path), segSize(segSize), background(thrs) {
            }
            ~async_io_thread_pool() { working = false; queue.notify_exit(); background::wait_for_end(); }
            using background::start;
        };

    private:
        const size_t segSize;
        const std::filesystem::path path;
        bfstream bfs{ path };
        std::shared_mutex smtx;
        std::unique_ptr<async_io_thread_pool> aioPool;

    public:
        // 单段读写，不启用异步 IO，不支持并发
        template<typename T>
            requires std::is_trivial_v<T>
        std::vector<T> read(size_t segID) const
        {
            std::shared_lock sl{ smtx };
            return bfs.read<T>(segID * segSize, segSize / sizeof(T));
        }

        template<typename T>
            requires std::is_trivial_v<T>
        void write(size_t segID, size_t pos, const std::vector<T>& data)
        {
            if (pos + data.size() * sizeof(T) > segSize)
                throw exceptions::fstream::ExceedRage(path, "pos, data.size", "pos + data.size <= segSize");
            std::unique_lock ul{ smtx };
            bfs.write<T>(segID * segSize + pos, data);
        }

        // 多段异步读写，根据配置启用异步 IO，支持并发
        // 读写模型：将 list 中的所有节视为一个整体，在整体中读写数据
        template<typename T>
            requires std::is_trivial_v<T>
        std::vector<T> read(std::list<size_t> segIDs) const
        {
            std::queue<std::vector<byte>> segDatas;
            if (!aioPool)   // 未启用异步IO，退化为同步操作
                for (const auto& id : segIDs)
                    segDatas.push(read<byte>(id));
            else
            {
                std::queue<std::future<std::vector<byte>>> futures;
                for(const auto& id:segIDs)
                    futures.push(aioPool->submit(
                        [id, this](bfstream& bfs) -> std::vector<byte>
                        {
                            return bfs.read<byte>(id * segSize, segSize);
                        }
                    ));
                for(auto& f : futures)
                    segDatas.push(f.get());
            }

            // 合并结果
            std::vector<T> res(segDatas.size() * segSize / sizeof(T), T{});
            size_t i = 0;
            while (!segDatas.empty())
            {
                std::vector<byte> vec = std::move(segDatas.front());
                segDatas.pop();
                assistant::memcpy(vec.data(), reinterpret_cast<byte*>(res.data()) + i * segSize, vec.size());
                i++;
            }
            return res;
        }

        template<typename T>
            requires std::is_trivial_v<T>
        void write(const std::list<size_t>& segIDs, size_t pos, const std::vector<T>& data, bool sync = true)
        {
            const size_t dataByteSize = data.size() * sizeof(T);
            std::list<size_t> seglst = segIDs;

            if (dataByteSize == 0)return;
            if (pos + dataByteSize > segIDs.size() * segSize)
                throw exceptions::fstream::ExceedRage(path, "segIDs, pos, data.bytesize", "pos + data.bytesize <= segSize * segIDs.size");

            // 处理头尾冗余的节
            for (size_t i = 0; i < pos / segSize; i++)seglst.pop_front();
            pos -= (pos / segSize) * segSize;
            for (size_t i = seglst.size(); i > ((pos + dataByteSize) - 1) / segSize + 1; i--)seglst.pop_back();

            size_t i = 0;
            std::queue<std::future<std::vector<byte>>> futures;
            for (const auto& id : seglst)
            {
                if (pos > (i + 1) * segSize)continue;

                const size_t currentSrcPos = std::min(dataByteSize, i > 0 ? i * segSize - pos : 0);
                const size_t remainSrcSize = dataByteSize - currentSrcPos;
                const size_t currentTgtPos = i > 0 ? 0 : pos;
                const size_t currentTgtSize = std::min(remainSrcSize, segSize);

                if (remainSrcSize == 0)continue;    // 没有更多数据

                std::vector<byte> segData(currentTgtSize, 0);
                assistant::memcpy(
                    reinterpret_cast<const byte*>(data.data()) + currentSrcPos,
                    segData.data(),
                    segData.size()
                );
                if(!aioPool)   // 未启用异步IO，退化为同步操作
                    write<byte>(id, currentTgtPos, segData);
                else
                    futures.push(aioPool->submit(
                        [id, currentTgtPos, segData = std::move(segData)](bfstream& bfs) -> std::vector<byte>
                        {
                            bfs.write<byte>(id * segData.size() + currentTgtPos, segData);
                            return {};
                        }
                    ));
                i++;
            }

            while (sync && !futures.empty()) // 同步写入，传递异常、保证写入完成
            {
                futures.front().get();
                futures.pop();
            }
        }

    public:
        bsfstream(const std::filesystem::path& path, size_t segSize, unsigned int aioThrs = 4)
            :path(path), segSize(segSize), aioPool(aioThrs > 0 ? std::make_unique<async_io_thread_pool>(path, segSize,aioThrs) : nullptr) {
            if (aioPool)aioPool->start();
        }
    };
}