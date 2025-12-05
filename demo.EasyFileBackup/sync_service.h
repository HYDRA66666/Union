#pragma once
#include "pch.h"
#include "resources.h"

#include "Union/archivist_interfaces.h"
#include "Union/iMutexies.h"
#include "Union/background.h"
#include "database_service.h"
#include "Union/logger.h"

struct sync_count { size_t synced; size_t total; };

// 负责将所有指定类型的文件复制到指令目录
class sync_service : labourer::background
{
    const std::list<archivist::ID> targets;
    const std::filesystem::path path;

    labourer::atomic_mutex mtx;
    std::list<archivist::ID>::const_iterator tarit;

    std::atomic<size_t> parsed{ 0 };

private:
    static std::list<archivist::ID> get_targets(file_states state)
    {
        archivist::incident icdt{
            .type = archivist::incident::incident_type::search,
            .param = archivist::incident::condition_param{
                .targetField = dbFields[5],
                .type = archivist::incident::condition_param::condition_type::equal,
                .reference = static_cast<archivist::INT>(state)
            }
        };

        return database_service::get_instance().excute({ icdt });
    }

    virtual void work() noexcept override
    {
        std::list<archivist::ID>::const_iterator it;
        database_service& db{ database_service::get_instance() };
        secretary::logger lgr{ "sync service thread" };
        while(true)
        {
            {
                std::unique_lock ul{ mtx };
                it = tarit;
                if (it == targets.end())return;
                tarit++;
            }
            try
            {
                auto pety = db.at(*it);
                std::shared_lock rsl{ *pety };

                std::filesystem::path srcPath = archivist::extract_string<char8_t>(pety->at(dbFields[2]));
                std::filesystem::path destPath = path / std::filesystem::relative(srcPath);
                file_states state = static_cast<file_states>(std::get<archivist::INT>(pety->at(dbFields[5])));

                if (!std::filesystem::exists(destPath.parent_path()))
                    std::filesystem::create_directories(destPath.parent_path());

                std::error_code ec;
                switch (state)
                {
                case file_states::updated:
                    std::filesystem::copy_file(srcPath, destPath, std::filesystem::copy_options::overwrite_existing, ec);
                    break;
                case file_states::deleted:
                    std::filesystem::remove(destPath, ec);
                    break;
                default:
                    break;
                }
                if (ec)
                    lgr.error("failed to copy file {}: {}", to_string(srcPath.u8string()), ec.message());
                else pety->set(dbFields[5], static_cast<archivist::INT>(file_states::normal));

            }
            catch (const std::exception& e) { lgr.error("failed to parse record {}: ", *it, e.what()); }
            parsed.fetch_add(1, std::memory_order::relaxed);
        }
    }

public:
    sync_count count() const { return { parsed.load(std::memory_order::relaxed), targets.size() }; }

public:
    sync_service(const std::filesystem::path& targetPath, file_states targetState)
        :path(targetPath), targets(get_targets(targetState)), background(std::thread::hardware_concurrency() / 2)
    {
        tarit = targets.begin();
        start();
    }
    ~sync_service() { wait_for_end(); }
};