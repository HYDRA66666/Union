#pragma once
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
// debug 模式下默认启用，使用 UNION_IEXPT_STACKTRACE_DISABLE 宏以禁用
// release 模式下默认不启用，使用 UNION_IEXPT_STACKTRACE_ENABLE 宏以启用
#ifdef _DEBUG
#ifndef UNION_IEXPT_STACKTRACE_DISABLE
#define UNION_IEXPT_STACKTRACE_ENABLE
#endif // !UNION_IEXPT_STACKTRACE_DISABLE
#endif // _DEBUG


// 默认线程池线程数目
#define UNION_DEFAULT_THREAD_COUNT std::thread::hardware_concurrency() / 2

namespace HYDRA15::Union
{
    // 类型定义
    using byte = std::byte;   // 字节类型

    // 全局 debug 变量
#ifdef _DEBUG
    inline bool debug = true;
#else 
    inline bool debug = false;
#endif

#ifdef _DEBUG
    static constexpr bool globalDebug = true;
#else
    static constexpr bool globalDebug = false;
#endif // _DEBUG

    
}
