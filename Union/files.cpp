#include "pch.h"
#include "files.h"

namespace HYDRA15::Union::assistant
{
    void bfstream::create()
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

    void bfstream::open()
    {
        file.open(path, std::ios::in | std::ios::out | std::ios::binary);
        if (!file.is_open())
            throw exceptions::assistant::FileNotAccessable();
    }

    void bfstream::close()
    {
        file.close();
    }

    std::vector<byte> bfstream::read(size_t pos, size_t size) const
    {
        std::vector<byte> data(size, 0);
        file.sync();
        file.seekg(pos);
        file.read(reinterpret_cast<char*>(data.data()), size);
        if (!file)
            throw exceptions::assistant::FileIOError();
        return data;
    }

    void bfstream::write(size_t pos, const std::vector<byte>& data)
    {
        file.seekp(pos);
        file.write(reinterpret_cast<const char*>(data.data()), data.size());
        file.flush();
        if (!file)
            throw exceptions::assistant::FileIOError();
    }

    void bfstream::read(size_t pos, size_t size, byte* pdata) const
    {
        file.sync();
        file.seekg(pos);
        file.read(reinterpret_cast<char*>(pdata), size);
        if (!file)
            throw exceptions::assistant::FileIOError();
    }

    void bfstream::write(size_t pos, size_t size, const byte* pdata)
    {
        file.seekp(pos);
        file.write(reinterpret_cast<const char*>(pdata), size);
        file.flush();
        if (!file)
            throw exceptions::assistant::FileIOError();
    }

    size_t bfstream::size() const
    {
        file.seekg(0, std::ios::end);
        return file.tellg();
    }

    void bfstream::resize(size_t size)
    {
        file.close();
        std::error_code ec;
        std::filesystem::resize_file(path, size, ec);
        if (ec)
            throw exceptions::assistant::FileIOError(ec);
        file.open(path);
    }

    bool bfstream::is_new() const { return newFile; }
    bool bfstream::is_open() const { return file.is_open(); }

    bfstream::bfstream(bfstream&& oth)
        :path(oth.path), file(std::move(oth.file))
    {
    }

    bfstream::bfstream(const std::filesystem::path& p)
        :path(p)
    {
        if (!std::filesystem::exists(path))
            create();
        open();
    }
}
