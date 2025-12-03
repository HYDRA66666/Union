#include "pch.h"


#include <iostream>
#include <filesystem>
#include <vector>
#include <string>
#include <iomanip>
#include <chrono>
#include <queue>

#include "Union\simple_memory_table.h"
#include "Union\single_loader.h"

using namespace HYDRA15::Union::archivist;
namespace fs = std::filesystem;

// 将 filesystem::file_time_type 转为 YYYYMMDDhhmmss 格式的整数 (例如 20251201100000)
static INT filetime_to_yyyymmddhhmmss(const fs::file_time_type& ftime)
{
    // 可移植转换：把 file_time 转换为 system_clock::time_point
    using file_clock = fs::file_time_type::clock;
    auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
        ftime - file_clock::now() + std::chrono::system_clock::now());
    std::time_t t = std::chrono::system_clock::to_time_t(sctp);
    std::tm tm;
#if defined(_WIN32)
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    // 组合为 YYYYMMDDhhmmss
    INT val = static_cast<INT>(
        (tm.tm_year + 1900) * 10000000000LL +
        (tm.tm_mon + 1) * 100000000LL +
        tm.tm_mday * 1000000LL +
        tm.tm_hour * 10000LL +
        tm.tm_min * 100LL +
        tm.tm_sec);
    return val;
}

int main()
{

    // 数据库文件名（保存在工作目录）
    fs::path dbPath = fs::current_path() / "file_index.arch";

    // 1) 扫描工作目录（含子目录）收集文件名与修改时间
    std::vector<std::pair<std::string, INT>> files;
    for (auto it = fs::recursive_directory_iterator(fs::current_path()); it != fs::recursive_directory_iterator(); ++it)
    {
        try
        {
            if (fs::is_regular_file(it->path()))
            {
                std::string name = it->path().string();
                auto mtime = filetime_to_yyyymmddhhmmss(fs::last_write_time(it->path()));
                files.emplace_back(std::move(name), mtime);
            }
        }
        catch (const std::exception&)
        {
            // 忽略单个文件可能的访问错误
        }
    }

    // 2) 创建表：系统字段 + 两个字段：filename (BYTES/string), mtime (INT)
    field_specs ftab;
    ftab.push_back(simple_memory_table::sysfldRowMark);
    ftab.push_back(field_spec{ "filename", "file path", field_spec::field_type::BYTES });
    ftab.push_back(field_spec{ "mtime", "modification time YYYYMMDDhhmmss", field_spec::field_type::INT });

    // 若已存在同名文件，删除以重新创建
    if (fs::exists(dbPath)) fs::remove(dbPath);

    // 创建 single_loader + simple_memory_table 并写入数据
    {
        auto loader = single_loader::make_unique(dbPath, ftab, simple_memory_table::version, sfstream::segment_size_level::I);
        simple_memory_table tbl(std::move(loader));

        for (const auto& [name, mtime] : files)
        {
            auto ent = tbl.create();
            std::unique_lock lock{ *ent };

            // filename -> BYTES 类型（使用 ASCII/UTF-8 字节）
            BYTES b(name.begin(), name.end());
            ent->set("filename", field{ b });

            // mtime -> INT
            ent->set("mtime", field{ mtime });
        }

        // 1. 在创建表时建立文件路径的索引（测试要求）
        tbl.create_index("idx_filename", field_specs{ field_spec{ "filename", "", field_spec::field_type::BYTES } });

        // 表的析构会执行 flush_all 并落盘，此处离开作用域会保存数据
    }

    // 3) 重新从磁盘加载该表并打印所有记录
    {
        auto loader2 = single_loader::make_unique(dbPath);
        simple_memory_table tbl2(std::move(loader2));

        std::cout << "Loaded records: " << tbl2.size() << "\n";
        for (ID id = 0; id < tbl2.size(); ++id)
        {
            auto ent = tbl2.at(id);
            std::unique_lock lock{ *ent };

            // 读取 filename
            const field& f_name = ent->at("filename");
            std::string name;
            if (auto p = std::get_if<BYTES>(&f_name))
                name.assign(p->begin(), p->end());
            else
                name = "<non-bytes>";

            // 读取 mtime
            const field& f_mtime = ent->at("mtime");
            INT mtime = 0;
            if (auto p = std::get_if<INT>(&f_mtime))
                mtime = *p;

            // 打印
            std::cout << std::setw(6) << id << "  " << mtime << "  " << name << "\n";
        }

        // 4) 从已加载的表任选一条记录，使用 excute 接口查询此文件路径，并打印查询结果
        if (!files.empty())
        {
            // 选取中间一条（或第 0 条）
            size_t pickIdx = files.size() / 2;
            const std::string& targetPath = files[pickIdx].first;

            std::cout << "index query for file path: " << targetPath << "\n";

            // 构造搜索条件： filename == targetPath
            BYTES keyBytes(targetPath.begin(), targetPath.end());
            incident::condition_param cond;
            cond.targetField = "filename";
            cond.type = incident::condition_param::condition_type::equal;
            cond.reference = field{ keyBytes };

            incident ic;
            ic.type = incident::incident_type::search;
            ic.param = cond;

            std::queue<incident> opers;
            opers.push(ic);

            // 执行查询
            std::list<ID> res = tbl2.excute(opers);
            std::cout << "\nQuery for \"" << targetPath << "\" returned " << res.size() << " record(s):\n";
            for (ID rid : res)
            {
                auto ent = tbl2.at(rid);
                std::unique_lock lock{ *ent };
                const field& fn = ent->at("filename");
                std::string name;
                if (auto p = std::get_if<BYTES>(&fn)) name.assign(p->begin(), p->end());
                const field& mt = ent->at("mtime");
                INT mtime = 0;
                if (auto p = std::get_if<INT>(&mt)) mtime = *p;
                std::cout << std::setw(6) << rid << "  " << mtime << "  " << name << "\n";
            }
        }
    }

    return 0;

}