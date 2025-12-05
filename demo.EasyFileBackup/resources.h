#pragma once
#include "pch.h"

#include "Union/archivist_interfaces.h"
#include "Union/simple_memory_table.h"

using namespace HYDRA15::Union;

// 常量定义
inline const archivist::field_specs dbFields{
    archivist::simple_memory_table::sysfldRowMark, // 系统行标记字段
    {.name = "UPDATE_TIME", .comment = "last record update time", .type = archivist::field_spec::field_type::INT},
    {.name = "FILE_PATH", .comment = "file relative path to database file", .type = archivist::field_spec::field_type::BYTES},
    {.name = "DATETIME", .comment = "file last modified time", .type = archivist::field_spec::field_type::INT},
    {.name = "SIZE", .comment = "file size in bytes", .type = archivist::field_spec::field_type::INT},
    {.name = "STATUS", .comment = "file backup status", .type = archivist::field_spec::field_type::INT}
};

constexpr std::string_view help_str{
    "用法：\n"
    "    启动参数(dbPath)：数据库文件的路径。数据库的父目录将会被视为源目录。\n"
    "    命令 set_all [normal, updated]：将所有文件状态设置为指定状态\n"
    "    命令 sync (destPath) [normal, deleted, updated, del_upd]：将源目录中的文件同步到目标目录，可选选项指定要同步的文件状态\n"
    "    命令 get [deleted, updated]：获取数据库中所有指定状态的文件的列表\n"
    "    命令 get [path]：获取指定路径的文件信息\n"
    "    命令 help：显示帮助信息\n"
    "以上命令加上 \"-\" 便可作为启动参数"
};


// 类型定义
enum class file_states : archivist::INT { normal = 0, deleted = 0x1, updated = 0x2 };   // 已备份 已删除 已更新/新建


// 全局变量


// 全局工具
inline int64_t get_date_time(time_t stamp)
{
    std::tm tm{};
    // 使用 Visual Studio 的线程安全函数 localtime_s
    if (localtime_s(&tm, &stamp) != 0) {
        return 0;
    }

    int64_t yy = (tm.tm_year + 1900) % 100;
    int64_t mm = tm.tm_mon + 1;
    int64_t dd = tm.tm_mday;
    int64_t hh = tm.tm_hour;
    int64_t min = tm.tm_min;
    int64_t ss = tm.tm_sec;

    // 组成格式 YYMMDDhhmmss -> 返回 int64_t
    return yy * 10000000000LL
        + mm * 100000000LL
        + dd * 1000000LL
        + hh * 10000LL
        + min * 100LL
        + ss;
}

inline int64_t get_date_time() { return get_date_time(std::time(nullptr)); }

inline std::string to_string(const std::u8string& str)
{
    std::string res;
    res.resize(str.size());
    assistant::memcpy(reinterpret_cast<const char*>(str.data()), reinterpret_cast<char*>(res.data()), str.size());
    return res;
}