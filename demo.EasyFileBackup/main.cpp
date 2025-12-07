/*************************************************************************************
* 此项目使用数据库记录文件信息和状态，以实现文件备份和自动更新备份的功能
* 在源目录运行此程序，此程序将在启动时自动扫描当前目录及其子目录中的文件，
*   比较文件是否有更新、删除，并按照指令完成向目标目录的备份工作
* 软件必须在需要备份的目录的下运行，同一个目录可以创建多个数据库文件
* 文件被分为三种状态：
*   normal (正常)：文件未被修改，已备份且未更新
*   updated (已更新)：文件已被修改，需重新备份
*   deleted (已删除)：文件已被删除，等待删除备份文件
* 
* 用法：
*   启动参数 (dbPath)：数据库文件的路径。数据库的父目录将会被视为源目录。
*   命令 set [normal, updated]：将所有文件状态设置为指定状态
*   命令 sync (destPath) [normal, deleted, updated, del_upd]：将源目录中的文件同步到目标目录，
*       可选选项指定要同步的文件状态
*   命令 get [normal, deleted, updated]：获取数据库中所有指定状态的文件的列表
*   命令 get [path]：获取指定路径的文件信息
*   命令 help：显示帮助信息
*   命令 quit：退出程序
* 以上命令加上 "-" 便可作为启动参数
* *************************************************************************************/


#include "pch.h"

#include "Union/PrintCenter.h"
#include "Union/lib_exceptions.h"
#include "Union/utilities.h"
#include "Union/logger.h"
#include "Union/string_utilities.h"

#include "database_service.h"
#include "scan_service.h"
#include "sync_service.h"

using namespace HYDRA15::Union;


scan_count scan_file(const std::filesystem::path& path)
{
    file_scan_service fss{ path };
    secretary::PrintCenter& pc{ secretary::PrintCenter::get_instance() };
    static constexpr std::string_view btmFmt{ "scanned {} files, {} updated" };
    auto btmID = pc.set(std::format(btmFmt, 0, 0));

    scan_count lastCnt = fss.count();
    int i = 0;
    while (true)
    {
        scan_count nowCnt = fss.count();
        pc.update(btmID, std::format(btmFmt, nowCnt.scanned, nowCnt.updated));
        if (lastCnt.scanned == nowCnt.scanned) i++;
        else i = 0;
        if (i > 4) break;
        lastCnt = nowCnt;
        std::this_thread::sleep_for(std::chrono::milliseconds(250));
    }

    pc.remove(btmID);
    return lastCnt;
}

scan_count scan_record()
{
    record_scan_service rss;
    secretary::PrintCenter& pc{ secretary::PrintCenter::get_instance() };
    static constexpr std::string_view btmFmt{ "scanned {} records, {} updated" };
    auto btmID = pc.set(std::format(btmFmt, 0, 0));

    scan_count lastCnt = rss.count();
    int i = 0;
    while (true)
    {
        scan_count nowCnt = rss.count();
        pc.update(btmID, std::format(btmFmt, nowCnt.scanned, nowCnt.updated));
        if (lastCnt.scanned == nowCnt.scanned) i++;
        else i = 0;
        if (i > 4) break;
        lastCnt = nowCnt;
        std::this_thread::sleep_for(std::chrono::milliseconds(250));
    }

    pc.remove(btmID);
    return lastCnt;

}


void set_handler(std::list<std::string>& params)
{
    if (params.empty())
        throw exceptions::common::BadParameter("set command", "empty", "normal or updated");

    std::string param = params.front();

    std::optional<archivist::INT> state;

    if (param == "normal")
        state = static_cast<archivist::INT>(file_states::normal);

    if (param == "updated")
        state = static_cast<archivist::INT>(file_states::normal);

    if (state)
    {
        for (auto& ety : database_service::get_instance())
        {
            std::unique_lock rul{ ety };
            ety.set(dbFields[5], state.value());
        }
        params.pop_front();
        database_service::get_instance().flush();
        secretary::PrintCenter::println("All file states set to {}", param);
        return;
    }
    else
        throw exceptions::common::BadParameter("set command", assistant::container_to_string(params), "normal or updated");

}

