#include "pch.h"
#include "random_access_fstream.h"

namespace HYDRA15::Union::archivist
{
    random_access_fstream::random_access_fstream(const std::string& fileName)
    {
        if (!std::filesystem::exists(fileName))
            create(fileName);
        open(fileName);
        if (!fstream.is_open())
            throw exceptions::archivist::RAFstreamFileNotAccessable();
    }

    random_access_fstream::random_access_fstream(const std::filesystem::path::value_type* fileName)
    {
        if (!std::filesystem::exists(fileName))
            create(fileName);
        open(fileName);
        if (!fstream.is_open())
            throw exceptions::archivist::RAFstreamFileNotAccessable();
    }

    random_access_fstream::random_access_fstream(random_access_fstream&& other) noexcept
        :fstream(std::move(other.fstream))
    {
    }

    bool random_access_fstream::open(const std::string& fileName)
    {
        std::lock_guard lg{ mtx };
        fstream.open(fileName, std::ios::in | std::ios::out | std::ios::binary);
        return fstream.is_open();
    }

    bool random_access_fstream::open(const std::filesystem::path::value_type* fileName)
    {
        std::lock_guard lg{ mtx };
        fstream.open(fileName, std::ios::in | std::ios::out | std::ios::binary);
        return fstream.is_open();
    }

    bool random_access_fstream::is_open() const
    {
        return fstream.is_open();
    }

    bool random_access_fstream::create(const std::string& fileName)
    {
        std::lock_guard lg{ mtx };
        std::filesystem::path path = fileName;
        std::filesystem::path parentPath = path.parent_path();

        // 创建指定路径
        if (!parentPath.empty() && !std::filesystem::exists(parentPath))
            std::filesystem::create_directories(parentPath);

        // 创建文件
        std::ofstream ofs(path);

        return true;
    }

    bool random_access_fstream::create(const std::filesystem::path::value_type* fileName)
    {
        std::lock_guard lg{ mtx };
        std::filesystem::path path = fileName;
        std::filesystem::path parentPath = path.parent_path();

        // 创建指定路径
        if (!parentPath.empty() && !std::filesystem::exists(parentPath))
            std::filesystem::create_directories(parentPath);

        // 创建文件
        std::ofstream ofs(path);

        return true;
    }

    void random_access_fstream::close()
    {
        std::lock_guard lg{ mtx };
        return fstream.close();
    }

    size_t random_access_fstream::size() const
    {
        std::lock_guard lg{ mtx };
        fstream.seekg(0, std::ios::end);
        return fstream.tellg();
    }

    std::vector<random_access_fstream::byte> random_access_fstream::read(size_t pos, size_t size) const
    {
        std::vector<random_access_fstream::byte> res;
        res.resize(size);
        std::lock_guard lg{ mtx };
        fstream.seekg(pos);
        fstream.read(reinterpret_cast<char*>(res.data()), size);
        return bool(fstream) ? res : std::vector<random_access_fstream::byte>{};
    }

    bool random_access_fstream::read(char* buf, size_t pos, size_t size) const
    {
        std::lock_guard lg{ mtx };
        fstream.seekg(pos);
        fstream.read(buf, size);
        return bool(fstream);
    }

    bool random_access_fstream::write(size_t pos, const std::vector<byte>& data)
    {
        std::lock_guard lg{ mtx };
        fstream.seekp(pos);
        fstream.write(reinterpret_cast<const char*>(data.data()), data.size());
        return bool(fstream);
    }


}
