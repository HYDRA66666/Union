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

    bfstream::bfstream(const std::filesystem::path& p)
        : path(p)
    {
        if (!std::filesystem::exists(path))
            create();
        open();
    }

    bool bfstream::is_new_file() const
    {
        return newFile;
    }

    bool bfstream::is_open() const
    {
        return file.is_open();
    }

    size_t bfstream::size() const
    {
        std::unique_lock ul{ mtx };
        file.seekg(0, std::ios::end);
        return file.tellg();
    }

    void bfstream::resize(size_t size)
    {
        std::unique_lock ul{ mtx };
        file.close();
        std::error_code ec;
        std::filesystem::resize_file(path, size, ec);
        if (ec)
            throw exceptions::assistant::FileIOError(ec);
        file.open(path);
    }

}
