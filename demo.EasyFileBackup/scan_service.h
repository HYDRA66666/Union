#pragma once
#include "pch.h"
#include "resources.h"

#include "database_service.h"

#include "Union/ThreadLake.h"
#include "Union/shared_containers.h"
#include "Union/iMutexies.h"
#include "Union/PrintCenter.h"
#include "Union/logger.h"
#include "Union/background.h"
using namespace HYDRA15::Union;

struct scan_count { size_t updated; size_t scanned; };

// 扫描硬盘文件并与数据库进行比对
class file_scan_service
{
private:
    class scan_mission : public labourer::mission_base
    {
    private:
        const std::filesystem::path path;

        std::function<void(labourer::mission&&)> submit;
        std::function<void(bool)> add_count;

        secretary::logger lgr{ "file scan service thread" };
        secretary::PrintCenter& pc{ secretary::PrintCenter::get_instance() };
        database_service& db{ database_service::get_instance() };

    public:
        scan_mission(
            const std::filesystem::path& p,
            const std::function<void(labourer::mission)>& s,
            const std::function<void(bool)>& a
        ) :path(p), submit(s), add_count(a) {}

        virtual void operator()() noexcept override
        {
            // 如果 path 指向的是目录，则递归扫描子文件和子目录，否则获取文件信息并与数据库进行比对
            try
            {
                if (std::filesystem::is_directory(path))
                    for (const auto& entry : std::filesystem::directory_iterator(path))
                    {
                        submit(std::make_unique<scan_mission>(entry.path(), submit, add_count));
                        lgr.debug("Dispatched scan task for path: {}", entry.path().string());
                    }
                else if (std::filesystem::is_regular_file(path))
                {
                    archivist::INT updateTime = get_date_time();
                    archivist::INT size = static_cast<archivist::INT>(std::filesystem::file_size(path));
                    // 获取文件的 last_write_time 并转换为 time_t，然后传入 get_date_time
                    auto ftime = std::filesystem::last_write_time(path);
                    auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                        ftime - decltype(ftime)::clock::now() + std::chrono::system_clock::now());
                    std::time_t mod_time = std::chrono::system_clock::to_time_t(sctp);
                    archivist::INT dateTime = get_date_time(mod_time);


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
                        newEntry->set(dbFields[5], static_cast<archivist::INT>(file_states::updated)); // 初始状态为未备份
                        add_count(true);
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
                            entry->set(dbFields[5], static_cast<archivist::INT>(file_states::updated));
                            lgr.debug("File {} updated", path.string());
                        }

                        entry->set(dbFields[1], updateTime);
                        add_count(modified);
                    }
                }
            }
            catch (const std::exception& e) { lgr.error("Error on scanning path {}: {}", path.string(), e.what()); }
            catch (...) { lgr.error("Unknown error on scanning path {}", path.string()); }
        }
    };

    using scan_thread_pool = labourer::thread_pool<labourer::basic_blockable_queue<labourer::mission, labourer::atomic_mutex>>;

private:
    scan_thread_pool threadPool{ std::thread::hardware_concurrency() > 4 ? std::thread::hardware_concurrency() / 4 : 1 };
    std::atomic<size_t> updatedFilesCount{ 0 };
    std::atomic<size_t> scannedFilesCount{ 0 };

    secretary::PrintCenter& pc{ secretary::PrintCenter::get_instance() };

public:
    scan_count count() const { return { .updated = updatedFilesCount.load(std::memory_order::relaxed),.scanned = scannedFilesCount.load(std::memory_order::relaxed) }; }
    
public:
    file_scan_service(const std::filesystem::path& path)
    {
        threadPool.submit(std::make_unique<scan_mission>(
            path,
            [this](labourer::mission&& mis) {threadPool.submit(std::move(mis)); },
            [this](bool updated) {scannedFilesCount.fetch_add(1, std::memory_order::relaxed); if (updated)updatedFilesCount.fetch_add(1, std::memory_order::relaxed); }
        ));
    }

    ~file_scan_service() = default;
};

// 扫描数据库并与硬盘文件进行比对
class record_scan_service : protected labourer::background
{
private:
    secretary::PrintCenter& pc{ secretary::PrintCenter::get_instance() };
    database_service& db{ database_service::get_instance() };

    std::atomic<size_t> updatedFilesCount{ 0 };
    std::atomic<size_t> scannedFilesCount{ 0 };

    std::atomic<archivist::ID> nextScanID{ 0 };
    const archivist::ID totalRecords{ db.size() };

private:
    virtual void work() noexcept override
    {
        secretary::logger lgr{ "DB scan service thread" };
        while (true)
        {
            archivist::ID id = nextScanID.fetch_add(1, std::memory_order::relaxed);
            if (id >= totalRecords)return;
            try
            {
                auto entry = db.at(id);
                if (!entry->valid())continue;
                std::filesystem::path filepath = archivist::extract_string(entry->at(dbFields[2]));
                if (!std::filesystem::exists(filepath) || !std::filesystem::is_regular_file(filepath))
                {
                    // 文件不存在，标记为已删除
                    std::unique_lock rul{ *entry };
                    entry->set(dbFields[5], static_cast<archivist::INT>(file_states::deleted));
                    updatedFilesCount.fetch_add(1, std::memory_order::relaxed);
                    lgr.debug("File {} marked as deleted", filepath.string());
                }
                scannedFilesCount.fetch_add(1, std::memory_order::relaxed);
            }
            catch (const std::exception& e) { lgr.error("Error on scanning record {}: {}", id, e.what()); }
            catch (...) { lgr.error("Unknown error on scanning record {}", id); }

        }
    }

public:
    scan_count count() const { return { .updated = updatedFilesCount.load(std::memory_order::relaxed),.scanned = scannedFilesCount.load(std::memory_order::relaxed) }; }

public:
    record_scan_service() :background(std::thread::hardware_concurrency() > 4 ? std::thread::hardware_concurrency() / 4 : 1) { start(); }

    ~record_scan_service() { wait_for_end(); }
};