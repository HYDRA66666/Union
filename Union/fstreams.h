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
                throw exceptions::files::FileIOFlowError(path, ofs.rdstate());

            newFile = true;
        }

        void open()
        {
            file.open(path, std::ios::in | std::ios::out | std::ios::binary);
            if (!file.is_open())
                throw exceptions::files::FileIOFlowError(path, file.rdstate());
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
                else throw exceptions::files::FileIOFlowError(path, file.rdstate());
            return res;
        }

        template<typename T>
            requires std::is_trivial_v<T>
        void write(size_t pos, const std::vector<T>& data)
        {
            file.seekp(pos);
            file.write(reinterpret_cast<const char*>(data.data()), sizeof(T) * data.size());
            if (!file)
                throw exceptions::files::FileIOFlowError(path, file.rdstate());
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
                else throw exceptions::files::FileIOFlowError(path, file.rdstate());
            return file.gcount() / sizeof(T);
        }

        template<typename T>
            requires std::is_trivial_v<T>
        void write(size_t pos, size_t count, const T* data)
        {
            file.seekp(pos);
            file.write(reinterpret_cast<const char*>(data), sizeof(T) * count);
            if (!file)
                throw exceptions::files::FileIOFlowError(path, file.rdstate());
        }

        // 读字符串
        std::string read_string(size_t pos)
        {
            file.sync();
            file.seekg(pos);

            std::string result;
            char ch;

            while (file.get(ch))
            {
                if (ch == '\0') // 遇到结束符
                    return result;
                result += ch;
            }

            // 如果到达这里，说明文件结束但未遇到结束符
            if (file.eof())
                throw exceptions::files::ContentNotFound(path)
                .set("content type", "string")
                .set("position", std::to_string(pos));

            // 如果是其他错误
            throw exceptions::files::FileIOFlowError(path, file.rdstate());
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
        mutable std::shared_mutex smtx;
        std::unique_ptr<async_io_thread_pool> aioPool;

    public:
        // 单段读写，不启用异步 IO，不支持并发
        template<typename T>
            requires std::is_trivial_v<T>
        std::vector<T> read(size_t segID, size_t pos, size_t count) const
        {
            if (pos + count * sizeof(T) > segSize)
                throw exceptions::files::ExceedRage(path, "pos, count", "pos + count * sizeof(T) < segSize");
            std::shared_lock sl{ smtx };
            return bfs.read<T>(segID * segSize + pos, count);
        }

        template<typename T>
            requires std::is_trivial_v<T>
        void write(size_t segID, size_t pos, const std::vector<T>& data)
        {
            if (data.empty())return;
            if (pos + data.size() * sizeof(T) > segSize)
                throw exceptions::files::ExceedRage(path, "pos, data.size", "pos + data.size <= segSize");
            std::unique_lock ul{ smtx };
            bfs.write<T>(segID * segSize + pos, data);
        }

        // 多段异步读写，根据配置启用异步 IO，支持并发
        // 读写模型：将 list 中的所有节视为一个整体，在整体中读写数据
        template<typename T>
            requires std::is_trivial_v<T>
        std::vector<T> read(const std::deque<size_t>& segIDs, size_t pos, size_t count) const
        {
            const size_t dataByteSize = count * sizeof(T);
            std::deque<size_t> seglst = segIDs;

            if (pos + dataByteSize > segIDs.size() * segSize)
                throw exceptions::files::ExceedRage(path, "segIDs, pos, count", "pos + count * sizeof(T) <= segSize * segIDs.size");

            // 处理头尾冗余的节
            for (size_t i = 0; i < pos / segSize; i++)seglst.pop_front();
            pos -= (pos / segSize) * segSize;
            for (size_t i = seglst.size(); i > ((pos + dataByteSize) - 1) / segSize + 1; i--)seglst.pop_back();

            std::queue<std::future<std::vector<byte>>> futures;
            std::vector<T> res(count, T{});

            for (size_t i = 0; i < seglst.size(); i++)
            {
                const size_t id = seglst[i];
                const size_t currentTgtPos = std::min(dataByteSize, i > 0 ? i * segSize - pos : 0);
                const size_t remainTgtSize = dataByteSize - currentTgtPos;
                const size_t currentSrcPos = i > 0 ? 0 : pos;
                const size_t currentSrcSize = std::min(remainTgtSize, segSize);

                if (remainTgtSize == 0)continue;

                if (aioPool)
                    futures.push(aioPool->submit(
                        [currentSrcPos, currentSrcSize, id, this](bfstream& bfs)->std::vector<byte> {
                            return bfs.read<byte>(id * segSize + currentSrcPos, currentSrcSize);
                        }
                    ));
                else
                {
                    std::vector<byte> segData = read<byte>(id, currentSrcPos, currentSrcSize);
                    assistant::memcpy(segData.data(), reinterpret_cast<byte*>(res.data()) + currentTgtPos, segData.size());
                }
            }

            if (!futures.empty())
            {
                for (size_t i = 0; i < seglst.size(); i++)
                {
                    const size_t id = seglst[i];
                    const size_t currentTgtPos = std::min(dataByteSize, i > 0 ? i * segSize - pos : 0);
                    const size_t remainTgtSize = dataByteSize - currentTgtPos;

                    std::vector<byte> segData = futures.front().get();
                    assistant::memcpy(segData.data(), reinterpret_cast<byte*>(res.data()) + currentTgtPos, segData.size());
                    futures.pop();
                }
            }

            return res;
        }

        template<typename T>
            requires std::is_trivial_v<T>
        void write(const std::deque<size_t>& segIDs, size_t pos, const std::vector<T>& data, bool sync = true)
        {
            const size_t dataByteSize = data.size() * sizeof(T);
            std::deque<size_t> seglst = segIDs;

            if (data.empty())return;
            if (pos + dataByteSize > segIDs.size() * segSize)
                throw exceptions::files::ExceedRage(path, "segIDs, pos, data.bytesize", "pos + data.bytesize <= segSize * segIDs.size");

            // 处理头尾冗余的节
            for (size_t i = 0; i < pos / segSize; i++)seglst.pop_front();
            pos -= (pos / segSize) * segSize;
            for (size_t i = seglst.size(); i > ((pos + dataByteSize) - 1) / segSize + 1; i--)seglst.pop_back();

            std::queue<std::future<std::vector<byte>>> futures;
            for (size_t i = 0; i < seglst.size(); i++)
            {
                const size_t id = seglst[i];
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
                if (aioPool)   // 未启用异步IO，退化为同步操作
                    futures.push(aioPool->submit(
                        [id, currentTgtPos, segData = std::move(segData), this](bfstream& bfs) -> std::vector<byte>
                        {
                            bfs.write<byte>(id * segSize + currentTgtPos, segData);
                            return {};
                        }
                    ));
                else
                    write<byte>(id, currentTgtPos, segData);
            }

            if (sync)  // 同步写入，传递异常、保证写入完成
                while (!futures.empty())
                {
                    futures.front().get();
                    futures.pop();
                }
        }

        // 读字符串
        std::string read_string(size_t segID, size_t pos) const
        {
            std::vector<char> segData = read<char>(segID, 0, segSize);
            if (pos >= segData.size())
                throw exceptions::files::ExceedRage(path, "pos", "pos <= segData.size");

            std::shared_lock sl{ smtx };

            std::string result;
            size_t currentPos = pos;


            for (size_t i = 0; i < segData.size() - pos; i++)
                if (segData[pos + i] == '\0')
                    return std::string{ segData.data() + pos,i };

            // 如果到达这里，说明节结束但未遇到结束符
            throw exceptions::files::ContentNotFound(path)
                .set("content type", "string")
                .set("position", std::to_string(pos))
                .set("segment ID", std::to_string(segID));
        }

        std::string read_string(const std::deque<size_t>& segIDs, size_t pos) const
        {
            std::deque<size_t> seglst = segIDs;
            if (pos >= segIDs.size() * segSize)
                throw exceptions::files::ExceedRage(path, "segIDs, pos", "pos < segSize * segIDs.size");

            // 处理头尾冗余的节
            for (size_t i = 0; i < pos / segSize; i++)seglst.pop_front();
            pos -= (pos / segSize) * segSize;

            std::string result;
            for (size_t i = 0; i < seglst.size(); i++)
            {
                const size_t id = seglst[i];
                const size_t currentSrcPos = i > 0 ? 0 : pos;
                const size_t currentSrcSize = segSize - currentSrcPos;
                std::vector<char> segData = read<char>(id, currentSrcPos, currentSrcSize);
                for (size_t j = 0; j < segData.size(); j++)
                    if (segData[j] == '\0')
                        return result + std::string{ segData.data(),j };
                result += std::string{ segData.data(),segData.size() };
            }

            // 如果到达这里，说明节结束但未遇到结束符
            throw exceptions::files::ContentNotFound(path)
                .set("content type", "string")
                .set("position", std::to_string(pos))
                .set("segment IDs", assistant::container_to_string(segIDs));
        }

    public:     // 管理接口
        size_t seg_size() const { return segSize; }

        size_t seg_count() const { std::shared_lock sl{ smtx }; return bfs.size() == 0 ? 0 : (bfs.size() - 1) / segSize + 1; }

        bfstream& data() { return bfs; }

    public:
        bsfstream(const std::filesystem::path& path, size_t segSize, unsigned int aioThrs = 4)
            :path(path), segSize(segSize), aioPool(aioThrs > 0 ? std::make_unique<async_io_thread_pool>(path, segSize,aioThrs) : nullptr) {
            if (aioPool)aioPool->start();
        }
    };
}