#pragma once
#include "pch.h"

#include "resources.h"

#include "Union/simple_memory_table.h"
#include "Union/single_loader.h"
using namespace HYDRA15::Union;


// 全局单例数据库
// 要求在使用前先调用 init() 进行初始化
class database_service : public archivist::simple_memory_table
{
private:
    static inline std::unique_ptr<database_service> dbInstance = nullptr;

    database_service(std::unique_ptr<archivist::loader>&& loader) :archivist::simple_memory_table(std::move(loader)) {}

public:
    database_service() = delete;

    database_service(database_service&&) = delete;

    database_service(const database_service&) = delete;

    ~database_service() = default;

    static void init(const std::filesystem::path& dbpath)
    {
        if (dbInstance)return;

        std::unique_ptr<archivist::loader> loader;
        // 若数据库文件不存在则创建数据库 假定父目录已存在，由主函数保证
        if (std::filesystem::exists(dbpath) && std::filesystem::is_regular_file(dbpath))
            loader = archivist::single_loader::make_unique(dbpath);
        else
            loader = archivist::single_loader::make_unique(
                    dbpath, 
                    dbFields, 
                    archivist::simple_memory_table::version, 
                    archivist::sfstream::segment_size_level::II
                );

        dbInstance.reset(new database_service(std::move(loader)));
    }

    static database_service& get_instance(const std::filesystem::path& dbpath)
    {
        if (!dbInstance)
            throw exceptions::common("Database service is not initialized. Call database_service::init() first.");
        return *dbInstance;
    }

};

