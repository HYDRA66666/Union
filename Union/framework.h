#pragma once

#define WIN32_LEAN_AND_MEAN             // 从 Windows 头文件中排除极少使用的内容


// 常用类型定义
#include "astring.h"
#define static_string static constexpr HYDRA15::Union::framework::astring
#define static_uint static constexpr unsigned int




// 自定义配置内容

// 库默认输出流
// 例如用于日志输出、错误信息输出
#define union_default_print(str) \
	HYDRA15::Union::secretary::PrintCenter::get_instance() << str

// 启用栈跟踪支持
// 启用可能会影响安全性和性能
#define UNION_IEXPT_STACKTRACE_ENABLE 

// 默认线程池线程数目
#define UNION_DEFAULT_THREAD_COUNT std::thread::hardware_concurrency() / 2
