#pragma once

#define WIN32_LEAN_AND_MEAN             // 从 Windows 头文件中排除极少使用的内容


// 常用类型定义
#define static_string static constexpr std::string_view
#define static_uint static constexpr unsigned int




// 自定义配置内容

// 库默认输出流
// 例如用于日志输出、错误信息输出
#define lib_default_print(str) \
	HYDRA15::Union::secretary::PrintCenter::get_instance() << str

// 启用栈跟踪支持
// 启用可能会影响安全性和性能
#define LIB_IEXPT_STACKTRACE_ENABLE 

// 版本号
namespace HYDRA15::Union::framewotk
{
    static_string libName = "HYDRA15.Union";
    static_string version = "build - 0.0.1 - 20250927 - lib";
}