void sync_handler(std::list<std::string>& params)
{
    if (params.empty())
        throw exceptions::common::BadParameter("set command", "empty", "normal, deleted, updated or destination path");

    std::string param = params.front();
    params.pop_front();

    if (!std::filesystem::exists(param) || !std::filesystem::is_directory(param))
        throw exceptions::common::BadParameter("sync command", param, "an existing destination path");

    bool syncDeled = false;
    bool syncUpdated = false;
    bool syncNormal = false;

    if (params.empty())
    {
        syncDeled = true;
        syncUpdated = true;
    }
    else while (!params.empty())
    {
        std::string subparam = params.front();
        if (subparam == "normal")
            syncNormal = true;
        else if (subparam == "deleted")
            syncDeled = true;
        else if (subparam == "updated")
            syncUpdated = true;
        else if (subparam == "del_upd")
        {
            syncDeled = true;
            syncUpdated = true;
        }
        else break;
    }

    auto doWork = [param](file_states state) {
        sync_service ss{ param, state };
        secretary::PrintCenter& pc{ secretary::PrintCenter::get_instance() };

        sync_count lastCnt = ss.count();
        int i = 0;
        auto btmID = pc.set("start synchronization");
        while (true)
        {
            sync_count nowCnt = ss.count();
            pc.update(btmID, std::format("synchronized {}/{} files of state 0x{:04X}", nowCnt.synced, nowCnt.total, static_cast<archivist::INT>(state)));
            if (lastCnt.synced == nowCnt.synced) i++;
            else i = 0;
            if (i > 4) break;
            lastCnt = nowCnt;
            std::this_thread::sleep_for(std::chrono::milliseconds(250));
        }
        pc.remove(btmID);
        return;
        };

    if (syncNormal)doWork(file_states::normal);
    if (syncDeled) doWork(file_states::deleted);
    if (syncUpdated)doWork(file_states::updated);
}

void get_handler(std::list<std::string>& params)
{
    if (params.empty())
        throw exceptions::common::BadParameter("set command", "empty", "normal, deleted, updated or file path");

    std::string param = params.front();
    params.pop_front();

    secretary::PrintCenter& pc{ secretary::PrintCenter::get_instance() };
    secretary::logger lgr = UNION_CREATE_LOGGER();

    database_service& db{ database_service::get_instance() };

    std::list<archivist::ID> srhRes;
    {
        archivist::incident srhIcdt{ .type = archivist::incident::incident_type::search };
        if (param == "normal")
            srhIcdt.param = archivist::incident::condition_param{
                .targetField = dbFields[5],
                .type = archivist::incident::condition_param::condition_type::equal,
                .reference = static_cast<archivist::INT>(file_states::normal)
        };
        else if (param == "deleted")
            srhIcdt.param = archivist::incident::condition_param{
                .targetField = dbFields[5],
                .type = archivist::incident::condition_param::condition_type::equal,
                .reference = static_cast<archivist::INT>(file_states::deleted)
        };
        else if (param == "updated")
            srhIcdt.param = archivist::incident::condition_param{
                .targetField = dbFields[5],
                .type = archivist::incident::condition_param::condition_type::equal,
                .reference = static_cast<archivist::INT>(file_states::updated)
        };
        else
            srhIcdt.param = archivist::incident::condition_param{
                .targetField = dbFields[2],
                .type = archivist::incident::condition_param::condition_type::equal,
                .reference = archivist::create_string_field(std::filesystem::absolute(param).u8string())
            };

        srhRes = database_service::get_instance().excute({ srhIcdt });
    }

    lgr.info("query for {} obtained {} results", param, srhRes.size());


    pc.rolling("|        |  state | last modified time | file path");
    for (const auto& id : srhRes)
    {
        try
        {
            auto pety = db.at(id);
            std::shared_lock rsl{ *pety };
            archivist::INT state = std::get<archivist::INT>(pety->at(dbFields[5]));
            archivist::INT datetime = std::get<archivist::INT>(pety->at(dbFields[3]));
            std::string filepath = std::filesystem::path(archivist::extract_string<char8_t>(pety->at(dbFields[2]))).string();

            pc.rolling(std::format("| {:6} | 0x{:04X} | {} | {}", id, state, datetime, filepath));
        }
        catch (const std::exception& e) { lgr.error("error while parsing record {} : {} ", id, e.what()); }
    }
    pc.flush();
}

