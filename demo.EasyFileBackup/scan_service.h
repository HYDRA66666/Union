#pragma once
#include "pch.h"
#include "resources.h"

#include "database_service.h"

#include "Union/ThreadLake.h"
#include "Union/shared_containers.h"
#include "Union/iMutexies.h"
#include "Union/PrintCenter.h"
#include "Union/logger.h"
using namespace HYDRA15::Union;

// 扫描硬盘文件并与数据库进行比对
class file_scan_service
{
private:
    class scan_mission : public labourer::mission_base
    {
    private:
        const std::filesystem::path path;

        std::function<void(labourer::mission&&)> submit;
        std::atomic<size_t>& updated;

        secretary::logger lgr{ "FileScanServiceThread" };
        secretary::PrintCenter& pc{ secretary::PrintCenter::get_instance() };
        database_service& db{ database_service::get_instance({}) };

    public:
        scan_mission(
            const std::filesystem::path& p,
            const std::function<void(labourer::mission)>& s,
            std::atomic<size_t>& u
        ) :path(p), submit(s), updated(u) {}

        virtual void operator()() noexcept override
        {
            // 如果 path 指向的是目录，则递归扫描子文件和子目录，否则获取文件信息并与数据库进行比对
            try
            {
                if (std::filesystem::is_directory(path))
                    for (const auto& entry : std::filesystem::directory_iterator(path))
                    {
                        submit(std::make_unique<scan_mission>(entry.path(), submit, updated));
                        lgr.debug("Dispatched scan task for path: {}", entry.path().string());
                    }
                else if (std::filesystem::is_regular_file(path))
                {
                    archivist::INT updateTime = get_date_time();
                    archivist::INT dateTime = get_date_time(std::filesystem::last_write_time(path).time_since_epoch().count());
                    archivist::INT size = static_cast<archivist::INT>(std::filesystem::file_size(path));

                    // 构建事件查找表记录
                    archivist::incident inct{
                        .type = archivist::incident::incident_type::search,
                        .param = archivist::incident::condition_param{
                            .targetField = dbFields[2],
                            .type = archivist::incident::condition_param::condition_type::equal,
                            .reference = archivist::create_string_field(path.string())
                        }
                    };

                    auto results = db.excute({ inct });

                    if (results.empty())
                    {
                        // 数据库中不存在该文件记录，创建新记录
                        auto newEntry = db.create();
                        std::unique_lock rul{ *newEntry };
                        newEntry->set(dbFields[1], updateTime);
                        newEntry->set(dbFields[2], archivist::create_string_field(path.string()));
                        newEntry->set(dbFields[3], dateTime);
                        newEntry->set(dbFields[4], size);
                        newEntry->set(dbFields[5], static_cast<archivist::INT>(file_status::updated)); // 初始状态为未备份
                        updated.fetch_add(1, std::memory_order_relaxed);
                        lgr.debug("New file record created for path: {}", path.string());
                    }
                    else
                    {
                        // 数据库中存在该文件记录，检查是否需要更新
                        auto entry = db.at(results.front());
                        std::unique_lock rul{ *entry };
                        bool modified = false;
                        if (std::get<archivist::INT>(entry->at(dbFields[3])) != dateTime)
                        {
                            entry->set(dbFields[3], dateTime);
                            modified = true;
                        }
                        if (std::get<archivist::INT>(entry->at(dbFields[4])) != size)
                        {
                            entry->set(dbFields[4], size);
                            modified = true;
                        }
                        if (modified)
                        {
                            entry->set(dbFields[1], updateTime);
                            updated.fetch_add(1, std::memory_order_relaxed);
                            lgr.debug("File record updated for path: {}", path.string());
                        }
                    }
                }
            }
            catch (const std::exception& e) { lgr.error("Error on scanning path {}: {}", path.string(), e.what()); }
            catch (...) { lgr.error("Unknown error on scanning path {}", path.string()); }
        }
    };

    using scan_thread_pool = labourer::thread_pool<labourer::basic_blockable_queue<labourer::mission, labourer::atomic_mutex>>;

private:
    scan_thread_pool threadPool{ std::thread::hardware_concurrency() / 2 };
    std::atomic<size_t> updatedFilesCount{ 0 };

    secretary::logger lgr{ "FileScanService" };
    secretary::PrintCenter& pc{ secretary::PrintCenter::get_instance() };
    
public:
    file_scan_service(const std::filesystem::path& path)
    {
        lgr.info("Starting file scan service from {}", path.string());
        threadPool.submit(std::make_unique<scan_mission>(
            path,
            [this](labourer::mission&& mis) {threadPool.submit(std::move(mis)); },
            updatedFilesCount
        ));
    }

    ~file_scan_service() = default;
};

// 扫描数据库并与硬盘文件进行比对
class db_scan_service
{

};