#include "pch.h"
#include "random_access_fstream.h"

namespace HYDRA15::Union::archivist
{
    random_access_fstream::random_access_fstream(const std::string& fileName)
        : random_access_fstream(fileName)
    {
    }

    random_access_fstream::random_access_fstream(const std::filesystem::path& path)
        : filePath(path)
    {
        if (!std::filesystem::exists(filePath))
            create();
        open();
        if (!fstream.is_open())
            throw exceptions::archivist::FileNotAccessable();
    }

    random_access_fstream::random_access_fstream(random_access_fstream&& other) noexcept
        :fstream(std::move(other.fstream)), filePath(std::move(other.filePath))
    {
    }

    bool random_access_fstream::open()
    {
        fstream.open(filePath, std::ios::in | std::ios::out | std::ios::binary);
        return fstream.is_open();
    }

    bool random_access_fstream::is_open() const
    {
        return fstream.is_open();
    }

    bool random_access_fstream::create()
    {
        std::filesystem::path parentPath = filePath.parent_path();

        // 创建指定路径
        if (!parentPath.empty() && !std::filesystem::exists(parentPath))
            std::filesystem::create_directories(parentPath);

        // 检查并创建文件
        if (std::filesystem::exists(filePath))
            throw exceptions::archivist::FileNotAccessable();
        std::ofstream ofs(filePath);
        if (!ofs)
            throw exceptions::archivist::FileNotAccessable();

        newFile = true;
        return true;
    }

    void random_access_fstream::close()
    {
        return fstream.close();
    }

    std::filesystem::path random_access_fstream::file_path()
    {
        return filePath;
    }

    size_t random_access_fstream::size() const
    {
        fstream.seekg(0, std::ios::end);
        return fstream.tellg();
    }

    bool random_access_fstream::new_file() const
    {
        return newFile;
    }

    std::vector<random_access_fstream::byte> random_access_fstream::read(size_t pos, size_t size) const
    {
        std::vector<random_access_fstream::byte> res;
        res.resize(size);
        read(pos, size, res.data());
        return res;
    }

    bool random_access_fstream::read(size_t pos, size_t size, byte* buf) const
    {
        if (!fstream)
            throw exceptions::archivist::FileIOError();
        fstream.sync();
        fstream.seekg(pos);
        fstream.read(reinterpret_cast<char*>(buf), size);
        if (!fstream)
            throw exceptions::archivist::FileIOError();
        return true;
    }

    bool random_access_fstream::write(size_t pos, const std::vector<byte>& data)
    {
        return write(pos, data.data(), data.size());
    }

    bool random_access_fstream::write(size_t pos, const byte* pdata, size_t size)
    {
        if (!fstream)
            throw exceptions::archivist::FileIOError();
        fstream.seekp(pos);
        fstream.write(reinterpret_cast<const char*>(pdata), size);
        fstream.flush();
        if (!fstream)
            throw exceptions::archivist::FileIOError();
        return true;
    }

    bool random_access_fstream::reallocate(size_t size)
    {
        close();
        std::error_code ec;
        std::filesystem::resize_file(filePath, size, ec);
        open();
        if (is_open() || ec)
            throw exceptions::archivist::FileIOError();
    }



}