void help_handler() { secretary::PrintCenter::println(help_str); }


class initializer
{
    secretary::logger lgr{ "initilizer" };
public:
    initializer(const std::list<std::string>& params)
    {
        referee::iExceptionBase::enableDebug = true;
        secretary::PrintCenter::enableAnsi = false;

        secretary::log::print = [](const std::string& msg) {static secretary::PrintCenter& pc = secretary::PrintCenter::get_instance(); pc.rolling(msg); };

        if (params.empty() || params.front().front() == '-')
        {
            help_handler();
            throw exceptions::common::BadParameter("init params", assistant::container_to_string(params), "see help infomation");
        }

        if constexpr (globalDebug)
            lgr.debug("init with params: {}", assistant::container_to_string(params));

        database_service::init(params.front());

    }

    ~initializer()
    {
        lgr.info("finalizing database...");
    }
};

int main(int argc, char* argv[])
{
    secretary::logger lgr = UNION_CREATE_LOGGER();
    secretary::PrintCenter& pc{ secretary::PrintCenter::get_instance() };

    try
    {
        std::list<std::string> params;
        for (int i = 1; i < argc; i++)
            params.emplace_back(argv[i]);

        pc << lgr.info("initializing and loading databass...");

        initializer init{ params };
        database_service& db{ database_service::get_instance() };

        std::filesystem::path basePath = std::filesystem::current_path();
        params.pop_front();

        {   // 启动先执行扫描任务
            lgr.info("starting scan tasks...");

            auto srfut = std::async(scan_record);
            auto sffut = std::async(scan_file, basePath);
            auto srres = srfut.get();
            auto sfres = sffut.get();

            lgr.info("record scan completed, scanned {}, updated {}", srres.scanned, srres.updated);
            lgr.info("file scan completed, scanned {}, updated {}", sfres.scanned, sfres.updated);
            lgr.info("saving database");
            db.flush();
            lgr.info("database saved");
        }

        // 然后逐条执行命令
        bool exit = false;
        while (!params.empty())
        {
            std::string cmd = params.front();
            params.pop_front();

            if (cmd.front() != '-')
            {
                lgr.error("bad command: {}", cmd);
                continue;
            }

            cmd = cmd.substr(1);
            try
            {
                if (cmd == "set")
                    set_handler(params);
                if (cmd == "sync")
                    sync_handler(params);
                if (cmd == "get")
                    get_handler(params);
                if (cmd == "help")
                    help_handler();
                if (cmd == "quit")
                {
                    exit = true;
                    break;
                }
            }
            catch (const std::exception& e) { lgr.error("exception during handle command {}: {}", cmd, e.what()); }
        }

        if (exit)return 0;

        // 进入交互模式
        pc.set_stick_btm(" > ");
        while (true)
        {
            {
                std::string cmdline;
                if (!std::getline(std::cin, cmdline))
                    break;
                params = assistant::split_by(cmdline, " ");
            }

            std::string cmd = params.front();
            params.pop_front();
            try
            {
                if (cmd == "set")
                    set_handler(params);
                if (cmd == "sync")
                    sync_handler(params);
                if (cmd == "get")
                    get_handler(params);
                if (cmd == "help")
                    help_handler();
                if (cmd == "quit")
                    return 0;
            }
            catch (const std::exception& e) { lgr.error("exception during handle command {}: {}", cmd, e.what()); }
            
            
        }
    }
    catch (const std::exception& e) { lgr.fatal("fatal exception: {}", e.what()); return -1; }
}
