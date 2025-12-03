#pragma once



/*************** 合并自 astring.h ***************/
// #pragma once
#include <string>

namespace HYDRA15::Union::framework
{
    // 仅接受 ascii 字符的字符串
    // 继承自 string_view ，仅可在编译器初始化，不可修改
    class astring : public std::string_view
    {
    public:
        constexpr astring() = default;
        constexpr astring(const astring&) = default;
        constexpr astring(const std::string_view& s) :std::string_view(s) { verify(); }
        constexpr astring(const char* p, size_t s) : std::string_view(p, s) { verify(); }
        constexpr astring(const char* p) : std::string_view(p) { verify(); }

        template<size_t N>
        constexpr astring(const char(&a)[N])
            :std::string_view(a, N)
        {
            verify();
        }


        constexpr ~astring() = default;

        constexpr static bool is_ascii(const char& c){ return (c >= 0 && c <= 127) ? true : false; }
        constexpr void verify()
        {
            for (const auto& c : *this)
                if (!is_ascii(c))
                    throw std::exception("astring can only handle ascii characters");
        }
    };
}




/*************** 合并自 pch.h ***************/
// pch.h: 这是预编译标头文件。
// 下方列出的文件仅编译一次，提高了将来生成的生成性能。
// 这还将影响 IntelliSense 性能，包括代码完成和许多代码浏览功能。
// 但是，如果此处列出的文件中的任何一个在生成之间有更新，它们全部都将被重新编译。
// 请勿在此处添加要频繁更新的文件，这将使得性能优势无效。


#define WIN32_LEAN_AND_MEAN             // 从 Windows 头文件中排除极少使用的内容


// #pragma once
// 添加要在此处预编译的标头
// 
// 标准库
#include <iostream>
#include <string>
#include <exception>
#include <format>
#include <functional>
#include <unordered_map>
#include <mutex>
#include <stdlib.h>
#include <memory>
#include <shared_mutex>
#include <thread>
#include <stacktrace>
#include <barrier>
#include <queue>
#include <future>
#include <any>
#include <deque>
#include <initializer_list>
#include <variant>
#include <ctime>
#include <fstream>
#include <chrono>
#include <condition_variable>
#include <sstream>
#include <atomic>
#include <array>
#include <coroutine>
#include <generator>
#include <map>
#include <expected>
#include <filesystem>
#include <bit>
#include <numeric>
#include <unordered_set>
#include <cstdio>
#include <set>
#include <algorithm>







/*************** 合并自 framework.h ***************/
// #pragma once
// 常用类型定义
// #include "astring.h"
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
    using byte = uint8_t;   // 字节类型

    // 全局 debug 变量
#ifdef _DEBUG
    inline bool debug = true;
#else 
    inline bool debug = false;
#endif

    
}




/*************** 合并自 byteswap.h ***************/
// #pragma once
// #include "pch.h"
// #include "framework.h"

namespace HYDRA15::Union::assistant::byteswap
{
    // 字节序转换
    template<typename I>
        requires std::is_integral_v<I>
    I to_little_endian(I i)
    {
        if constexpr (std::endian::native == std::endian::little)
            return i;
        else if constexpr (std::endian::native == std::endian::big)
            return std::byteswap(i);
        else throw exceptions::common("Local byte order uncertain");
    }

    template<typename I>
        requires std::is_integral_v<I>
    I to_big_endian(I i)
    {
        if constexpr (std::endian::native == std::endian::little)
            return std::byteswap(i);
        else if constexpr (std::endian::native == std::endian::big)
            return i;
        else throw exceptions::common("Local byte order uncertain");
    }

    template<typename I>
        requires std::is_integral_v<I>
    I from_little_endian(I i)
    {
        if constexpr (std::endian::native == std::endian::little)
            return i;
        else if constexpr (std::endian::native == std::endian::big)
            return std::byteswap(i);
        else throw exceptions::common("Local byte order uncertain");
    }

    template<typename I>
        requires std::is_integral_v<I>
    I from_big_endian(I i)
    {
        if constexpr (std::endian::native == std::endian::little)
            return std::byteswap(i);
        else if constexpr (std::endian::native == std::endian::big)
            return i;
        else throw exceptions::common("Local byte order uncertain");
    }



    // 整组字节转换，直接在传入的数据中原地转换
    template<typename T>
        requires std::is_integral_v<T>
    void to_little_endian_vector(std::vector<T>& data)
    {
        for (auto& i : data)
            i = to_little_endian<T>(i);
    }

    template<typename T>
        requires std::is_integral_v<T>
    void to_big_endian_vector(std::vector<T>& data)
    {
        for (auto& i : data)
            i = to_big_endian<T>(i);
    }

    template<typename T>
        requires std::is_integral_v<T>
    void from_little_endian_vector(std::vector<T>& data)
    {
        for (auto& i : data)
            i = from_little_endian<T>(i);
    }

    template<typename T>
        requires std::is_integral_v<T>
    void from_big_endian_vector(std::vector<T>& data)
    {
        for (auto& i : data)
            i = from_big_endian<T>(i);
    }

    // Range 版本：支持任意可迭代容器（传入可写容器引用）
    // ** AI 生成的代码，未经广泛测试
    template<typename R>
        requires std::is_integral_v<std::remove_reference_t<decltype(*std::begin(std::declval<R&>()))>>
    void to_little_endian_range(R& data)
    {
        for (auto& i : data)
            i = to_little_endian(std::remove_reference_t<decltype(i)>(i));
    }

    template<typename R>
        requires std::is_integral_v<std::remove_reference_t<decltype(*std::begin(std::declval<R&>()))>>
    void to_big_endian_range(R& data)
    {
        for (auto& i : data)
            i = to_big_endian(std::remove_reference_t<decltype(i)>(i));
    }

    template<typename R>
        requires std::is_integral_v<std::remove_reference_t<decltype(*std::begin(std::declval<R&>()))>>
    void from_little_endian_range(R& data)
    {
        for (auto& i : data)
            i = from_little_endian(std::remove_reference_t<decltype(i)>(i));
    }

    template<typename R>
        requires std::is_integral_v<std::remove_reference_t<decltype(*std::begin(std::declval<R&>()))>>
    void from_big_endian_range(R& data)
    {
        for (auto& i : data)
            i = from_big_endian(std::remove_reference_t<decltype(i)>(i));
    }
}



/*************** 合并自 concepts.h ***************/
// #pragma once
// #include "framework.h"
// #include "pch.h"

namespace HYDRA15::Union::framework
{
    // 信息输出相关概念
    template<typename T>
    concept can_be_transfer_to_string = requires(std::remove_cvref_t<T> a) {
        { std::to_string(a) } -> std::convertible_to<std::string>;
    } || requires(std::remove_cvref_t<T> a) {
        { to_string(a) } -> std::convertible_to<std::string>;
    };

    template<class T>
    concept has_info_interface = requires(std::remove_cvref_t<T> a) {
        { a.info() } -> std::convertible_to<std::string>;
    };

    // 键约束
    template<typename K>
    concept compare_key = requires(K k1, K k2) {
        { k1 < k2 }->std::same_as<bool>;
    };

    template<typename K>
    concept hash_key = requires(K k) {
        { std::hash<K>()(k) } -> std::same_as<size_t>;
    }&& requires(K a, K b) {
        { a == b } -> std::same_as<bool>;
    };

    template<typename K>
    concept index_key = compare_key<K> && hash_key<K>;

    // 锁约束
    template<typename T>
    concept lockable = requires(T t) {
        { t.lock() } -> std::same_as<void>;
        { t.unlock() } -> std::same_as<void>;
    };

    template<typename T>
    concept shared_lockable = lockable<T> && requires(T t) {
        { t.lock_shared() } -> std::same_as<void>;
        { t.unlock_shared() } -> std::same_as<void>;
    };

    // 去除修饰符后真实类型
    template<typename T, typename RT>
    concept is_really_same_v = std::is_same_v<std::remove_cvref_t<T>, std::remove_cvref_t<RT>>;

    template<typename T, typename RT>
    concept is_really_derived_from = std::derived_from<std::remove_cvref_t<T>, std::remove_cvref_t<RT>>;

    // 容器相关概念
    template<typename C>
    concept container = requires(C c) {
        { c.begin() } ;
        { c.end() } ;
        { c.size() } -> std::same_as<size_t>;
    };

    template<typename M>
    concept map_container = container<M> && requires(M m) {
        typename M::key_type;
        typename M::mapped_type;
        { m.contains(typename M::key_type{}) } -> std::same_as<bool>;
        { m.try_emplace(typename M::key_type{}, typename M::mapped_type{}) };
        { m.erase(typename M::key_type{}) } -> std::same_as<size_t>;
        { m.at(typename M::key_type{}) } -> std::same_as<typename M::mapped_type&>;
    };

    // 函数指针类型
    template<typename F>
    concept is_function_pointer_v = std::is_pointer_v<F> && std::is_function_v<std::remove_pointer_t<F>>;
}



/*************** 合并自 iExceptionBase.h ***************/
// #pragma once
// #include "pch.h"
// #include "framework.h"

namespace HYDRA15::Union::referee
{
    class iExceptionBase : public std::exception
    {
    public:
        using expt_code = unsigned int;

    private:    // 数据
        const expt_code exptCode;           // 错误码（必需）
        const std::string desp;             // 错误描述
        std::unordered_map<std::string, std::string> info;  // 错误信息 / 参数
        std::list<std::string> stackTrace;  // 调用栈
    public:     // 数据配置
        bool enableDebug = debug;

    private:    // 工具函数
#ifdef UNION_IEXPT_STACKTRACE_ENABLE
        static std::string format_stacktrace_entry(const std::stacktrace_entry& e)
        {
            if (e.source_file().empty())
                return std::format("at {}", e.description());

            std::filesystem::path path{ e.source_file() };
            auto fileName = path.filename();
            auto filePath = path.parent_path().filename();
            return std::format(
                "at {} in {} ({}),",
                e.description(),
                std::format("{}/{}",filePath.string(), fileName.string()),
                e.source_line()
            );
        }
#endif

    public:     // 接口
        expt_code code() const { return exptCode; }

        const std::string& description() const { return desp; }

        const std::unordered_map<std::string, std::string>& infomation() const { return info; }
        const std::string& get(const std::string& k)const { return info.at(k); }
        iExceptionBase& set(const std::string& k, const std::string& v) { info[k] = v; return *this; }

        const std::list<std::string>& stack_trace() const { return stackTrace; }

    protected:
        mutable std::string whatStr;
    public:     // 系统接口
        virtual const char* what() const noexcept override
        {
            if (desp.empty())
                whatStr = std::format("unknow exception. (0x{:08X})", exptCode);
            else
                whatStr = std::format("{}. (0x{:08X})", desp, exptCode);
            if (enableDebug)
            {
                if (!info.empty())whatStr += " " + detail();
                if (!stackTrace.empty())whatStr += "\n" + trace();
            }
            return whatStr.c_str();
        }

    public:     // 扩展信息接口
        virtual std::string detail() const
        {
            if (info.empty())return {};
            size_t size = 2;
            for (const auto& [k, v] : info)
                size += k.size() + v.size() + 5;
            std::string res{ "[" };
            res.reserve(size);
            for (const auto& [k, v] : info)
                res.append(std::format("{} : {}, ", k, v));
            res.pop_back(); res.pop_back();
            res.append("]");
            return res;
        }

        virtual std::string trace() const
        {
            if (stackTrace.empty())return {};
            size_t size = 13;
            for (const auto& i : stackTrace)
                size += i.size() + 6;
            std::string res{ "Stack trace:\n" };
            res.reserve(size);
            for (const auto& i : stackTrace)
                res.append(i.empty() ? "" : std::format("    {}\n", i));
            res.pop_back(); res.pop_back();
            return res;
        }

    public:     // 构造
        iExceptionBase(expt_code c, const std::string& d = std::string{}) 
            :exptCode(c), desp(d)
        {
#ifdef UNION_IEXPT_STACKTRACE_ENABLE
            auto stks = std::stacktrace::current();
            size_t i = 0;
            for (const auto& e : stks)
                stackTrace.push_back(format_stacktrace_entry(e));
#endif // UNION_IEXPT_STACKTRACE_ENABLE
        }

        virtual ~iExceptionBase() = default;
    };


}



/*************** 合并自 iExceptionManager.h ***************/
// #pragma once
// #include "framework.h"
// #include "pch.h"

namespace HYDRA15::Union::referee
{
    // 提供错误处理相关工具
    class iExceptionManager
    {
        static struct visualize
        {

        }vslz;

        // 程序包装器，简单保护程序不因为异常而崩溃
    public:
        // 简单包装，仅用于保证程序不崩溃
        template<typename F, typename... Args>
        static auto warp(F f, Args... args)
            -> std::expected<std::invoke_result_t<F, Args...>, std::exception_ptr>
        {
            try { return std::invoke(std::forward<F>(f), std::forward<Args>(args)...); }
            catch (...) { return std::unexpected(std::current_exception()); }
        }
        // 带出错回调的包装
        template<typename F, typename... Args>
        static auto warp_c(std::function<void(std::exception_ptr)> callback, F f, Args... args)
            -> std::expected<std::invoke_result_t<F, Args...>, std::exception_ptr>
        {
            try { return std::invoke(std::forward<F>(f), std::forward<Args>(args)...); }
            catch (...) { callback(std::current_exception); return std::unexpected(std::current_exception()); }
        }
        // 带重试的包装
        template<typename F, typename... Args>
        static auto warp_r(unsigned int times, F f, Args... args)
            -> std::expected<std::invoke_result_t<F, Args...>, std::exception_ptr>
        {
            unsigned int i = 0;
            while (true)
            {
                try { return std::invoke(std::forward<F>(f), std::forward<Args>(args)...); }
                catch (...) { if (i < times) { i++; continue; } else return std::unexpected(std::current_exception()); }
            }
        }
        // 带重试和回调的包装
        template<typename F, typename... Args>
        static auto warp_rc(unsigned int times, std::function<void(std::exception_ptr)> callback, F f, Args... args)
            -> std::expected<std::invoke_result_t<F, Args...>, std::exception_ptr>
        {
            unsigned int i = 0;
            while (true)
            {
                try { return std::invoke(std::forward<F>(f), std::forward<Args>(args)...); }
                catch (...) 
                { 
                    if (i < times) { i++; continue; } 
                    else { callback(std::current_exception); return std::unexpected(std::current_exception()); }
                }
            }
        }
    };

}



/*************** 合并自 iMutexies.h ***************/
// #pragma once
// #include "pch.h"
// #include "framework.h"

namespace HYDRA15::Union::labourer
{
    template<size_t retreatFreq = 32>
    class basic_atomic_mutex
    {
    private:
        std::atomic_bool lck = false;

    public:
        void lock()
        {
            while (true)
            {
                for (size_t i = 0; i < retreatFreq; i++)
                    if (try_lock())return;
                std::this_thread::yield();
            }
        }

        void unlock() { lck.store(false, std::memory_order::release); }

        bool try_lock()
        {
            bool expected = false;
            return lck.compare_exchange_strong(expected, true, std::memory_order::acquire);
        }
    };

    using atomic_mutex = basic_atomic_mutex<>;


    // 使用原子变量实现的读写锁，自旋等待，适用于短临界区或读多写少
    // 支持锁升级，此特性对于一般读写锁操作几乎没有额外开销
    // 限制总读锁数量为 0xFFFFFFFF
    // 
    // 性能测试：
    //                           debug       release
    //  无锁（纯原子计数器）  4450w tps     6360w tps
    //  std::shared_mutex      480w tps      670w tps
    //  atomic_shared_mutex    920w tps     1470w tps
    template<size_t retreatFreq = 32>
    class basic_atomic_shared_mutex
    {
    private:
        std::atomic_bool writer = false;
        std::atomic_bool upgraded = false;
        std::atomic<size_t> readers = 0;

    public:
        void lock()
        {
            size_t i = 0;
            while (true)
            {
                bool expected = false;
                if (writer.compare_exchange_weak(
                    expected, true,
                    std::memory_order::acquire, std::memory_order::relaxed
                ))break;
                i++;
                if (i >= retreatFreq) { i = 0; std::this_thread::yield(); }
            }
            while (true)
            {
                if (readers.load(std::memory_order::acquire) == 0)break;
                i++;
                if (i >= retreatFreq) { i = 0; std::this_thread::yield(); }
            }
        }

        void unlock() { writer.store(false, std::memory_order::release); }

        bool try_lock()
        {
            bool expected = writer.load(std::memory_order::acquire);
            if (expected)return false;  // 已有写锁
            if (!readers.load(std::memory_order::acquire) == 0)
                return false;           // 有读锁
            if(!writer.compare_exchange_strong(
                expected, true,
                std::memory_order::acquire, std::memory_order::relaxed
            ))return false;             // 获取写锁失败
            return true;
        }
        
        void lock_shared()
        {
            size_t i = 0;
            while (true)
            {
                if (!writer.load(std::memory_order::acquire) && !upgraded.load(std::memory_order::acquire))
                {
                    readers.fetch_add(1, std::memory_order::acquire);
                    if (!writer.load(std::memory_order::acquire) && !upgraded.load(std::memory_order::acquire))return;
                    readers.fetch_sub(1, std::memory_order::relaxed);
                }
                i++;
                if (i >= retreatFreq) { i = 0; std::this_thread::yield(); }
            }
        }

        void unlock_shared() { readers.fetch_sub(1, std::memory_order::release); }

        bool try_lock_shared()
        {
            if (!writer.load(std::memory_order::acquire) && !upgraded.load(std::memory_order::acquire))
            {
                readers.fetch_add(1, std::memory_order::acquire);
                if (!writer.load(std::memory_order::acquire) && !upgraded.load(std::memory_order::acquire))return true;
                readers.fetch_sub(1, std::memory_order::relaxed);
            }
            return false;
        }

        // 升级和降级。upgrade 的返回值应当传递给 downgrade 的 before 参数。
        void upgrade()
        {
            readers.fetch_add(0xFFFFFFFF, std::memory_order::acquire);
            size_t i = 0;
            while (true)
            {
                bool expected = false;
                if(upgraded.compare_exchange_strong(
                    expected, true,
                    std::memory_order::acq_rel, std::memory_order::relaxed
                ))break;
                i++;
                if (i >= retreatFreq) { i = 0; std::this_thread::yield(); }
            }
            while (true)
            {
                if ((readers.load(std::memory_order_acquire) & 0xFFFFFFFF) == 0)break;
                i++;
                if (i >= retreatFreq) { i = 0; std::this_thread::yield(); }
            }
        }

        void downgrade()
        {
            readers.fetch_sub(0xFFFFFFFF, std::memory_order::release);
            upgraded.store(false, std::memory_order::release);
        }

        bool try_upgrade()
        {
            readers.fetch_add(0xFFFFFFFF, std::memory_order::acquire);

            bool expected = false;
            if (!upgraded.compare_exchange_strong(
                expected, true,
                std::memory_order::acq_rel, std::memory_order::relaxed))
            {
                readers.fetch_sub(0xFFFFFFFF, std::memory_order::relaxed);
                return false;
            }

            if ((readers.load(std::memory_order::relaxed) & 0xFFFFFFFFu) != 1u)
            {
                upgraded.store(false, std::memory_order::relaxed);
                readers.fetch_sub(0xFFFFFFFF, std::memory_order::relaxed);
                return false;
            }

            return true;
        }
    };

    using atomic_shared_mutex = basic_atomic_shared_mutex<>;


    // 将在多次失败之后回退到系统调度的混合型互斥锁
    template<size_t retreatFreq = 32>
    class mixed_mutex_temp
    {
    private:
        std::atomic_bool lck = false;

    public:
        void lock()
        {
            while (true)
            {
                for (size_t i = 0; i < retreatFreq; i++)
                    if (try_lock())return;
                lck.wait(true, std::memory_order::relaxed);
            }
        }

        void unlock() { lck.store(false, std::memory_order::release); lck.notify_one(); }

        bool try_lock()
        {
            bool expected = false;
            return lck.compare_exchange_strong(expected, true, std::memory_order::acquire);
        }
    };

    using mixed_mutex = mixed_mutex_temp<>;

    template<size_t retreatFreq = 32>
    class mixed_shared_mutex_temp
    {
    private:
        std::atomic_bool writer = false;
        std::atomic_bool upgraded = false;
        std::atomic<size_t> readers = 0;

    public:
        void lock()
        {
            size_t i = 0;
            while (true)
            {
                bool expected = false;
                if (writer.compare_exchange_weak(
                    expected, true,
                    std::memory_order::acquire, std::memory_order::relaxed
                ))break;
                i++;
                if (i >= retreatFreq)
                    writer.wait(true, std::memory_order::relaxed);
            }
            i = 0;
            while (true)
            {
                size_t old;
                if ((old = readers.load(std::memory_order::acquire)) == 0)break;
                i++;
                if (i >= retreatFreq)
                    readers.wait(old, std::memory_order::relaxed);
            }
        }

        void unlock() { writer.store(false, std::memory_order::release); writer.notify_all(); }

        bool try_lock()
        {
            bool expected = writer.load(std::memory_order::acquire);
            if (expected)return false;  // 已有写锁
            if (!readers.load(std::memory_order::acquire) == 0)
                return false;           // 有读锁
            if (!writer.compare_exchange_strong(
                expected, true,
                std::memory_order::acquire, std::memory_order::relaxed
            ))return false;             // 获取写锁失败
            return true;
        }

        void lock_shared()
        {
            size_t i = 0;
            while (true)
            {
                if (!writer.load(std::memory_order::acquire) && !upgraded.load(std::memory_order::acquire))
                {
                    readers.fetch_add(1, std::memory_order::acquire);
                    if (!writer.load(std::memory_order::acquire) && !upgraded.load(std::memory_order::acquire))return;
                    readers.fetch_sub(1, std::memory_order::relaxed);
                    readers.notify_all();
                }
                i++;
                if (i > retreatFreq)
                {
                    writer.wait(true, std::memory_order::relaxed);
                    upgraded.wait(true, std::memory_order::relaxed);
                }
            }
        }

        void unlock_shared() { readers.fetch_sub(1, std::memory_order::release); readers.notify_all(); }

        bool try_lock_shared()
        {
            if (!writer.load(std::memory_order::acquire) && !upgraded.load(std::memory_order::acquire))
            {
                readers.fetch_add(1, std::memory_order::acquire);
                if (!writer.load(std::memory_order::acquire) && !upgraded.load(std::memory_order::acquire))return true;
                readers.fetch_sub(1, std::memory_order::relaxed);
                readers.notify_all();
            }
            return false;
        }

        // 升级和降级。upgrade 的返回值应当传递给 downgrade 的 before 参数。
        void upgrade()
        {
            readers.fetch_add(0xFFFFFFFF, std::memory_order::acquire);
            size_t i = 0;
            while (true)
            {
                bool expected = false;
                if (upgraded.compare_exchange_strong(
                    expected, true,
                    std::memory_order::acquire, std::memory_order::relaxed
                ))break;
                i++;
                if (i >= retreatFreq)
                    upgraded.wait(true, std::memory_order::relaxed);
            }
            i = 0;
            while (true)
            {
                size_t old;
                if (((old = readers.load(std::memory_order_acquire)) & 0xFFFFFFFF) == 0)break;
                i++;
                if (i >= retreatFreq)
                    readers.wait(old, std::memory_order::relaxed);
            }
        }

        void downgrade()
        {
            readers.fetch_sub(0xFFFFFFFF, std::memory_order::release);
            upgraded.store(false, std::memory_order::release);
            readers.notify_all();
            upgraded.notify_all();
        }
    };

    using mixed_shared_mutex = mixed_shared_mutex_temp<>;


    // 在同一线程重复上锁时不会重复操作底层锁的读写锁
    // 有效解决读写锁在重入时的性能和死锁问题
    // 要求解锁必须按照上锁的逆序进行，与标准库一致，否则行为未定义
    template<typename L>
    class thread_mutex
    {
    private:
        struct state
        {
            size_t readers = 0;
            size_t writer = 0;
            bool upgraded = 0;
        };

    private:
        static thread_local std::unordered_map<const void*, state> states;

        L mtx;

    private:
        state& state() { return states[static_cast<const void*>(this)]; }

    public:
        void lock()
        {
            auto& s = state();
            if (s.writers++ > 0) return; // 已持有写锁，重入仅计数

            if (s.readers > 0)
            {
                // 有本线程的读锁：需要升级到写锁（阻塞式）
                mtx.upgrade();
                s.upgraded = true;
                // 升级后底层已成为写锁（并保留了本线程的读计数语义）
            }
            else
            {
                mtx.lock();
            }
        }

        void unlock()
        {
            auto& s = state();
            if (--s.writers > 0) return; // 仍有重入写锁，延迟释放

            // 最后一个写锁释放：根据是否通过 upgrade 获得决定降级或直接释放
            if (s.upgraded)
            {
                // 之前由读升级到写：降级恢复为读（底层会处理 readers 计数）
                mtx.downgrade();
                s.upgraded = false;
                // 保留 s.readers 原来的计数（调用者可能还持有读锁）
            }
            else
            {
                mtx.unlock();
            }
        }

        bool try_lock()
        {
            auto& s = state();
            if (s.writers > 0)
            {
                ++s.writers;
                return true;
            }
            if (s.readers > 0)
            {
                if (mtx.try_upgrade())
                {
                    ++s.writers;
                    s.upgraded = true;
                    return true;
                }
                else return false;
            }
            if (mtx.try_lock())
            {
                ++s.writers;
                return true;
            }
            return false;
        }

        void lock_shared()
        {
            auto& s = state();
            if (s.writers > 0)
            {
                // 已持有写锁，本次 shared 只是逻辑计数
                ++s.readers;
                return;
            }
            if (s.readers++ == 0)
                mtx.lock_shared(); // 真正第一次读时申请底层读锁
        }

        void unlock_shared()
        {
            auto& s = state();
            if (s.writers > 0)
            {
                // 在写锁下的读只是局部计数
                if (s.readers > 0) --s.readers;
                return;
            }
            if (s.readers == 0) return; // 防御性：多余的 unlock 忽略（或可断言）
            if (--s.readers == 0)
                mtx.unlock_shared(); // 最后一个本线程的读释放底层读锁
        }

        bool try_lock_shared()
        {
            auto& s = state();
            if (s.writers > 0)
            {
                ++s.readers;
                return true;
            }
            if (s.readers == 0)
            {
                if (mtx.try_lock_shared())
                {
                    ++s.readers;
                    return true;
                }
                return false;
            }
            else
            {
                // 已有本线程读锁，重入直接成功
                ++s.readers;
                return true;
            }
        }
    };


    template<typename L>
        requires requires(L l) { l.upgrade(); l.downgrade(); }
    class upgrade_lock
    {
    private:
        L& smtx;

    public:
        upgrade_lock(L& m) :smtx(m) { smtx.upgrade(); }
        ~upgrade_lock() { smtx.downgrade(); }
    };
}




/*************** 合并自 lib_info.h ***************/
// #pragma once
// #include "pch.h"
// #include "framework.h"

// #include "astring.h"


namespace HYDRA15::Union::framework
{
    static_string libName = "HYDRA15.Union";
    static_string version = "ver.lib.beta.1.2.0";

    // 子系统代码
    static struct libID
    {
        static_uint Union = 0x000000;
        static_uint referee = 0x0A0000;
        static_uint assistant = 0x0B0000;
        static_uint labourer = 0x0C0000;
        static_uint secretary = 0x0D0000;
    }libID;

}



/*************** 合并自 progress.h ***************/
// #pragma once
// #include "framework.h"
// #include "pch.h"

namespace HYDRA15::Union::secretary
{
    // 格式化进度条
    // 返回格式化后的字符串，用户需要自行处理输出
    class progress
    {
        // 禁止构造
    private:
        progress() = delete;
        progress(const progress&) = delete;
        ~progress() = delete;

        // 公有数据
    private:
        static struct visualize
        {
            static_string digitalProgressFormat = "{0}... {1:02d}%";
            static_string simpleBarFormat = "{0}[{1}] {2:02d}%";
        }vslz;

        // 接口
    public:
        static std::string digital(std::string title, float progress)
        {
            if (progress < 0.0f) progress = 0.0f;
            if (progress > 1.0f) progress = 1.0f;
            return std::format(
                vslz.digitalProgressFormat.data(),
                title,
                static_cast<int>(progress * 100)
            );
        }

        static std::string simple_bar(std::string title, float progress, unsigned int barWidth = 10, char barChar = '=')
        {
            if (progress < 0.0f) progress = 0.0f;
            if (progress > 1.0f) progress = 1.0f;

            size_t bar = static_cast<size_t>(barWidth * progress);
            size_t space = barWidth - bar;

            return std::format(
                vslz.simpleBarFormat.data(),
                title,
                std::string(bar, barChar) + std::string(space, ' '),
                static_cast<int>(progress * 100)
            );
        }
    };

}



/*************** 合并自 string_utilities.h ***************/
// #pragma once
// #include "pch.h"
// #include "framework.h"

namespace HYDRA15::Union::assistant
{
    // 将一个字符串重复数遍
    inline std::string operator*(std::string str, size_t count)
    {
        std::string result;
        result.reserve(str.size() * count);
        for (size_t i = 0; i < count; ++i) {
            result += str;
        }
        return result;
    }

    // 删除头尾的空字符，默认删除所有非可打印字符和空格
    inline std::string strip_front(
        const std::string& str,
        std::function<bool(char)> is_valid = [](char c) {return c > 0x20 && c < 0x7F; }
    ) {
        std::string res = str;
        size_t pos = 0;
        while (pos < res.size())
        {
            if (is_valid(res[pos]))
                break;
            pos++;
        }
        res.erase(0, pos);
        return res;
    }

    inline std::string strip_back(
        const std::string& str,
        std::function<bool(char)> is_valid = [](char c) {return c > 0x20 && c < 0x7F; }
    ) {
        std::string res = str;
        size_t pos = res.size() - 1;
        while (pos > 0)
        {
            if (is_valid(res[pos]))
                break;
            pos--;
        }
        res.erase(pos + 1);
        return res;
    }

    inline std::string strip(
        const std::string& str,
        std::function<bool(char)> is_valid = [](char c) {return c > 0x20 && c < 0x7F; }
    ) {
        return strip_back(strip_front(str, is_valid), is_valid);
    }

    // 删除字符串中所有的空字符，默认删除所有非可打印字符和空格
    inline std::string strip_all(
        const std::string& str,
        std::function<bool(char)> is_valid = [](char c) {return c > 0x20 && c < 0x7F; }
    ) {
        std::string res;
        res.reserve(str.size());

        for (char c : str)
            if (is_valid(c))
                res.push_back(c);

        res.shrink_to_fit();
        return res;
    }

    // 删除字符串中的ansi颜色格式
    inline std::string strip_color(const std::string& str)
    {
        std::string res;
        res.reserve(str.size());

        for (size_t i = 0; i < str.size(); i++)
        {
            if (str[i] == '\033' && i + 1 < str.size() && str[i + 1] == '[') // 发现 ANSI 转义序列起始
            {
                // 查找 'm' 结尾
                size_t j = i + 2;
                while (j < str.size() && str[j] != 'm')
                    j++;
                if (j < str.size() && str[j] == 'm') {
                    // 找到完整的 ANSI 颜色代码，跳过整个序列 (从 \033 到 m)
                    i = j;
                    continue;
                }
            }
            // 将可保留的字符复制到 res 中
            res.push_back(str[i]);
        }

        return res;
    }

    // 删除字符串中所有的 ansi 转义串
    // ansi 转义串以 \0x1b 开始，任意字母结束
    inline std::string strip_ansi_secquence(const std::string& str)
    {
        if (str.empty())return std::string();

        auto is_charactor = [](char c) {return (c >= 0x41 && c <= 0x5A) || (c >= 0x61 && c <= 0x7A); };

        std::string res;
        res.reserve(str.size());

        size_t fast = 0, slow = 0;

        while (slow < str.size())
        {
            fast = str.find('\x1b', fast);
            if (fast == str.npos)fast = str.size();
            res.append(str.substr(slow, fast));
            for (; fast < str.size(); fast++)
                if (is_charactor(str[fast]))
                    break;
            slow = ++fast;
        }

        return res;
    }

    // 检查字符串内容是否全部合法，如不合法则报错
    inline bool check_content(
        const std::string& str,
        std::function<bool(char)> is_valid = [](char c) {return c > 0x20 && c < 0x7F; },
        bool throwExpt = true
    ) {
        for (const auto& c : str)
            if (!is_valid(c))
                if (throwExpt)
                    throw exceptions::common("Invalid character detected");
                else return false;
        return true;
    }

    // 用给定的字符切分字符串
    inline std::list<std::string> split_by(
        const std::string& str,
        const std::string& delimiter = " "
    ) {
        auto slow = str.cbegin();
        auto fast = str.cbegin();
        std::list<std::string> res;

        while (fast != str.cend())
        {
            fast = std::search(slow, str.cend(), delimiter.cbegin(), delimiter.cend());
            res.emplace_back(slow, fast);
            if (fast != str.cend())
                fast += delimiter.size();
            slow = fast;
        }
        return res;
    }

    inline std::list<std::string> split_by(
        const std::string& str,
        const std::list<std::string>& deliniters
    ) {
        std::list<std::string> res = { str };

        for (const auto& deliniter : deliniters)
        {
            std::list<std::string> strs;
            strs.swap(res);
            for (const auto& str : strs)
                res.splice(res.end(), split_by(str, deliniter));
        }

        return res;
    }

    template<typename C>
        requires requires(const C& c) {
            { c.begin() } -> std::input_or_output_iterator;
            { c.end() } -> std::sentinel_for<decltype(c.begin())>;
            { std::to_string(*(c.begin())) }->std::convertible_to<std::string>;
            { c.empty() }->std::convertible_to<bool>;
    }
    std::string container_to_string(const C& c)
    {
        if (c.empty())return {};
        std::string res;
        for (const auto& i : c)
            res += std::to_string(i) + ", ";
        res.pop_back(); res.pop_back();
        return res;
    }
}



/*************** 合并自 background.h ***************/
// #pragma once
// #include "pch.h"
// #include "framework.h"

// #include "iMutexies.h"
namespace HYDRA15::Union::labourer
{
    // 继承此类的子类将在初始化时自动根据设定的参数启动后台线程
    // 使用方法：
    //   - 重写 work() 方法
    //   - 构造函数中指定线程参数
    //   - 子类初始化完成后调用 start() 方法启动线程
    //   - 结束工作后，需要自行通知工作线程结束任务并返回
    //   - 结束工作后调用 stop() 方法等待线程返回，所有线程都返回后此函数将返回
    // 线程的数量一经初始化后不可更改，需要动态调整线程数量建议创建多个 Background 实例
    class background
    {
    private:
        std::barrier<> checkpoint;  //启动和结束同步
        std::list<std::thread> threads; // 异步线程组

        void work_shell() // 封装了启动与结束同步的工作函数
        {
            // 等待启动信号
            checkpoint.arrive_and_wait();
            // 执行工作
            work();
            // 等待所有线程完成工作
            auto t = checkpoint.arrive();
        }

    protected:
        virtual void work() noexcept = 0;  // 重写此方法以异步执行

        // 启动同步和结束同步
    protected:
        void start() { checkpoint.arrive_and_wait(); }  // 某些系统依赖后台线程初始化完成才能正常工作

        void detach() { auto t = checkpoint.arrive(); } // 无需等待后台线程初始化完成可以使用此接口

        void wait_for_end() { checkpoint.arrive_and_wait(); }

        // 构造函数，参数为异步线程数量，默认为1
    protected:
        background(unsigned int bkgThrCount)
            : checkpoint(bkgThrCount + 1)
        {
            for (unsigned int i = 0; i < bkgThrCount; i++)
                threads.emplace_back(&background::work_shell, this);
        }

        background() :background(1) {}

        virtual ~background() { for (auto& i : threads)if (i.joinable())i.detach(); }

        background(background&&) = delete;
        background(const background&) = delete;
        background& operator=(const background&) = delete;
    };

}




/*************** 合并自 secretary_streambuf.h ***************/
// #pragma once
// #include "pch.h"
// #include "framework.h"

// #include "iMutexies.h"


namespace HYDRA15::Union::secretary
{
    // AI 生成的代码
    // 自定义输出流缓冲区，自动将缓冲区内容传递给PrintCenter，用于重定向 std::cout
    class ostreambuf : public std::streambuf {
    public:
        explicit ostreambuf(std::function<void(const std::string&)> c, std::size_t initial_size = 256, std::size_t max_size = 65536)
            : buffer_(initial_size), max_size_(max_size), callback(c)
        {
            setp(buffer_.data(), buffer_.data() + buffer_.size());
        }

    protected:
        int_type overflow(int_type ch) override
        {
            std::lock_guard lg(mtx_);
            if (ch == traits_type::eof()) return traits_type::not_eof(ch);

            if (pptr() == epptr()) {
                if (buffer_.size() < max_size_) {
                    expand_buffer();
                }
                else {
                    flush_buffer();
                }
            }
            *pptr() = ch;
            pbump(1);

            if (ch == '\n') {
                flush_buffer();
            }
            return ch;
        }

        std::streamsize xsputn(const char* s, std::streamsize n) override
        {
            std::lock_guard lg(mtx_);
            std::streamsize written = 0;
            while (written < n) {
                std::size_t space_left = epptr() - pptr();
                if (space_left == 0) {
                    if (buffer_.size() < max_size_) {
                        expand_buffer();
                        space_left = epptr() - pptr();
                    }
                    else {
                        flush_buffer();
                        space_left = epptr() - pptr();
                    }
                }
                std::size_t to_write = std::min<std::size_t>(space_left, n - written);

                const char* nl = static_cast<const char*>(memchr(s + written, '\n', to_write));
                if (nl) {
                    std::size_t nl_pos = nl - (s + written);
                    std::memcpy(pptr(), s + written, nl_pos + 1);
                    pbump(static_cast<int>(nl_pos + 1));
                    written += nl_pos + 1;
                    flush_buffer();
                }
                else {
                    std::memcpy(pptr(), s + written, to_write);
                    pbump(static_cast<int>(to_write));
                    written += to_write;
                }
            }
            return written;
        }

        int sync() override
        {
            std::lock_guard lg(mtx_);
            flush_buffer();
            return 0;
        }

    private:
        std::vector<char> buffer_;
        std::size_t max_size_;
        labourer::atomic_mutex mtx_;
        std::function<void(const std::string&)> callback;

        void expand_buffer()
        {
            std::lock_guard lg(mtx_);
            std::size_t current_size = buffer_.size();
            std::size_t new_size = std::min(current_size * 2, max_size_);
            std::ptrdiff_t offset = pptr() - pbase();
            buffer_.resize(new_size);
            setp(buffer_.data(), buffer_.data() + buffer_.size());
            pbump(static_cast<int>(offset));
        }

        void flush_buffer()
        {
            std::ptrdiff_t n = pptr() - pbase();
            if (n > 0) {
                callback(std::string(pbase(), n));
                pbump(static_cast<int>(-n));
            }
        }
    };

    // AI生成的代码
    // 自定义输入流缓冲区，用于将输入重定向至 Command 类，用于重定向 std::cin
    class istreambuf : public std::streambuf 
    {
    private:
        std::string buffer_;   // 使用 std::string 作为缓冲区（自动管理内存）
        bool eof_ = false;     // 是否已到逻辑 EOF
        std::mutex mtx_;

        // 回调函数：用于获取一行输入
        std::function<std::string()> getline_callback;

        // 从 Command::getline() 获取一行并加载到 buffer_
        bool refill()
        {
            if (eof_) return false;

            buffer_ = std::move(getline_callback());
            if (buffer_.empty()) {
                // 假设返回空字符串表示输入结束（EOF）
                eof_ = true;
                return false;
            }

            // 将整行 + 换行符存入 buffer_
            // 注意：即使原输入无 '\n'，我们也添加一个，以模拟标准输入行为
            buffer_ += '\n';

            // 设置 get area: [begin, end)
            char* beg = &buffer_[0];
            char* end = beg + buffer_.size();
            setg(beg, beg, end);

            return true;
        }

    public:
        // 构造时传入回调
        explicit istreambuf(std::function<std::string()> callback)
            : getline_callback(std::move(callback))
        {
            setg(nullptr, nullptr, nullptr); // 初始化 get area
        }

        // 重写 underflow：单字符读取时调用
        int underflow() override
        {
            std::unique_lock ul{ mtx_ };
            if (gptr() < egptr()) {
                return traits_type::to_int_type(*gptr());
            }
            if (!refill()) {
                return traits_type::eof();
            }
            return traits_type::to_int_type(*gptr());
        }

        // 1. 优化批量读取：重写 xsgetn
        std::streamsize xsgetn(char* s, std::streamsize count) override
        {
            std::unique_lock ul{ mtx_ };
            std::streamsize total = 0;

            while (total < count) {
                // 当前缓冲区中可用字符数
                std::streamsize avail = egptr() - gptr();
                if (avail > 0) {
                    std::streamsize to_copy = std::min(avail, count - total);
                    std::copy(gptr(), gptr() + to_copy, s + total);
                    gbump(static_cast<int>(to_copy)); // 移动 get 指针
                    total += to_copy;
                }
                else {
                    // 缓冲区为空，尝试 refill
                    if (!refill()) {
                        // EOF：返回已读取的字节数（可能为 0）
                        break;
                    }
                    // refill 成功后，下一轮循环会复制数据
                }
            }

            return total;
        }

        // 2. 支持 cin.getline()：确保换行符被正确消费
        //    标准要求：getline 会读取直到 '\n' 或缓冲区满，并丢弃 '\n'
        //    我们的 underflow/xsgetn 已提供 '\n'，因此无需额外处理
        //    但需确保：当遇到 EOF 且无 '\n' 时，仍能正确终止
        //    —— 这由标准库的 istream::getline 逻辑处理，我们只需提供字符流

        // 可选：重写 showmanyc() 以提示可用字符数（非必需，但可优化）
        std::streamsize showmanyc() override
        {
            std::unique_lock ul{ mtx_ };
            if (gptr() < egptr()) {
                return egptr() - gptr();
            }
            return eof_ ? -1 : 0; // -1 表示 EOF
        }

    };
}



/*************** 合并自 shared_containers.h ***************/
// #pragma once
// #include "pch.h"
// #include "framework.h"

// #include "concepts.h"
// #include "iMutexies.h"

namespace HYDRA15::Union::labourer
{
    // 基于 std::queue std::mutex std::conditional_variable 的基本阻塞队列
    template<typename T>
    class basic_blockable_queue
    {
        std::queue<T> queue;
        std::mutex mtx;
        std::condition_variable cv;
        std::atomic_bool working = true;

    public:
        template<typename U>
            requires framework::is_really_same_v<T, U>
        void push(U&& item)
        {
            if (!working.load(std::memory_order_acquire))return;
            std::unique_lock ul{ mtx };
            queue.push(std::forward<U>(item));
            cv.notify_one();
            return;
        }

        T pop()
        {
            std::unique_lock ul{ mtx };
            while (working.load(std::memory_order_acquire) && queue.empty())cv.wait(ul);
            if (!working.load(std::memory_order_acquire))return T{};
            if constexpr (std::is_move_constructible_v<T>)
            {
                T t = std::move(queue.front());
                queue.pop();
                return std::move(t);
            }
            else
            {
                T t = queue.front();
                queue.pop();
                return t;
            }
        }

        size_t size() const { return queue.size(); }
        bool empty() const { return queue.empty(); }

        void notify_exit()
        {
            working.store(false, std::memory_order_release);
            cv.notify_all();
        }

    };



    // 无锁队列采用环形缓冲区 + 序号标记实现
    // 定长缓冲区
    // 可能存在忙等问题。要求元素有空构造
    // 性能测试：
    //                           debug       release
    //  std::queue + std::mutex 20w tps     100w tps
    //  lockless_queue<1M>     250w tps     300w tps
    template<typename T, size_t bufSize, size_t maxRetreatFreq = 32>
    class lockless_queue
    {
        struct cell { std::atomic<size_t> seqNo{}; T data{}; };
    private:
        alignas(64) std::unique_ptr<cell[]> buffer = std::make_unique<cell[]>(bufSize);
        alignas(64) std::atomic<size_t> pNextEnque = 0;
        alignas(64) std::atomic<size_t> pNextDeque = 0;
        std::atomic_bool working = true;

    public:
        lockless_queue() { for (size_t i = 0; i < bufSize; i++)buffer[i].seqNo = i; }
        lockless_queue(const lockless_queue&) = delete;
        lockless_queue(lockless_queue&&) = default;

        // 入出队
        template<typename U>
            requires framework::is_really_same_v<T, U>
        void push(U&& item)
        {
            size_t retreatFreq = maxRetreatFreq;
            while (working.load(std::memory_order_acquire))
            {
                for (size_t i = 0; i < retreatFreq; i++)
                    if (try_push(std::forward<U>(item)))
                        return;
                if (retreatFreq > 1)retreatFreq /= 2;
                std::this_thread::yield();
            }
        }

        T pop()
        {
            size_t retreatFreq = maxRetreatFreq;
            while (working.load(std::memory_order_acquire))
            {
                for (size_t i = 0; i < retreatFreq; i++)
                    if (auto [success, item] = try_pop(); success)
                        if constexpr (std::is_move_constructible_v<T>) return std::move(item);
                        else return item;
                if (retreatFreq > 1)retreatFreq /= 2;
                std::this_thread::yield();
            }
            return T{};
        }

        template<typename U>
            requires framework::is_really_same_v<T, U>
        bool try_push(U&& item)
        {
            size_t seq = pNextEnque.load(std::memory_order_relaxed);
            size_t p = seq % bufSize;
            if (buffer[p].seqNo.load(std::memory_order_acquire) != seq)
                return false;
            if (!pNextEnque.compare_exchange_strong(seq, seq + 1, std::memory_order_relaxed))
                return false;
            buffer[p].data = std::forward<U>(item);
            buffer[p].seqNo.store(seq + 1, std::memory_order_release);
            return true;
        }

        std::pair<bool, T> try_pop()
        {
            size_t seq = pNextDeque.load(std::memory_order_relaxed);
            size_t p = seq % bufSize;
            if (buffer[p].seqNo.load(std::memory_order_acquire) != seq + 1)
                return { false,T{} };
            if (!pNextDeque.compare_exchange_strong(seq, seq + 1, std::memory_order_relaxed))
                return { false,T{} };
            if constexpr (std::is_move_constructible_v<T>)
            {
                T item = std::move(buffer[p].data);
                buffer[p].seqNo.store(seq + bufSize, std::memory_order_release);
                return { true, std::move(item) };
            }
            else
            {
                T item = buffer[p].data;
                buffer[p].seqNo.store(seq + bufSize, std::memory_order_release);
                return { true,item };
            }

        }

        // 信息和控制
        size_t size() const
        {
            const size_t enq = pNextEnque.load(std::memory_order_acquire);
            const size_t deq = pNextDeque.load(std::memory_order_acquire);
            return (enq >= deq) ? (enq - deq) : std::numeric_limits<size_t>::max() - deq + enq;
        }

        size_t empty() const 
        {  
            return pNextDeque.load(std::memory_order_acquire) ==
                pNextEnque.load(std::memory_order_acquire);
        }

        void notify_exit() { working.store(false, std::memory_order_release); }

    };


    // 基于 atomic_mutex 的共享 set
    template<typename T>
    class basic_shared_set
    {
        atomic_mutex mtx;
        std::set<T> set;

    public:
        template<typename U>
        auto insert(U&& t) { std::unique_lock ul{ mtx }; return set.insert(std::forward<U>(t)); }

        template<typename U>
        auto lower_bound(U&& t) { std::unique_lock ul{ mtx }; return set.lower_bound(std::forward<U>(t)); }

        template<typename U>
        auto upper_bound(U&& t) { std::unique_lock ul{ mtx }; return set.upper_bound(std::forward<U>(t)); }

        auto clear() { std::unique_lock ul{ mtx }; return set.clear(); }
    };
}



/*************** 合并自 lib_exceptions.h ***************/
// #pragma once
// #include "pch.h"
// #include "framework.h"

// #include "iExceptionBase.h"
// #include "lib_info.h"

namespace HYDRA15::Union::exceptions
{
    // 用于没有其他特殊处理的错误
    class common : public referee::iExceptionBase
    {
        static constexpr expt_code cateExptCode = 0x00000000;

    public:
        common& set(const std::string& k, const std::string& v) { iExceptionBase::set(k, v); return *this; }

        common(expt_code libID, expt_code e, const std::string& desp)
            :iExceptionBase(cateExptCode | libID | e, desp) {
        }
        common(const std::string& desp) :iExceptionBase(cateExptCode, desp) {}
        virtual ~common() = default;

    public: // 快速创建常见异常
        // 杂项
        static common BadParameter(const std::string& param, const std::string& provided, const std::string& desired) noexcept
        {
            return common{ cateExptCode, 0x001, "The provided argument is invalid" }
            .set("param", param).set("provided", provided).set("desired", desired);
        }

        static common UnsupportedFormat(const std::string& desp) noexcept
        {
            return common{ cateExptCode,0x002,"Target object contains unsupported format" }
            .set("requires", desp);
        }

        static common UnsupportedInterface(const std::string& vclassName, const std::string& className, const std::string& interfaceName)
        {
            return common{ cateExptCode,0x003,"This interface is not suppotred" }
            .set("interface class", vclassName).set("impl class", className).set("interface", interfaceName);
        }
    };


    class files : public referee::iExceptionBase
    {
        static constexpr expt_code cateExptCode = 0x000A0000;
        
    public:
        files& set(const std::string& k, const std::string& v) { iExceptionBase::set(k, v); return *this; }

    public:
        files(const std::filesystem::path& path, expt_code code, const std::string& desp)
            : iExceptionBase(cateExptCode | code, desp) {
            set("file path", path.string());
        }

        virtual ~files() = default;

    public:
        static files FileIOFlowError(const std::filesystem::path& path, int state) noexcept
        {
            return files{ path, 0xA01, "File stream flow error" }
            .set("state", std::to_string(state));
        }

        static files FileNotExist(const std::filesystem::path& path) noexcept
        {
            return files{ path,0xA02,"File not exist" };
        }

        static files FileAreadyExist(const std::filesystem::path& path) noexcept
        {
            return files{ path,0xA03,"File aready exist" };
        }

        static files FileFull(const std::filesystem::path& path, size_t limit) noexcept
        {
            return files{ path,0xA04,"File size has reached the configured limit." }.set("limit", std::to_string(limit));
        }

        static files ExceedRage(const std::filesystem::path& path, const std::string& relParam, const std::string& req) noexcept
        {
            return files{ path,0xB01,"File request exceed allowed range" }
            .set("relative parameter", relParam).set("reason", req);
        }

        static files FormatNotSupported(const std::filesystem::path& path, const std::string& req) noexcept
        {
            return files{ path,0xB02,"The file format is not supported" }
            .set("reason", req);
        }

        static files ContentNotFound(const std::filesystem::path& path) noexcept
        {
            return files{ path,0xB03,"The requested content was not found." };
        }

        
    };
}



/*************** 合并自 utilities.h ***************/
// #pragma once
// #include "pch.h"
// #include "framework.h"

// #include "string_utilities.h"
// #include "concepts.h"

namespace HYDRA15::Union::assistant
{
    // 输出十六进制的原始数据和对应的ascii字符
    inline std::string hex_heap(const unsigned char* pBegin, unsigned int size, const std::string& title = "Hex Heap", unsigned int preLine = 32)
    {
        std::string str = std::format("   -------- {} --------   \n", title);
        str.reserve((preLine * 0x4 + 0x20) * (size / preLine + 1) + 0x100 + title.size());

        // 打印表头
        str += "          ";
        for (unsigned int i = 0; i < preLine; i++)
            str += std::format("{:02X} ", i);
        str += "\n\n";

        // 打印数据
        std::string dataStr, charStr;
        dataStr.reserve(preLine * 3);
        charStr.reserve(preLine);
        for (unsigned int i = 0; i < size; i++)
        {
            if (i % preLine == 0)   // 行头地址
                str += std::format("{:08X}  ", i);

            dataStr += std::format("{:02X} ", pBegin[i]);
            if (pBegin[i] >= 0x20 && pBegin[i] <= 0x7E)
                charStr += pBegin[i];
            else
                charStr += ' ';

            if ((i + 1) % preLine == 0)
            {
                str += std::format("{}  |{}| \n", dataStr, charStr);
                dataStr.clear();
                charStr.clear();
            }

            if (i == size - 1 && (i + 1) % preLine != 0)  // 最后一行对齐
            {
                dataStr += std::string("   ") * (preLine - i % preLine - 1);
                charStr += std::string(" ") * (preLine - i % preLine - 1);
                str += std::format("{}  |{}| \n", dataStr, charStr);
                dataStr.clear();
                charStr.clear();
            }
        }

        return str;
    }

    // 将字节转换为十六进制（高效版）
    inline std::string byte_to_hex(const unsigned char* pBegin, unsigned int size)
    {
        static char lut[16] = { '0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F' };

        std::string res;
        res.reserve(2LL * size);
        for (const unsigned char* p = pBegin; p < pBegin + size; p++)
        {
            res.push_back(lut[(*p) & 0xF]);
            res.push_back(lut[((*p) >> 4)]);
        }

        return res;
    }
    

    // 解析 propreties 文件
    // 强制要求键值分隔符为 = ，unicode字符保持原样，
    inline std::unordered_map<std::string, std::string> parse_propreties(const std::string& ppts)
    {
        // 预处理：去除所有的空格，处理转义字符
        std::string content;
        content.reserve(ppts.size());
        for (size_t i = 0; i < ppts.size(); i++)
        {
            switch (ppts[i])
            {
                // 去除空格
            case ' ':
                break;
                // 处理转义
            case '\\':
                if (i + 1 >= ppts.size())
                    break;
                switch (ppts[i + 1])
                {
                    // 普通转义
                case 't': content += '\t'; i++; break;
                case 'n': content += '\n'; i++; break;
                case 'r': content += '\r'; i++; break;
                case 'f': content += '\f'; i++; break;
                case '\\': content += '\n'; i++; break;
                case ':': content += ':'; i++; break;
                    // 等号等会再处理
                case '=': content += ppts.substr(i, 2); i++; break;
                    // unicode 字符不处理
                case 'u':
                    if (i + 5 >= ppts.size())
                        throw exceptions::common::UnsupportedFormat("Unicode charactor format must be '\\uxxxx'");
                    content += ppts.substr(i, 6);
                    i += 5;
                    break;
                    // 换行符直接去除
                case '\n': i++; break;
                    // 不知道的保留原样
                default: content += ppts.substr(i, 2); i++; break;
                }
                break;
                // 注释，一直删到行尾
            case '!':
            case '#':
                for (; i < ppts.size(); i++)
                    if (ppts[i] == '\n')
                        break;
                break;
                // 其余字符直接拷贝
            default:
                content += ppts[i];
            }
        }

        // 处理1：按行分割后按分隔符 = 分割
        std::list<std::list<std::string>> entryLst;
        {
            std::list<std::string> itemLst = split_by(content, "\n");
            content.~basic_string();    // 节省空间
            for (const auto& item : itemLst)
                entryLst.push_back(split_by(item, "="));
        }

        // 处理2：处理 \n 转义
        for (auto& entryPair : entryLst)
        {
            auto it = entryPair.begin();
            while (it != entryPair.end())
            {
                if (it->back() == '\\')
                {
                    auto current = it++;
                    current->pop_back();
                    *current += "=" + *it;
                    it = entryPair.erase(it);
                }
                else
                    it++;
            }
        }

        // 以上处理完成后，每个 list 中应该只有两个元素，分别为键和值
        std::unordered_map<std::string, std::string> res;
        for (auto& entryPair : entryLst)
        {
            if (entryPair.size() != 2)
                throw exceptions::common::UnsupportedFormat("Key - value pair joined by '='");
            res.emplace(std::pair{ std::move(entryPair.front()),std::move(entryPair.back()) });
        }

        return res;
    }

    // 解析 csv 文件
    // 支持引号不分割，但是不删除引号
    inline std::list<std::list<std::string>> parse_csv(const std::string& csv)
    {
        std::list<std::list<std::string>> res;

        std::list<std::string> lines = split_by(csv, "\n");
        for (const auto& line : lines)
        {
            std::list<std::string> entries = split_by(line, ",");
            auto ientry = entries.begin();
            while (ientry != entries.end()) // 处理引号不分割
            {
                if (strip(*ientry).front() = '"')
                {
                    auto next = ientry;
                    next++;
                    while (next != entries.end() && strip(*next).back() != '"')
                    {
                        (*ientry).append(*next);
                        next = entries.erase(next);
                    }
                    if (next != entries.end())
                        (*ientry).append(*next);
                }
                ientry++;
            }
            res.push_back(entries);
        }

        return res;
    }


    // 内存拷贝
    template<typename T>
    void memcpy(const T* src, T* dest, size_t size)
    {
        for (size_t i = 0; i < size; i++)
            dest[i] = src[i];
    }

    // 内存设置
    template<typename T>
    void memset(T* dest, const T& src, size_t size)
    {
        for (size_t i = 0; i < size; i++)
            dest[i] = src;
    }



    // 打印多个参数到控制台
    template<typename ... Args>
    std::ostream& print(Args ... args)
    {
        return (std::cout << ... << args);
    }


    // 计算不小于某数的倍数
    inline size_t multiple_m_not_less_than_n(size_t m, size_t n)
    {
        if (m == 0) return 0;
        return ((n + m - 1) / m) * m;
    }

    // 计算不小于某数的2的幂次
    inline size_t power_of_2_not_less_than_n(size_t n)
    {
        if (n == 0) return 0;
        size_t power = 1;
        while (power < n)
            power <<= 1;
        return power;
    }


    // 集合操作
    namespace set_operation
    {
        template<typename T, template<typename ...>typename S>
        concept is_set_container =
            framework::is_really_same_v<S<T>, std::set<T>> ||
            framework::is_really_same_v<S<T>, std::unordered_set<T>>;

        // 并集
        template<typename T, template<typename ...>typename S>
            requires is_set_container<T, S>
        inline S<T> operator+(const S<T>& l, const S<T>& r)
        {
            S<T> res;
            res.insert_range(l);
            res.insert_range(r);
            return res;
        }

        // 交集
        template<typename T, template<typename ...>typename S>
            requires is_set_container<T, S>
        inline S<T> operator&(const S<T>& l, const S<T>& r)
        {
            if (l.size() < r.size())
            {
                S<T> res;
                for (const auto& i : l)
                    if (r.contains(i))
                        res.insert(i);
                return res;
            }
            else 
            {
                S<T> res;
                for (const auto& i : r)
                    if (l.contains(i))
                        res.insert(i);
                return res;
            }
        }

        // 查集
        template<typename T, template<typename ...>typename S>
            requires is_set_container<T, S>
        inline S<T> operator-(const S<T>& l, const S<T>& r)
        {
            S<T> res;
            for (const auto& i : l)
                if (!r.contains(i))
                    res.insert(i);
            return res;
        }

        // 对称交集
        template<typename T, template<typename ...>typename S>
            requires is_set_container<T, S>
        inline S<T> operator|(const S<T>& l, const S<T>& r)
        {
            return (l + r) - (l & r);
        }
    }
    
   

}



/*************** 合并自 ThreadLake.h ***************/
// #pragma once
// #include "pch.h"
// #include "framework.h"

// #include "background.h"
// #include "concepts.h"
// #include "shared_containers.h"


namespace HYDRA15::Union::labourer
{
    /***************************** 线程池基础 *****************************/
    // 预留任务调度策略的改造空间

    // 定义任务工作的接口，任务细节存储在派生类中
    class mission_base
    {
    public:
        virtual ~mission_base() = default;

        virtual void operator()() noexcept = 0;
    };
    using mission = std::unique_ptr<mission_base>;

    // 定义线程池的基本行为：提交任务、线程执行任务
    template<typename queue_t>
        requires requires(queue_t q, mission pkg)
    {
        { q.push(pkg) };                            // 应当是阻塞式
        { q.pop() }-> std::convertible_to<mission>; // 应当是阻塞式
        { q.empty() }->std::convertible_to<bool>;
        { q.notify_exit() };                        // 用于结束时使用，通知等待线程应该退出
    }
    class thread_pool : public background
    {
    protected: // 数据
        queue_t queue;
    private:
        std::atomic_bool working = true;
        std::atomic<unsigned int> activeCount = 0;
        std::mutex gexptrMtx;
        std::exception_ptr gexptr = nullptr;

    private: // 后台任务
        virtual void work() noexcept override
        {
            try
            {
                mission mis;
                while (working.load(std::memory_order::relaxed) || !queue.empty())
                {
                    // 取任务
                    mis = queue.pop();
                    // 执行任务
                    activeCount.fetch_add(1, std::memory_order::relaxed);
                    (*mis)();
                    activeCount.fetch_add(-1, std::memory_order::relaxed);
                }
            }
            catch (...) { std::unique_lock ul{ gexptrMtx }; gexptr = std::current_exception(); }
        }

    public: // 提交接口
        void submit(mission&& mis) { queue.push(std::move(mis)); }

    public: // 构造
        thread_pool() = delete;
        thread_pool(const thread_pool&) = delete;
        thread_pool(thread_pool&&) = delete;
        thread_pool(unsigned int threadCount) :background(threadCount) { background::start(); }
        virtual ~thread_pool()
        {
            working.store(false, std::memory_order_release);
            queue.notify_exit();
            background::wait_for_end();
        }

    public: // 管理接口
        size_t size() const { return queue.size(); }
        unsigned int active() const { return activeCount.load(std::memory_order::relaxed); }
        bool alive() const  // 任意一个线程出错则抛出最后一个异常，否则返回 true
        { 
            std::unique_lock ul{ gexptrMtx }; 
            if (gexptr)std::rethrow_exception(gexptr); 
            return true; 
        } 
    };

    /***************************** 基本线程池实现 *****************************/
    // 专用于 ThreadLake 的任务实现
    template<typename ret>
    class lake_mission : public mission_base
    {
        std::function<ret()> tsk;
        std::function<void(const ret&)> cb;
        std::promise<ret> prms;
    public:
        virtual ~lake_mission() = default;
        lake_mission(const std::function<ret()>& task, const std::function<void(const ret&)>& callback)
            :tsk(task), cb(callback) {
        }

        virtual void operator()() noexcept override
        {
            try
            {
                if (!tsk)return;
                if constexpr (std::is_void_v<ret>) { tsk(); if (cb)cb(); prms.set_value(); }
                else { 
                    ret t = tsk(); if (cb)cb(t); 
                    if constexpr (std::is_move_constructible_v<ret>)prms.set_value(std::move(t));
                    else prms.set_value(t);
                }
                return;
            }
            catch (...) { prms.set_exception(std::current_exception()); }
        }

        std::future<ret> get_future() { return prms.get_future(); }
    };


    template<template<typename ...> typename queue_t>
    class thread_lake : public thread_pool<queue_t<mission>>
    {
    public: // 构造
        virtual ~thread_lake() = default;
        thread_lake(unsigned int threadCount) :thread_pool<queue_t<mission>>(threadCount) {}
    public: // 提交任务和回调函数
        template<typename ret>
        auto submit(
            const std::function<ret()>& task,
            const std::function<void(const ret&)>& callback = std::function<void(const ret&)>()
        ) -> std::future<ret> {
            auto pmis = std::make_unique<lake_mission<ret>>(task, callback);
            auto fut = pmis->get_future();
            thread_pool<queue_t<mission>>::submit(std::move(pmis));
            return fut;
        }

        // 提交函数和参数，此方法不支持回调
        template<typename F, typename ... Args>
        requires std::invocable<F,Args...>
        auto submit(F&& f, Args&& ... args)
            -> std::future<std::invoke_result_t<F, Args...>> 
        {
            using ret = std::invoke_result_t<F, Args...>;
            return submit<ret>(
                std::function<ret()>(std::bind(std::forward<F>(f), std::forward<Args>(args)...))
            );
        }
    };

    using ThreadLake = thread_lake<basic_blockable_queue>;
}




/*************** 合并自 archivist_interfaces.h ***************/
// #pragma once
// #include "pch.h"
// #include "framework.h"

// #include "lib_exceptions.h"

namespace HYDRA15::Union::archivist
{
    /**************************** 基 础 ****************************/
    // 内部数据类型
    using ID = uint64_t;    // 主键
    using BYTE = uint8_t;   // 字节

    // 字段数据类型
    using NOTHING = std::monostate;
    using INT = int64_t;
    using FLOAT = double;
    using INTS = std::vector<INT>;
    using FLOATS = std::vector<FLOAT>;
    using BYTES = std::vector<BYTE>;

    // 版本号
    using version_id = ID;

    // 字段
    using field = std::variant<NOTHING, INT, FLOAT, INTS, FLOATS, BYTES>;

    // 字段信息
    struct field_spec
    {
        // 字段类型枚举
        enum class field_type :char
        {
            NOTHING = 0, INT = 'I', FLOAT = 'F',
            INTS = 'U', FLOATS = 'D', BYTES = 'B'
        };

        std::string name{};
        std::string comment{};
        field_type type = field_type::NOTHING;
        uint8_t mark[7]{};
        bool operator==(const field_spec& oth) const { return name == oth.name; }
        operator std::string() const noexcept { return name; }
    };
    using field_specs = std::vector<field_spec>;

    

    
    /**************************** 持久层 ****************************/
    /*
    * 持久层：负责数据存储，提供表加载相关接口，定义数据在硬盘或其他介质中的布局
    * 分页加载：
    *   仅针对表的数据部分。
    *   页被定义为连续的一部分记录，大小（以记录数计）在加载时确定
    * 索引：
    *   索引在传递时只传递记录的 ID，ID排序按照索引列增序排列
    *   索引一次性加载，不分页
    */

    // 表页
    struct page
    {
        ID id;      // 页号
        ID start;   // 页中首条记录的 ID
        ID count;   // 页内有效记录条数
        std::unordered_set<ID> modified; // 修改过的记录列表
        std::deque<field> data;
    };

    // 索引
    struct index_spec
    {
        std::string name;       // 索引表名
        std::string comment;    // 索引注释
        field_specs fieldSpecs; // 索引列

        bool operator==(const field_spec& oth) const { return name == oth.name; }
        operator std::string() const noexcept { return name; }
    };
    using index_specs = std::vector<index_spec>;

    struct index
    {
        index_spec spec;
        std::deque<ID> data;   // 按照索引顺序排列的记录 ID 数据
    };
    
    class loader
    {
    public:
        virtual ~loader() = default;

        // 信息相关
        virtual std::pair<ID, ID> version() const = 0;  // 返回上层table的版本号
        virtual size_t size() const = 0;    // 返回完整的数据大小
        virtual ID tab_size() const = 0;    // 返回表行数
        virtual ID page_size() const = 0;   // 返回页大小（以记录数计）
        virtual field_specs fields() const = 0; // 返回完整的字段表
        virtual void clear() = 0;            // 清空所有数据, 包括表数据和索引数据

        // 表数据相关
        virtual page rows(ID) const = 0;    // 返回包含指定页号的页
        virtual void rows(const page&) = 0; // 写入整页数据

        // 索引相关
        virtual index_specs indexies() const = 0;               // 返回完整的索引表信息
        virtual void index_tab(index) = 0;                      // 保存索引表（包含创建）
        virtual index index_tab(const std::string&) const = 0;  // 加载索引表
    };


    /**************************** 数据层 ****************************/
    /*
    * 数据层：负责数据组织，提供数据表操作相关接口，定义数据在内存中的布局
    * 条目 entry ：
    *   逻辑上即一行记录。
    *   技术上应当类似迭代器，持有一行数据的引用，用户应当可以通过条目对象直接操作表中实际的数据
    * 事件 incident ：
    *   二进制格式封装的增删改查操作，避免接口频繁调用造成的性能浪费，也方便表内部算法优化
    *   执行一系列事件（incidents）时，要求下一个事件都在上一个事件的结果的基础上执行。
    *   如 查 - 查 - 改 系列事件，第一次查询获得结果集 A ，第二次查询在 A 的基础上筛选获得结果集 B ，最后只修改 B 中的记录
    *   执行到分隔事件时，将当前结果暂存，然后以全表为基础执行后续的事件；
    *   执行到联合事件时，将当前结果和暂存的结果结合，然后后续事件在结合的基础上执行
    * 表迭代器和 range ：
    *   数据表支持迭代器，迭代器即 entry 对象本身。
    *   使用迭代器，配合 stl range 相关算法，实现对整表结果的筛选、修改
    */

    class entry
    {
    public:
        virtual ~entry() = default;
        virtual std::unique_ptr<entry> clone() = 0;

        // 记录信息与控制
        virtual ID id() const = 0;  // 返回 记录 ID
        virtual const field_specs& fields() const = 0; // 返回完整的字段表
        virtual entry& erase() = 0; // 删除当前记录，迭代器移动到下一条有效记录

        // 获取、写入记录项
        virtual const field& at(const std::string&) const = 0;       // 返回 指定字段的数据
        virtual entry& set(const std::string&, const field&) = 0;   // 写入指定字段

        // 将数据行对象直接作为迭代器使用，需要支持迭代器的操作
        virtual entry& operator++() = 0;
        virtual entry& operator--() = 0;
        virtual const entry& operator++() const = 0;
        virtual const entry& operator--() const = 0;
        virtual std::strong_ordering operator<=>(const entry&) const = 0;
        virtual bool operator==(const entry&) const = 0;

        // 行锁
        virtual void lock() = 0;
        virtual void unlock() = 0;
        virtual bool try_lock() = 0;
        virtual void lock_shared() const = 0;
        virtual void unlock_shared() const = 0;
        virtual bool try_lock_shared() const = 0;

    public:     // 扩展功能
        std::string str() const
        {
            std::stringstream ss;
            ss << std::format(" {:8X} |", id());
            for (const auto& field : fields())
            {
                switch (field.type)
                {
                case field_spec::field_type::INT:
                    ss << std::format(" {:8X} |", std::get<INT>(at(field)));
                    break;
                case field_spec::field_type::FLOAT:
                    ss << std::format(" {:6.2f} |", std::get<FLOAT>(at(field)));
                    break;
                case field_spec::field_type::INTS:
                    ss << std::format(" {:8X} items |", std::get<INTS>(at(field)).size());
                    break;
                case field_spec::field_type::FLOATS:
                    ss << std::format(" {:8X} items |", std::get<FLOATS>(at(field)).size());
                    break;
                case field_spec::field_type::BYTES:
                    // 检查是否为可打印字符串
                {
                    const auto& bdata = std::get<BYTES>(at(field));
                    bool isPrintable = true;
                    for (const auto& b : bdata)
                        if (b < 32 || b > 126)
                        {
                            isPrintable = false;
                            break;
                        }
                    if (isPrintable)
                    {
                        std::string s(bdata.begin(), bdata.end());
                        ss << std::format(" {:8} |", s);
                    }
                    else
                        ss << std::format(" {:8X} bytes |", bdata.size());
                }
                break;
                default:
                    ss << std::format(" {:10} |", "");
                    break;
                }
            }
        }

        std::string header_str() const
        {
            std::stringstream ss;
            ss << "   RecordID |";
            for (const auto& field : fields())
            {
                ss << std::format(" {:8} |", field.name);
            }
            return ss.str();
        }

        static bool compare_field_ls(const field& l, const field& r, const field_spec& spec)
        {
            // NOTHING 总是最小的
            if (std::holds_alternative<NOTHING>(l) && std::holds_alternative<NOTHING>(r))return false;
            if (std::holds_alternative<NOTHING>(l))return true;
            if (std::holds_alternative<NOTHING>(r))return false;

            switch (spec.type)
            {
            case field_spec::field_type::INT:
                return std::get<INT>(l) < std::get<INT>(r);
            case field_spec::field_type::FLOAT:
                return std::get<FLOAT>(l) < std::get<FLOAT>(r);
            case field_spec::field_type::INTS:
                return std::get<INTS>(l) < std::get<INTS>(r);
            case field_spec::field_type::FLOATS:
                return std::get<FLOATS>(l) < std::get<FLOATS>(r);
            case field_spec::field_type::BYTES:
                return std::get<BYTES>(l) < std::get<BYTES>(r);
            default:
                return false;
            }
        }

        static bool compare_field_eq(const field& l, const field& r, const field_spec& spec)
        {
            // NOTHING 总是最小的
            if (std::holds_alternative<NOTHING>(l) && std::holds_alternative<NOTHING>(r))return true;
            if (std::holds_alternative<NOTHING>(l) || std::holds_alternative<NOTHING>(r))return false;

            switch (spec.type)
            {
            case field_spec::field_type::INT:
                return std::get<INT>(l) == std::get<INT>(r);
            case field_spec::field_type::FLOAT:
                return std::get<FLOAT>(l) == std::get<FLOAT>(r);
            case field_spec::field_type::INTS:
                return std::get<INTS>(l) == std::get<INTS>(r);
            case field_spec::field_type::FLOATS:
                return std::get<FLOATS>(l) == std::get<FLOATS>(r);
            case field_spec::field_type::BYTES:
                return std::get<BYTES>(l) == std::get<BYTES>(r);
            default:
                return true;
            }
        }

        static bool compare_ls(const entry& l, const entry& r, const field_specs& specs)
        {
            for (const auto& spec : specs)
                if (compare_field_eq(l.at(spec), r.at(spec), spec))
                    continue;
                else return compare_field_ls(l.at(spec), r.at(spec), spec);
            return false;
        }

        static bool compare_eq(const entry& l, const entry& r, const field_specs& specs)
        {
            for (const auto& spec : specs)
                if (!compare_field_eq(l.at(spec), r.at(spec), spec))
                    return false;
            return true;
        }
    };

    struct incident
    {
        // 事件类型
        enum class incident_type { 
            nothing, separate, join, ord_lmt,  // 无操作，分割，联合，排序并限制条数
            create, drop, modify, search       // 增，删，改，查
        };
        // 事件参数
        struct condition_param  // 条件参数
        {
            enum class condition_type { nothing, equal, dequal, less, greater };    // 均为左闭右开
            std::string targetField;// 目标字段
            condition_type type;    // 条件类型：等于，不等于，小于，大于
            field reference;        // 参考值
        };
        struct modify_param     // 修改参数
        {
            std::string targetField;//目标字段
            field value;            // 目标值
        };
        struct ord_lmt_param    // 排序限制参数
        {
            field_specs targetFields;   // 目标字段
            bool increaseSeq;           // 是否增序
            ID limit;                   // 限制条数
        };
        using incident_param = std::variant<
            std::monostate,         // 用于无操作
            std::shared_ptr<entry>, // 用于 增
            condition_param,        // 用于 查
            modify_param,           // 用于 改
            ord_lmt_param           // 用于排序
        >;

        incident_type type;
        incident_param param;
    };
    using incidents = std::queue<incident>;

    class table
    {
    public:
        virtual ~table() = default;

        // 表信息与控制
        virtual ID size() const = 0;        // 返回 记录条数
        virtual ID trim() = 0;              // 优化表记录，返回优化后的记录数
        virtual const field_specs& fields() const = 0;                     // 返回完整的字段表
        virtual const field_spec& get_field(const std::string&) const = 0; // 通过字段名获取指定的字段信息
        virtual const field_specs system_fields() const = 0;               // 返回系统字段表（如有）

        // 行访问接口
        virtual std::unique_ptr<entry> create() = 0;                            // 增：创建一条记录，返回相关的条目对象
        virtual void drop(std::unique_ptr<entry>) = 0;                          // 删：通过条目对象删除记录
        virtual std::unique_ptr<entry> at(ID id) = 0;                           // 通过行号访问记录
        virtual std::list<ID> at(const std::function<bool(const entry&)>&) = 0; // 改、查：通过过滤器查找记录
        virtual std::list<ID> excute(const incidents&) = 0;                     // 执行一系列事件，按照 ID 顺序返回执行结果

        // 索引接口
        virtual void create_index(const std::string&, const field_specs&) = 0; // 创建指定名称、基于指定字段的索引
        virtual void drop_index(const std::string&) = 0;                // 删除索引
        
    private:// 由派生类实现：返回指向首条记录的迭代器 / 尾后迭代器
        virtual std::unique_ptr<entry> begin_impl() = 0;
        virtual std::unique_ptr<entry> end_impl() = 0;

    public:     // 迭代器接口
        // 说明：iterator 封装了具体的 std::unique_ptr<entry>，并实现了前向迭代器所需的操作。
        class iterator
        {
        public:
            using iterator_category = std::forward_iterator_tag;
            using difference_type = std::ptrdiff_t;

        private:
            const std::unique_ptr<entry> pe;

        public:
            entry& operator*() const { return *pe; }
            entry* operator->() const { return pe.get(); }

            iterator& operator++() { ++(*pe); return *this; }
            iterator operator++(int) { iterator tmp(*this); ++(*pe); return tmp; }

            friend bool operator==(const iterator& a, const iterator& b) { return *a.pe == *b.pe; }
            friend bool operator!=(const iterator& a, const iterator& b) { return !(a == b); }
            friend std::strong_ordering operator<=>(const iterator& a, const iterator& b) { return *a.pe <=> *b.pe; }

        private:
            friend class table;
            iterator(std::unique_ptr<entry> e) : pe(std::move(e)) {
                if (!pe)throw exceptions::common("Cannot create iterator with null entry");
            }
            iterator(const iterator& oth) :pe(oth.pe->clone()) {}
        };
        
        iterator begin() { return iterator(begin_impl()); }
        iterator end() { return iterator(end_impl()); }
    };

}



/*************** 合并自 datetime.h ***************/
// #pragma once
// #include "framework.h"
// #include "pch.h"

// #include "lib_exceptions.h"

namespace HYDRA15::Union::assistant
{
    // 存储时间、提供时间和日期的格式化输出
    // 仅用于日期和时间的输出，不用于精确计时
    constexpr int localTimeZone = 8; // 本地时区，默认为东八区
    class datetime
    {
        // 记录的时间
        time_t stamp;

        // 构造和析构
    public:
        datetime() :stamp(std::time(NULL)){}
        datetime(time_t t) :stamp(t){}
        datetime(const datetime&) = default;
        datetime& operator=(const datetime&) = default;
        ~datetime() = default;

        // 输出
    public:
        std::string date_time(
            std::string format = "%Y-%m-%d %H:%M:%S",
            int timeZone = localTimeZone
        ) const {
            if (timeZone < -12 || timeZone > 14)
                throw exceptions::common::BadParameter("timeZone", std::to_string(timeZone), "-12 < timeZone < 14");

            time_t localStamp = stamp + timeZone * 3600LL;
            tm local;
            gmtime_s(&local, &localStamp);
            std::string str;
            str.resize(format.size() * 2 + 20, '\0');
            size_t len = strftime(str.data(), str.size(), format.data(), &local);
            str.resize(len);
            return str;
        }

        // 静态工具函数
    public:
        static datetime now(){ return datetime(); }
        static std::string now_date_time(
            std::string format = "%Y-%m-%d %H:%M:%S", 
            int timeZone = localTimeZone
        ){
            return datetime().date_time(format, timeZone);
        }
    };
}



/*************** 合并自 fstreams.h ***************/
// #pragma once
// #include "pch.h"
// #include "framework.h"

// #include "lib_exceptions.h"
// #include "background.h"
// #include "shared_containers.h"
// #include "utilities.h"



namespace HYDRA15::Union::assistant
{
    // 管理、读写二进制文件
    // 读写任意类型数据数组、修改文件大小、自动处理部分流错误
    // 创建即绑定文件路径
    class bfstream
    {
    private:
        const std::filesystem::path path;
        mutable std::fstream file;
        bool newFile = false;

    private:
        // 工具函数
        void create()
        {
            std::filesystem::path parentPath = path.parent_path();

            // 创建指定路径
            if (!parentPath.empty() && !std::filesystem::exists(parentPath))
                std::filesystem::create_directories(parentPath);

            // 检查并创建文件
            if (std::filesystem::exists(path))
                return;

            std::ofstream ofs(path);
            if (!ofs)
                throw exceptions::files::FileIOFlowError(path, ofs.rdstate());

            newFile = true;
        }

        void open()
        {
            file.open(path, std::ios::in | std::ios::out | std::ios::binary);
            if (!file.is_open())
                throw exceptions::files::FileIOFlowError(path, file.rdstate());
            file.exceptions(std::ios::badbit);
        }

        void close() { file.close(); }

    public:
        // 任意类型读写
        template<typename T>
            requires std::is_trivial_v<T>
        std::vector<T> read(size_t pos, size_t count) const
        {
            std::vector<T> res(count, T{});
            file.sync();
            file.seekg(pos);
            file.read(reinterpret_cast<char*>(res.data()), sizeof(T) * count);
            if (!file)
                if (file.rdstate() & std::ios::eofbit)
                {
                    file.clear();
                    res.resize(file.gcount() / sizeof(T));
                }
                else throw exceptions::files::FileIOFlowError(path, file.rdstate());
            return res;
        }

        template<typename T>
            requires std::is_trivial_v<T>
        void write(size_t pos, const std::vector<T>& data)
        {
            file.seekp(pos);
            file.write(reinterpret_cast<const char*>(data.data()), sizeof(T) * data.size());
            if (!file)
                throw exceptions::files::FileIOFlowError(path, file.rdstate());
        }

        // 兼容任意类型指针读写
        template<typename T>
            requires std::is_trivial_v<T>
        size_t read(size_t pos, size_t count, T* data) const    // 返回实际读取的数据数量
        {
            file.sync();
            file.seekg(pos);
            file.read(reinterpret_cast<char*>(data), sizeof(T) * count);
            if (!file)
                if (file.rdstate() & std::ios::eofbit)file.clear();
                else throw exceptions::files::FileIOFlowError(path, file.rdstate());
            return file.gcount() / sizeof(T);
        }

        template<typename T>
            requires std::is_trivial_v<T>
        void write(size_t pos, size_t count, const T* data)
        {
            file.seekp(pos);
            file.write(reinterpret_cast<const char*>(data), sizeof(T) * count);
            if (!file)
                throw exceptions::files::FileIOFlowError(path, file.rdstate());
        }

        // 读字符串
        std::string read_string(size_t pos)
        {
            file.sync();
            file.seekg(pos);

            std::string result;
            char ch;

            while (file.get(ch))
            {
                if (ch == '\0') // 遇到结束符
                    return result;
                result += ch;
            }

            // 如果到达这里，说明文件结束但未遇到结束符
            if (file.eof())
                throw exceptions::files::ContentNotFound(path)
                .set("content type", "string")
                .set("position", std::to_string(pos));

            // 如果是其他错误
            throw exceptions::files::FileIOFlowError(path, file.rdstate());
        }

    public:     // 信息和管理接口
        void resize(size_t size)    // 修改文件大小
        {
            file.close();
            std::filesystem::resize_file(path, size);
            file.open(path);
        }

        size_t size() const { file.seekg(0, std::ios::end); return file.tellg(); }

        bool is_new() const { return newFile; }

        std::filesystem::path file_path() const { return path; }

        std::fstream& data() { return file; }

        void reopen()
        {
            if (file.is_open())close();
            if (!std::filesystem::exists(path))create(); 
            open();
        }

        operator bool() const { return file.operator bool(); }


        // 只允许从文件名构造
        bfstream() = delete;
        bfstream(const bfstream& oth) :bfstream(oth.path) {}
        bfstream(bfstream&&) noexcept = default;
        bfstream(const std::filesystem::path& p) :path(p) { if (!std::filesystem::exists(path))create(); open(); }
    };


    // 段化文件
    // 多路 IO、批量段读写，依赖 OS 缓存
    // 创建即绑定文件、段大小
    class bsfstream
    {
    private:    
        class async_io_thread_pool : protected labourer::background
        {
        private:
            const size_t segSize;
            const std::filesystem::path path;
            std::atomic<bool> working = true;
            std::exception_ptr gexptr;
            std::mutex gexptrMtx;

        private:
            labourer::basic_blockable_queue<std::packaged_task<std::vector<byte>(bfstream&)>> queue;

        private:
            virtual void work() noexcept override
            {
                try
                {
                    bfstream bfs{ path };
                    while (working.load(std::memory_order_relaxed) || !queue.empty())
                    {
                        auto tsk = queue.pop();
                        if (tsk.valid())tsk(bfs);
                        if (!bfs)bfs.reopen();
                    }
                }
                catch (...) { std::unique_lock ul{ gexptrMtx }; gexptr = std::current_exception(); }
            }

        public:
            std::future<std::vector<byte>> submit(const std::function<std::vector<byte>(bfstream&)>& mis)
            {
                auto tsk = std::packaged_task<std::vector<byte>(bfstream&)>(mis);
                auto fut = tsk.get_future();
                queue.push(std::move(tsk));
                return fut;
            }

        public:
            void start() { background::start(); }

        public:
            async_io_thread_pool(const std::filesystem::path& path, size_t segSize, unsigned int thrs)
                :path(path), segSize(segSize), background(thrs) {
            }

            ~async_io_thread_pool() { working = false; queue.notify_exit(); background::wait_for_end(); }
            
        };

    private:
        const size_t segSize;
        bfstream bfs;
        mutable std::shared_mutex smtx;
        std::unique_ptr<async_io_thread_pool> aioPool;
        // 专为拷贝存储的数据
        const unsigned int aioThreads;

    public:
        // 单段读写，不启用异步 IO，不支持并发
        template<typename T>
            requires std::is_trivial_v<T>
        std::vector<T> read(size_t segID, size_t pos, size_t count) const
        {
            if (pos + count * sizeof(T) > segSize)
                throw exceptions::files::ExceedRage(bfs.file_path(), "pos, count", "pos + count * sizeof(T) < segSize");
            std::shared_lock sl{ smtx };
            return bfs.read<T>(segID * segSize + pos, count);
        }

        template<typename T>
            requires std::is_trivial_v<T>
        void write(size_t segID, size_t pos, const std::vector<T>& data)
        {
            if (data.empty())return;
            if (pos + data.size() * sizeof(T) > segSize)
                throw exceptions::files::ExceedRage(bfs.file_path(), "pos, data.size", "pos + data.size <= segSize");
            std::unique_lock ul{ smtx };
            bfs.write<T>(segID * segSize + pos, data);
        }

        // 多段异步读写，根据配置启用异步 IO，支持并发
        // 读写模型：将 list 中的所有节视为一个整体，在整体中读写数据
        template<typename T>
            requires std::is_trivial_v<T>
        std::vector<T> read(const std::deque<size_t>& segIDs, size_t pos, size_t count) const
        {
            const size_t dataByteSize = count * sizeof(T);
            std::deque<size_t> seglst = segIDs;

            if (pos + dataByteSize > segIDs.size() * segSize)
                throw exceptions::files::ExceedRage(bfs.file_path(), "segIDs, pos, count", "pos + count * sizeof(T) <= segSize * segIDs.size");

            // 处理头尾冗余的节
            for (size_t i = 0; i < pos / segSize; i++)seglst.pop_front();
            pos -= (pos / segSize) * segSize;
            for (size_t i = seglst.size(); i > ((pos + dataByteSize) - 1) / segSize + 1; i--)seglst.pop_back();

            std::queue<std::future<std::vector<byte>>> futures;
            std::vector<T> res(count, T{});

            for (size_t i = 0; i < seglst.size(); i++)
            {
                const size_t id = seglst[i];
                const size_t currentTgtPos = std::min(dataByteSize, i > 0 ? i * segSize - pos : 0);
                const size_t remainTgtSize = dataByteSize - currentTgtPos;
                const size_t currentSrcPos = i > 0 ? 0 : pos;
                const size_t currentSrcSize = i > 0 ? std::min(remainTgtSize, segSize) : std::min(segSize - pos, remainTgtSize);

                if (remainTgtSize == 0)continue;

                if (aioPool)
                    futures.push(aioPool->submit(
                        [currentSrcPos, currentSrcSize, id, this](bfstream& bfs)->std::vector<byte> {
                            return bfs.read<byte>(id * segSize + currentSrcPos, currentSrcSize);
                        }
                    ));
                else
                {
                    std::vector<byte> segData = read<byte>(id, currentSrcPos, currentSrcSize);
                    assistant::memcpy(segData.data(), reinterpret_cast<byte*>(res.data()) + currentTgtPos, segData.size());
                }
            }

            if (!futures.empty())
            {
                for (size_t i = 0; i < seglst.size(); i++)
                {
                    const size_t id = seglst[i];
                    const size_t currentTgtPos = std::min(dataByteSize, i > 0 ? i * segSize - pos : 0);
                    const size_t remainTgtSize = dataByteSize - currentTgtPos;

                    std::vector<byte> segData = futures.front().get();
                    assistant::memcpy(segData.data(), reinterpret_cast<byte*>(res.data()) + currentTgtPos, segData.size());
                    futures.pop();
                }
            }

            return res;
        }

        template<typename T>
            requires std::is_trivial_v<T>
        void write(const std::deque<size_t>& segIDs, size_t pos, const std::vector<T>& data, bool sync = true)
        {
            const size_t dataByteSize = data.size() * sizeof(T);
            std::deque<size_t> seglst = segIDs;

            if (data.empty())return;
            if (pos + dataByteSize > segIDs.size() * segSize)
                throw exceptions::files::ExceedRage(bfs.file_path(), "segIDs, pos, data.bytesize", "pos + data.bytesize <= segSize * segIDs.size");

            // 处理头尾冗余的节
            for (size_t i = 0; i < pos / segSize; i++)seglst.pop_front();
            pos -= (pos / segSize) * segSize;
            for (size_t i = seglst.size(); i > ((pos + dataByteSize) - 1) / segSize + 1; i--)seglst.pop_back();

            std::queue<std::future<std::vector<byte>>> futures;
            for (size_t i = 0; i < seglst.size(); i++)
            {
                const size_t id = seglst[i];
                const size_t currentSrcPos = std::min(dataByteSize, i > 0 ? i * segSize - pos : 0);
                const size_t remainSrcSize = dataByteSize - currentSrcPos;
                const size_t currentTgtPos = i > 0 ? 0 : pos;
                const size_t currentTgtSize = i > 0 ? std::min(remainSrcSize, segSize) : std::min(remainSrcSize, segSize - currentTgtPos);

                if (remainSrcSize == 0)continue;    // 没有更多数据

                std::vector<byte> segData(currentTgtSize, 0);
                assistant::memcpy(
                    reinterpret_cast<const byte*>(data.data()) + currentSrcPos,
                    segData.data(),
                    segData.size()
                );
                if (aioPool)   // 未启用异步IO，退化为同步操作
                    futures.push(aioPool->submit(
                        [id, currentTgtPos, segData = std::move(segData), this](bfstream& bfs) -> std::vector<byte>
                        {
                            bfs.write<byte>(id * segSize + currentTgtPos, segData);
                            return {};
                        }
                    ));
                else
                    write<byte>(id, currentTgtPos, segData);
            }

            if (sync)  // 同步写入，传递异常、保证写入完成
                while (!futures.empty())
                {
                    futures.front().get();
                    futures.pop();
                }
        }

        // 读字符串
        std::string read_string(size_t segID, size_t pos) const
        {
            std::vector<char> segData = read<char>(segID, 0, segSize);
            if (pos >= segData.size())
                throw exceptions::files::ExceedRage(bfs.file_path(), "pos", "pos <= segData.size");

            std::shared_lock sl{ smtx };

            std::string result;
            size_t currentPos = pos;


            for (size_t i = 0; i < segData.size() - pos; i++)
                if (segData[pos + i] == '\0')
                    return std::string{ segData.data() + pos,i };

            // 如果到达这里，说明节结束但未遇到结束符
            throw exceptions::files::ContentNotFound(bfs.file_path())
                .set("content type", "string")
                .set("position", std::to_string(pos))
                .set("segment ID", std::to_string(segID));
        }

        std::string read_string(const std::deque<size_t>& segIDs, size_t pos) const
        {
            std::deque<size_t> seglst = segIDs;
            if (pos >= segIDs.size() * segSize)
                throw exceptions::files::ExceedRage(bfs.file_path(), "segIDs, pos", "pos < segSize * segIDs.size");

            // 处理头尾冗余的节
            for (size_t i = 0; i < pos / segSize; i++)seglst.pop_front();
            pos -= (pos / segSize) * segSize;

            std::string result;
            for (size_t i = 0; i < seglst.size(); i++)
            {
                const size_t id = seglst[i];
                const size_t currentSrcPos = i > 0 ? 0 : pos;
                const size_t currentSrcSize = segSize - currentSrcPos;
                std::vector<char> segData = read<char>(id, currentSrcPos, currentSrcSize);
                for (size_t j = 0; j < segData.size(); j++)
                    if (segData[j] == '\0')
                        return result + std::string{ segData.data(),j };
                result += std::string{ segData.data(),segData.size() };
            }

            // 如果到达这里，说明节结束但未遇到结束符
            throw exceptions::files::ContentNotFound(bfs.file_path())
                .set("content type", "string")
                .set("position", std::to_string(pos))
                .set("segment IDs", assistant::container_to_string(segIDs));
        }

    public:     // 管理接口
        size_t seg_size() const { return segSize; }

        size_t seg_count() const { std::shared_lock sl{ smtx }; return bfs.size() == 0 ? 0 : (bfs.size() - 1) / segSize + 1; }

        bfstream& data() { return bfs; }

        const bfstream& data() const { return bfs; }

    public:
        bsfstream(const std::filesystem::path& path, size_t segSize, unsigned int aioThrs = 4)
            :bfs(path), segSize(segSize), aioPool(aioThrs > 0 ? std::make_unique<async_io_thread_pool>(path, segSize, aioThrs) : nullptr),
            aioThreads(aioThrs)
        {
            if (aioPool)aioPool->start();
        }

        bsfstream(const bfstream& bfs, size_t segSize, unsigned int aioThrs = 4)
            :bfs(bfs), segSize(segSize), aioPool(aioThrs > 0 ? std::make_unique<async_io_thread_pool>(bfs.file_path(), segSize, aioThrs) : nullptr),
            aioThreads(aioThrs)
        {
            if (aioPool)aioPool->start();
        }

        bsfstream(const bsfstream& oth) :bsfstream(oth.bfs.file_path(), oth.segSize, oth.aioThreads) {}

        bsfstream(bsfstream&&) noexcept = delete;
    };
}



/*************** 合并自 simple_memory_table.h ***************/
// #pragma once
// #include "pch.h"
// #include "framework.h"

// #include "archivist_interfaces.h"

// #include "iMutexies.h"
// #include "utilities.h"
// #include "lib_exceptions.h"
// #include "shared_containers.h"

namespace HYDRA15::Union::archivist
{
    /***************************** 设 计 ****************************
    *
    * 始终在初始化阶段尝试将所有数据加载到内存中，如果内存不足则会收到系统异常
    * 使用单字段索引加速查询，加速查询仅在 excute 接口中有效
    * 
    * 查询优先：有迭代器（entry）存在时可能会阻塞插入操作
    * 
    * 构造要求：
    *   构造一个 loader ，其包含的 version == simple_memort_table::version，
    *   并且其第一个字段为 simple_memory_table::sysfldRowMark
    *   从此 loader 构造 simple_memory_table 对象
    * 
    * 系统字段：
    *   $RowMark：位图模式的行标记
    *       0x1 无效记录标记 当此字段没有数据时默认为此标记
    *       0x2 已删除标记
    * 
    * ****************************************************************/
    class simple_memory_table : public table
    {
    public:
        // 表关联的 entry 容器，拥有迭代和修改数据能力，但是可能会影响全表性能，应限制作用域
        class entry_impl : public entry
        {
        private:
            simple_memory_table& tableRef;
            mutable std::optional<std::shared_lock<labourer::atomic_shared_mutex>> sl;
            mutable ID rowID;

            bool locked = false;
            mutable bool lockShared = false;

        private:
            void transport(ID target) const // 移动到指定行，同时转移锁状态
            {
                if (locked)tableRef.rowMtxs[rowID].unlock();
                if (lockShared)tableRef.rowMtxs[rowID].unlock_shared();
                rowID = target;
                if (locked)tableRef.rowMtxs[rowID].lock();
                if (lockShared)tableRef.rowMtxs[rowID].lock_shared();
            }

            INT get_row_mark() const
            {
                if (auto pint = std::get_if<INT>(&tableRef.tabData[rowID * tableRef.fieldTab.size() + tableRef.fieldNameTab.at(tableRef.sysfldRowMark)]))
                    return *pint;
                return simple_memory_table::row_bit_mark::invalid_bit;
            }

            void init() const { if (!sl)sl.emplace(tableRef.tableMtx); }    // 延迟锁表到首次访问时

        public:     // 系统接口
            virtual ~entry_impl() = default;
            virtual std::unique_ptr<entry> clone() override
            {
                return std::make_unique<entry_impl>(tableRef, rowID);
            }

            // 记录信息
            virtual ID id() const override { return rowID; };  // 返回 记录 ID

            virtual const field_specs& fields() const override { return tableRef.fieldTab; } // 返回完整的字段表

            virtual entry& erase() // 删除当前记录，迭代器移动到下一条有效记录
            {
                init();
                if (!(get_row_mark() & (simple_memory_table::row_bit_mark::deleted_bit | simple_memory_table::row_bit_mark::deleted_bit)))
                {
                    std::get<INT>(tableRef.tabData[rowID * tableRef.fieldTab.size() + tableRef.fieldNameTab.at(sysfldRowMark)])
                        |= simple_memory_table::row_bit_mark::deleted_bit;
                    tableRef.modifiedRows.insert(rowID);
                    tableRef.update_index(rowID);
                }
                operator++();
                return *this;
            }

            // 获取、写入记录项
            virtual const field& at(const std::string& fieldName) const override  // 返回 指定字段的数据
            {
                init();
                if (get_row_mark() & (simple_memory_table::row_bit_mark::deleted_bit | simple_memory_table::row_bit_mark::deleted_bit))
                    throw exceptions::common("Record id invalid");
                return tableRef.tabData[rowID * tableRef.fieldTab.size() + tableRef.fieldNameTab.at(fieldName)];
            }

            virtual entry& set(const std::string& fieldName, const field& data) override   // 写入指定字段
            {   
                init();
                if (get_row_mark() & (simple_memory_table::row_bit_mark::deleted_bit | simple_memory_table::row_bit_mark::deleted_bit))
                    throw exceptions::common("Writing data to a invalid row is not allowed");
                tableRef.tabData[rowID * tableRef.fieldTab.size() + tableRef.fieldNameTab.at(fieldName)] = data;
                tableRef.modifiedRows.insert(rowID);
                tableRef.update_index(rowID);
                return *this;
            }

            // 将数据行对象直接作为迭代器使用，需要支持迭代器的操作
            virtual entry& operator++() override    // 滑动到下一条有效记录
            { 
                init();
                ID currentRecordCount = tableRef.recordCount.load(std::memory_order::relaxed);
                if (rowID < currentRecordCount)
                    transport(rowID + 1);
                while (rowID < currentRecordCount &&
                    (get_row_mark() & (simple_memory_table::row_bit_mark::deleted_bit | simple_memory_table::row_bit_mark::invalid_bit)))
                    transport(rowID + 1);
                if (rowID >= currentRecordCount)
                    rowID = tableRef.end()->id();
                return *this; 
            }

            virtual entry& operator--() override // 滑动到上一条有效记录
            { 
                init();
                if (rowID > 0)
                    transport(rowID - 1);
                while (rowID > 0 &&
                    (get_row_mark() & (simple_memory_table::row_bit_mark::deleted_bit | simple_memory_table::row_bit_mark::invalid_bit)))
                    transport(rowID - 1);
                if (rowID == 0 &&
                    (get_row_mark() & (simple_memory_table::row_bit_mark::deleted_bit | simple_memory_table::row_bit_mark::invalid_bit)))
                    rowID = tableRef.end()->id();
                return *this; 
            }

            virtual const entry& operator++() const override
            {
                init();
                ID currentRecordCount = tableRef.recordCount.load(std::memory_order::relaxed);
                if (rowID < currentRecordCount)
                    transport(rowID + 1);
                while (rowID < currentRecordCount &&
                    (get_row_mark() & (simple_memory_table::row_bit_mark::deleted_bit | simple_memory_table::row_bit_mark::invalid_bit)))
                    transport(rowID + 1);
                if (rowID >= currentRecordCount)
                    rowID = tableRef.end()->id();
                return *this;
            }

            virtual const entry& operator--() const override
            {
                init();
                if (rowID > 0)
                    transport(rowID - 1);
                while (rowID > 0 &&
                    (get_row_mark() & (simple_memory_table::row_bit_mark::deleted_bit | simple_memory_table::row_bit_mark::invalid_bit)))
                    transport(rowID - 1);
                if (rowID == 0 &&
                    (get_row_mark() & (simple_memory_table::row_bit_mark::deleted_bit | simple_memory_table::row_bit_mark::invalid_bit)))
                    rowID = tableRef.end()->id();
                return *this;
            }

            virtual std::strong_ordering operator<=>(const entry& oth) const override
            {
                ID othID = oth.id();
                if(rowID < othID)return std::strong_ordering::less;
                else if(rowID > othID)return std::strong_ordering::greater;
                else return std::strong_ordering::equal;
            }

            virtual bool operator==(const entry& oth) const override { return rowID == oth.id(); }

        public:     // 行锁
            virtual void lock() override { init(); tableRef.rowMtxs[rowID].lock(); locked = true; }

            virtual void unlock() override { init(); tableRef.rowMtxs[rowID].unlock(); locked = false; }

            virtual bool try_lock() override { init(); locked = tableRef.rowMtxs[rowID].try_lock(); return locked; }

            virtual void lock_shared() const override { init(); tableRef.rowMtxs[rowID].lock_shared(); lockShared = true; }

            virtual void unlock_shared() const override { init(); tableRef.rowMtxs[rowID].unlock_shared(); lockShared = false; }

            virtual bool try_lock_shared() const override { init(); lockShared = tableRef.rowMtxs[rowID].try_lock_shared(); return lockShared; }

        public:
            simple_memory_table& get_table() const { return tableRef; }

        public:
            entry_impl(simple_memory_table& tab, ID id) : tableRef(tab), rowID(id) {}
        };
        friend class entry_impl;

        // 存储数据的 entry 容器，不关联表，不能修改表中数据，不能迭代，不涉及的接口不实现
        class data_entry_impl : public entry
        {
        private:
            const std::unordered_map<std::string, size_t> fieldNameTab;

            std::vector<field> rowData;


        public:     // 系统接口
            virtual std::unique_ptr<entry> clone() override
            {
                return std::make_unique<data_entry_impl>(*this);
            }

            // 记录信息
            virtual ID id() const override { return std::numeric_limits<ID>::max(); }
            virtual const field_specs& fields() const override                      { throw exceptions::common::UnsupportedInterface("HYDRA15::Union::archivist::entry", "HYDRA15::Union::archivist::simple_memory_table::data_entry_impl", "fields"); }
            virtual entry& erase() override                                         { throw exceptions::common::UnsupportedInterface("HYDRA15::Union::archivist::entry", "HYDRA15::Union::archivist::simple_memory_table::data_entry_impl", "erase"); }

            // 获取、写入记录项
            virtual const field& at(const std::string& fieldName) const override  // 返回 指定字段的数据
            {
                return rowData[fieldNameTab.at(fieldName)];
            }

            virtual entry& set(const std::string& fieldName, const field& data) override
            {
                rowData[fieldNameTab.at(fieldName)] = data;
                return *this;
            }

            // 将数据行对象直接作为迭代器使用，需要支持迭代器的操作
            virtual entry& operator++() override                                    { throw exceptions::common::UnsupportedInterface("HYDRA15::Union::archivist::entry", "HYDRA15::Union::archivist::simple_memory_table::data_entry_impl", "operator++"); }
            virtual entry& operator--() override                                    { throw exceptions::common::UnsupportedInterface("HYDRA15::Union::archivist::entry", "HYDRA15::Union::archivist::simple_memory_table::data_entry_impl", "operator--"); }
            virtual const entry& operator++() const override                        { throw exceptions::common::UnsupportedInterface("HYDRA15::Union::archivist::entry", "HYDRA15::Union::archivist::simple_memory_table::data_entry_impl", "operator++ const"); }
            virtual const entry& operator--() const override                        { throw exceptions::common::UnsupportedInterface("HYDRA15::Union::archivist::entry", "HYDRA15::Union::archivist::simple_memory_table::data_entry_impl", "operator-- const"); }
            virtual std::strong_ordering operator<=>(const entry&) const override   { throw exceptions::common::UnsupportedInterface("HYDRA15::Union::archivist::entry", "HYDRA15::Union::archivist::simple_memory_table::data_entry_impl", "operator<=>"); }
            virtual bool operator==(const entry&) const override                    { throw exceptions::common::UnsupportedInterface("HYDRA15::Union::archivist::entry", "HYDRA15::Union::archivist::simple_memory_table::data_entry_impl", "operator=="); }

            // 行锁
            virtual void lock() override                                            { throw exceptions::common::UnsupportedInterface("HYDRA15::Union::archivist::entry", "HYDRA15::Union::archivist::simple_memory_table::data_entry_impl", "lock"); }
            virtual void unlock() override                                          { throw exceptions::common::UnsupportedInterface("HYDRA15::Union::archivist::entry", "HYDRA15::Union::archivist::simple_memory_table::data_entry_impl", "unlock"); }
            virtual bool try_lock() override                                        { throw exceptions::common::UnsupportedInterface("HYDRA15::Union::archivist::entry", "HYDRA15::Union::archivist::simple_memory_table::data_entry_impl", "try_lock"); }
            virtual void lock_shared() const override                               { throw exceptions::common::UnsupportedInterface("HYDRA15::Union::archivist::entry", "HYDRA15::Union::archivist::simple_memory_table::data_entry_impl", "lock_shared"); }
            virtual void unlock_shared() const override                             { throw exceptions::common::UnsupportedInterface("HYDRA15::Union::archivist::entry", "HYDRA15::Union::archivist::simple_memory_table::data_entry_impl", "unlock_shared"); }
            virtual bool try_lock_shared() const override                           { throw exceptions::common::UnsupportedInterface("HYDRA15::Union::archivist::entry", "HYDRA15::Union::archivist::simple_memory_table::data_entry_impl", "try_lock_shared"); }

        public:
            data_entry_impl(const simple_memory_table& tab, const std::vector<field>& data = {})
                : rowData{ data }, fieldNameTab(tab.fieldNameTab) {
                if (rowData.empty())rowData.resize(tab.fieldTab.size());
            }
            data_entry_impl(const data_entry_impl& oth)
                : rowData{ oth.rowData }, fieldNameTab(oth.fieldNameTab) {
            }
            data_entry_impl(const entry_impl& oth)
                : rowData{}, fieldNameTab(oth.get_table().fieldNameTab) {
                ID rid = oth.id();
                rowData.resize(oth.get_table().fieldTab.size());
                for (size_t i = 0; i < rowData.size(); i++)
                    rowData[i] = oth.get_table().tabData[rid * oth.get_table().fieldTab.size() + i];
            }
        };
        friend class data_entry_impl;

    private:
        // 单字段索引表
        class index_impl
        {
        private:    // 异构比较器
            class comparator
            {
            public:
                using is_transparent = void;

            private:
                const simple_memory_table& tableRef;
                const field_spec& fieldSpec;

            public:
                bool operator()(ID l, ID r) const
                {
                    const field& lfield = tableRef.tabData[l * tableRef.fieldTab.size() + tableRef.fieldNameTab.at(fieldSpec)];
                    const field& rfield = tableRef.tabData[r * tableRef.fieldTab.size() + tableRef.fieldNameTab.at(fieldSpec)];
                    return entry_impl::compare_field_ls(lfield, rfield, fieldSpec);
                }

                bool operator()(ID l, const entry& r) const 
                {
                    const field& lfield = tableRef.tabData[l * tableRef.fieldTab.size() + tableRef.fieldNameTab.at(fieldSpec)];
                    const field& rfield = r.at(fieldSpec);
                    return entry_impl::compare_field_ls(lfield, rfield, fieldSpec);
                }

                bool operator()(const entry& l, ID r) const 
                {
                    const field& lfield = l.at(fieldSpec);
                    const field& rfield = tableRef.tabData[r * tableRef.fieldTab.size() + tableRef.fieldNameTab.at(fieldSpec)];
                    return entry_impl::compare_field_ls(lfield, rfield, fieldSpec);
                }

            public:
                comparator(const simple_memory_table& tab, const field_spec& fs) : tableRef(tab), fieldSpec(fs) {}
            };

        private:
            std::multiset<ID, comparator> data;
            std::unordered_map<ID, std::multiset<ID, comparator>::iterator> id2it;
            mutable labourer::atomic_shared_mutex mtx;

        public:
            const field_spec& fieldSpec;

        public:
            void insert(ID id) { std::unique_lock ul{ mtx }; auto it = data.insert(id); id2it[id] = it; }

            void erase(ID id)
            {
                std::unique_lock ul{ mtx };
                auto it = id2it.find(id);
                if (it != id2it.end()) { data.erase(it->second); id2it.erase(it); }
                else for(auto itr = data.begin(); itr != data.end(); ++itr)
                    if (*itr == id) { data.erase(itr); break; }
            }

            std::unordered_set<ID> search(const entry& ent, const std::unordered_set<ID>& ref = {}) const
            {
                std::unordered_set<ID> result;
                std::shared_lock sl{ mtx };
                auto range = data.equal_range(ent);
                for (auto& it = range.first; it != range.second; ++it)
                    if (ref.empty()) result.insert(*it);
                    else if (ref.contains(*it)) result.insert(*it);
                return result;
            }

            std::unordered_set<ID> search_range(const entry& lower, const entry& upper, const std::unordered_set<ID>& ref = {}) const
            {
                std::unordered_set<ID> result;
                std::shared_lock sl{ mtx };
                auto range = std::make_pair(data.lower_bound(lower), data.lower_bound(upper));
                for (auto& it = range.first; it != range.second; ++it)
                    if (ref.empty()) result.insert(*it);
                    else if (ref.contains(*it)) result.insert(*it);
                return result;
            }

            std::unordered_set<ID> search_ls(const entry& upper, const std::unordered_set<ID>& ref = {}) const
            {
                std::unordered_set<ID> res;
                std::shared_lock sl{ mtx };
                auto up = data.lower_bound(upper);
                for (auto it = data.begin(); it != up; it++)
                    if (ref.empty()) res.insert(*it);
                    else if (ref.contains(*it)) res.insert(*it);
                return res;
            }

            std::unordered_set<ID> search_gr(const entry& lower, const std::unordered_set<ID>& ref = {}) const
            {
                std::unordered_set<ID> res;
                std::shared_lock sl{ mtx };
                auto lw = data.lower_bound(lower);
                for (auto it = lw; it != data.end(); it++)
                    if (ref.empty()) res.insert(*it);
                    else if (ref.contains(*it)) res.insert(*it);
                return res;
            }

            bool contains(ID id) const { std::shared_lock sl{ mtx }; return id2it.contains(id); }

            size_t size() const { std::shared_lock sl{ mtx }; return id2it.size(); }

            void clear() { std::unique_lock ul{ mtx }; data.clear(); id2it.clear(); }

            std::deque<ID> to_deque() const { std::shared_lock sl{ mtx }; return std::deque<ID>(data.begin(), data.end()); }

        public:
            bool operator==(const index_impl& oth) const { return fieldSpec == oth.fieldSpec; }
            bool operator<(const index_impl& oth) const { return fieldSpec.name < oth.fieldSpec.name; }
            bool operator==(const std::string& oth) const { return fieldSpec.name == oth; }
            bool operator<(const std::string& oth) const { return fieldSpec.name < oth; }

        public:
            index_impl(const simple_memory_table& tab, const field_spec& fs, const std::deque<ID>& dat)
                : data(comparator(tab, fs)), fieldSpec(fs) {
                for (ID id : dat)
                {
                    auto it = data.insert(data.end(), id);
                    id2it[id] = it;
                }
            }

            index_impl(const simple_memory_table& tab, const field_spec& fs)
                : data(comparator(tab, fs)), fieldSpec(fs) {
            }
        };
        friend class index_table;

    public:
        struct row_bit_mark
        {
            static constexpr INT invalid_bit = 0x1;
            static constexpr INT deleted_bit = 0x2;
        };

        static constexpr std::pair<ID, ID> version{ 0x4172634D656D74,0x0000000100000000 };  // "ArchMemt", 1.0

        static const inline field_spec sysfldRowMark{ "$RowMark","stores the state of data row",field_spec::field_type::INT };

    private:
        // 系统
        labourer::atomic_shared_mutex tableMtx;
        const std::unique_ptr<loader> loader;

        // 字段
        const field_specs fieldTab;
        const std::unordered_map<std::string, size_t> fieldNameTab;

        // 数据
        std::atomic<ID> recordCount = 0;
        std::atomic<ID> fakeRecordCount = 0;
        std::deque<field> tabData;
        mutable std::deque<std::shared_mutex> rowMtxs;
        labourer::basic_shared_set<ID> modifiedRows;

        // 索引
        std::vector<std::unique_ptr<index_impl>> indexTab;

    private:
        std::unique_ptr<entry_impl> get_entry(ID id) { return std::make_unique<entry_impl>(*this, id); }

        std::unique_ptr<const entry_impl> get_const_entry(ID id) { return std::make_unique<const entry_impl>(*this, id); }

        void update_index(ID id)
        {
            for(auto& idx : indexTab)
            {
                if (!idx)continue;
                if (idx->contains(id))
                    idx->erase(id);
                if (!std::holds_alternative<NOTHING>(tabData[id * fieldTab.size() + fieldNameTab.at(idx->fieldSpec)]))
                    idx->insert(id);
            }
        }

        void sync_all()
        {
            // 按页加载数据
            tabData.clear(); rowMtxs.clear();
            ID currentRecordCount = loader->tab_size();
            recordCount.store(currentRecordCount, std::memory_order::relaxed);
            if (currentRecordCount > 0)
            {
                ID pageSize = loader->page_size();
                ID pageCount = (currentRecordCount - 1) / pageSize + 1;
                for (ID i = 0; i < pageCount; ++i)
                    tabData.append_range(loader->rows(i).data);
                tabData.resize(assistant::power_of_2_not_less_than_n(currentRecordCount) * fieldTab.size());
                rowMtxs.resize(assistant::power_of_2_not_less_than_n(currentRecordCount));
            }

            // 加载索引
            indexTab.clear();
            indexTab.resize(fieldNameTab.size());
            for (const auto& idxSpec : loader->indexies())
                indexTab[fieldNameTab.at(idxSpec)] = std::make_unique<index_impl>(*this, fieldTab[fieldNameTab.at(idxSpec.fieldSpecs[0])], loader->index_tab(idxSpec).data);
        }

        void flush_all()
        {
            // 按页写入数据
            ID pageSize = loader->page_size();
            ID currentRecordCount = recordCount.load(std::memory_order::relaxed);
            ID pageCount = (currentRecordCount - 1) / pageSize + 1;
            for (ID i = 0; i < pageCount; ++i)
            {
                page pg{ 
                    .id = i,
                    .start = i * pageSize,
                    .count = std::min(pageSize, currentRecordCount - i * pageSize) 
                };
                pg.modified = std::unordered_set<ID>{ modifiedRows.lower_bound(pg.start), modifiedRows.lower_bound(pg.start + pg.count) };
                size_t startElem = pg.start * fieldTab.size();
                size_t elemCount = pg.count * fieldTab.size();
                pg.data = std::deque<field>(tabData.begin() + startElem, tabData.begin() + startElem + elemCount);
                loader->rows(pg);
            }

            // 写入索引
            for (auto& idx : indexTab)if (idx)
            {
                index i{ .spec = index_spec{
                    .name = idx->fieldSpec.name,
                    .comment = "",
                    .fieldSpecs = field_specs{
                        idx->fieldSpec
                    }
                } };
                i.data = idx->to_deque();
                loader->index_tab(i);
            }
        }

        void resize_data_storage(ID newRowSize)
        {
            ID targetSize = assistant::power_of_2_not_less_than_n(newRowSize);
            tabData.resize(targetSize * fieldTab.size());
            rowMtxs.resize(targetSize);
        }

        static std::unordered_map<std::string, size_t> build_field_name_table(const field_specs& ftab)
        {
            std::unordered_map<std::string, size_t> result;
            for (size_t i = 0; i < ftab.size(); ++i)
                result[ftab[i].name] = i;
            return result;
        }

    public:     // 系统接口
        // 表信息与控制
        virtual ID size() const override { return recordCount.load(std::memory_order::relaxed); }        // 返回 记录条数

        virtual ID trim() override             // 优化表记录，返回优化后的记录数
        {
            std::unique_lock ul{ tableMtx };

            {
                std::queue<std::unique_lock<std::shared_mutex>> lockQueue;
                for (auto& mtx : rowMtxs)lockQueue.emplace(mtx);
                ID writePos = 0;
                for (ID readPos = 0; readPos < recordCount.load(std::memory_order::relaxed); ++readPos)
                {
                    if (!(std::get<INT>(tabData[readPos * fieldTab.size() + fieldNameTab.at(sysfldRowMark)])
                        & row_bit_mark::deleted_bit))
                    {
                        if (writePos != readPos)
                            for (size_t fidx = 0; fidx < fieldTab.size(); fidx++)
                                tabData[writePos * fieldTab.size() + fidx] = std::move(tabData[readPos * fieldTab.size() + fidx]);
                        writePos++;
                    }
                }
                recordCount.store(writePos, std::memory_order::relaxed);
            }
            ID currentRecordCount = recordCount.load(std::memory_order::relaxed);
            resize_data_storage(currentRecordCount);

            // 重建索引
            for (auto& idx : indexTab)if(idx)
            {
                idx->clear();
                for (ID id = 0; id < currentRecordCount; ++id)
                {
                    if (!std::holds_alternative<NOTHING>(tabData[id * fieldTab.size() + fieldNameTab.at(idx->fieldSpec)]))
                        idx->insert(id);
                }
            }

            // 更新修改记录
            modifiedRows.clear();
            for(ID i = 0; i < currentRecordCount; ++i)
                modifiedRows.insert(i);

            // 落盘所有数据
            loader->clear();
            flush_all();
            
            return currentRecordCount;
        }

        virtual const field_specs& fields() const override { return fieldTab; } // 返回完整的字段表

        virtual const field_spec& get_field(const std::string& name) const override { return fieldTab[fieldNameTab.at(name)]; }// 通过字段名获取指定的字段信息

        virtual const field_specs system_fields() const { return field_specs{ sysfldRowMark }; }               // 返回系统字段表（如有）

        // 行访问接口
        virtual std::unique_ptr<entry> create() override                                                // 增：创建一条记录，返回相关的条目对象
        {
            ID pos;
            {
                std::shared_lock sl{ tableMtx };
                pos = fakeRecordCount.fetch_add(1, std::memory_order::relaxed);

                // 扩展数据存储
                if (tabData.size() < (pos + 1) * fieldTab.size())
                {
                    labourer::upgrade_lock ugl{ tableMtx };
                    if (tabData.size() < (pos + 1) * fieldTab.size())
                        resize_data_storage(pos + 1);
                }

                std::unique_lock rul{ rowMtxs[pos] };
                tabData[pos * fieldTab.size() + fieldNameTab.at(sysfldRowMark)] = INT{ 0 }; // 初始化系统字段
                recordCount.fetch_add(1, std::memory_order::relaxed);
            }
            return get_entry(pos);
        }

        virtual void drop(std::unique_ptr<entry> ety) override                                          // 删：通过条目对象删除记录
        {
            std::unique_lock rul{ *ety };
            ety->erase();
        }

        virtual std::unique_ptr<entry> at(ID id) override                                               // 通过行号访问记录
        {
            if (id >= recordCount.load(std::memory_order::relaxed))
                throw exceptions::common("Record ID out of range");
            return get_entry(id);
        }

        virtual std::list<ID> at(const std::function<bool(const entry&)>& filter) override // 改、查：通过过滤器查找记录
        {
            std::list<ID> result;
            for (auto& e : *this)
                if (filter(e))
                    result.push_back(e.id());
            return result;
        }

        virtual std::list<ID> excute(const incidents& icdts) override // 执行一系列事件，按照指定字段的排序顺序返回执行结果
        {
            // 字段搜索条件
            struct field_search_condition
            {
                bool impossible = false;
                bool hasDequal = false; std::list<field> dequalVals;
                bool hasEqual = false; field equalVal = NOTHING{};
                bool hasLower = false; field lowerVal = NOTHING{}; // lower is >=
                bool hasUpper = false; field upperVal = NOTHING{}; // upper is <
            };

            // helper：在无索引、或对索引结果做最终过滤时，用于判断某字段值是否满足该字段所有条件（AND）
            const auto eval_range_conditions = [this](const field& fv, const field_search_condition& conds, const std::string& fieldName)->bool 
                {
                    using ctype = incident::condition_param::condition_type;

                    if (conds.hasLower)
                        if (!(entry::compare_field_ls(conds.lowerVal, fv, fieldTab[fieldNameTab.at(fieldName)])
                            || entry::compare_field_eq(conds.lowerVal, fv, fieldTab[fieldNameTab.at(fieldName)])))
                            return false;
                    if (conds.hasUpper)
                        if (!entry::compare_field_ls(fv, conds.upperVal, fieldTab[fieldNameTab.at(fieldName)]))
                            return false;
                    return true;
                };

            const auto eval_eq_deq_conditions = [this](const field& fv, const field_search_condition& conds, const std::string& fieldName)->bool
                {
                    if (conds.hasEqual)
                        if (!entry::compare_field_eq(fv, conds.equalVal, fieldTab[fieldNameTab.at(fieldName)]))
                            return false;
                    if (conds.hasDequal)
                        for (const auto& dv : conds.dequalVals)
                            if (entry::compare_field_eq(fv, dv, fieldTab[fieldNameTab.at(fieldName)]))
                                return false;
                    return true;
                };

            const auto eval_qeual_by_index = [this](const index_impl& idx, const std::unordered_set<ID>& ref,const std::string& fieldName, const field_search_condition& fsc)->std::unordered_set<ID>
                {
                    using namespace assistant::set_operation;
                    std::unordered_set<ID> currentResult = ref;
                    if (fsc.hasEqual)
                    {
                        auto pdatent = std::make_unique<data_entry_impl>(*this);
                        pdatent->set(fieldName, fsc.equalVal);
                        currentResult = idx.search(*pdatent, currentResult);
                    }
                    // 处理不等于条件
                    if (fsc.hasDequal)
                    {
                        auto pdatent = std::make_unique<data_entry_impl>(*this);
                        for (const auto& dv : fsc.dequalVals)
                        {
                            pdatent->set(fieldName, dv);
                            currentResult = currentResult - idx.search(*pdatent, currentResult);
                        }
                    }
                    return currentResult;
                };

            // 集合操作命名空间
            using namespace assistant::set_operation;

            std::unordered_set<ID> finalResult;
            std::unordered_set<ID> currentResult;
            incidents opers = icdts;

            while (!opers.empty())
            {
                auto icdt = std::move(opers.front());
                opers.pop();

                switch (icdt.type)
                {
                case incident::incident_type::separate:
                {
                    finalResult = finalResult + currentResult;
                    currentResult = {};
                    break;
                }
                case incident::incident_type::join:
                {
                    currentResult = finalResult + currentResult;
                    finalResult = {};
                    break;
                }
                case incident::incident_type::ord_lmt:
                {
                    incident::ord_lmt_param param = std::get<incident::ord_lmt_param>(icdt.param);
                    std::vector<ID> ordered(currentResult.begin(),currentResult.end());
                    std::sort(ordered.begin(), ordered.end(), [this, param](ID l, ID r) {
                        data_entry_impl le{ *get_const_entry(l) };
                        data_entry_impl re{ *get_const_entry(r) };
                        return entry::compare_ls(le, re, param.targetFields);
                        });

                    ordered.resize(std::min(param.limit,ordered.size()));
                    currentResult.clear();
                    currentResult.insert_range(ordered);
                    break;
                }
                case incident::incident_type::create:
                {
                    entry& e = *std::get<std::shared_ptr<entry>>(icdt.param);
                    auto ne = create();
                    std::unique_lock ulpe{ *ne };
                    for (const auto& field : fieldTab)
                        ne->set(field, e.at(field));
                    break;
                }
                case incident::incident_type::drop:
                {
                    for (const auto& id : currentResult)
                    {
                        auto pe = get_entry(id);
                        std::unique_lock ule{ *pe };
                        pe->erase();
                    }
                    currentResult.clear();
                    break;
                }
                case incident::incident_type::modify:
                {
                    incident::modify_param param = std::get<incident::modify_param>(icdt.param);
                    for (const auto id : currentResult)
                    {
                        auto pe = get_entry(id);
                        std::unique_lock ule{ *pe };
                        pe->set(param.targetField, param.value);
                    }
                    break;
                }
                case incident::incident_type::search:
                {
                    bool impossiable = false;
                    std::unordered_map<std::string, field_search_condition> condByField;

                    {   // 收集连续的 search 事件 并计算全部条件
                        // 收集连续的 search 事件（包含当前这个）
                        std::unordered_map<std::string, std::list<incident::condition_param>> condParamsByField;
                        {
                            incident::condition_param cond = std::get<incident::condition_param>(icdt.param);
                            condParamsByField[cond.targetField].push_back(cond);
                        }
                        while (!opers.empty() && opers.front().type == incident::incident_type::search)
                        {
                            auto nxt = std::move(opers.front()); opers.pop();
                            {
                                incident::condition_param cond = std::get<incident::condition_param>(nxt.param);
                                condParamsByField[cond.targetField].push_back(cond);
                            }
                        }

                        // 计算每个字段的条件
                        for (const auto& [fieldName, conditions] : condParamsByField)
                        {
                            field_search_condition fsc;
                            const field_spec& fs = fieldTab[fieldNameTab.at(fieldName)];
                            for (const auto& cp : conditions)
                            {
                                switch (cp.type)
                                {
                                case incident::condition_param::condition_type::equal:
                                    if (!fsc.hasEqual) { fsc.hasEqual = true; fsc.equalVal = cp.reference; }
                                    else if (!entry::compare_field_eq(fsc.equalVal, cp.reference, fs)) { fsc.impossible = true; }
                                    break;
                                case incident::condition_param::condition_type::dequal:
                                    fsc.hasDequal = true; fsc.dequalVals.push_back(cp.reference);
                                    break;
                                case incident::condition_param::condition_type::less:
                                    if (!fsc.hasUpper) { fsc.hasUpper = true; fsc.upperVal = cp.reference; }
                                    else if (entry::compare_field_ls(cp.reference, fsc.upperVal, fs)) fsc.upperVal = cp.reference; // upper = min(upper, cp.ref)
                                    break;
                                case incident::condition_param::condition_type::greater:
                                    if (!fsc.hasLower) { fsc.hasLower = true; fsc.lowerVal = cp.reference; }
                                    else if (entry::compare_field_ls(fsc.lowerVal, cp.reference, fs)) fsc.lowerVal = cp.reference; // lower = max(lower, cp.ref)
                                    break;
                                default:
                                    break;
                                }
                            }
                            // 判断是否有矛盾条件
                            if (fsc.impossible)
                            {
                                impossiable = true;
                                break;
                            }
                            // equal 与 dequal 冲突
                            if (fsc.hasEqual && fsc.hasDequal)
                                for (const auto& ex : fsc.dequalVals)
                                    if (entry::compare_field_eq(fsc.equalVal, ex, fs))
                                    {
                                        impossiable = true;
                                        fsc.impossible = true;
                                        break;
                                    }
                            // equal 与下界/上界冲突
                            if (fsc.hasEqual)
                            {
                                if (fsc.hasLower)
                                    // equalVal < lowerVal 则不存在满足 >= lower 的等值
                                    if (entry::compare_field_ls(fsc.equalVal, fsc.lowerVal, fs))
                                    {
                                        impossiable = true;
                                        fsc.impossible = true;
                                    }
                                if (fsc.hasUpper)
                                    // 需要 equalVal < upperVal 成立（上界为开区间）
                                    if (!entry::compare_field_ls(fsc.equalVal, fsc.upperVal, fs))
                                    {
                                        impossiable = true;
                                        fsc.impossible = true;
                                    }
                            }
                            else
                                // 下界与上界相互冲突：需要 lowerVal < upperVal，否则区间为空
                                if (fsc.hasLower && fsc.hasUpper)
                                    if (!entry::compare_field_ls(fsc.lowerVal, fsc.upperVal, fs))
                                    {
                                        impossiable = true;
                                        fsc.impossible = true;
                                        break;
                                    }

                            if (impossiable)break;

                            // 如果有 equal 条件，则删除其他所有条件
                            if (fsc.hasEqual)
                            {
                                fsc.hasDequal = false; fsc.dequalVals = {};
                                fsc.hasLower = false; fsc.lowerVal = NOTHING{};
                                fsc.hasUpper = false; fsc.upperVal = NOTHING{};
                            }

                            condByField[fieldName] = fsc;
                        }
                    }

                    // 无解则直接返回空结果
                    if(impossiable)
                    {
                        currentResult.clear();
                        break;
                    }

                    std::shared_lock sl{ tableMtx };

                    // 寻找最佳索引
                    index_impl* perfectIdx = nullptr;
                    std::string perfectIdxField;
                    {   // 寻找总体积最小的索引，如果有相等条件就在相等条件终寻找
                        size_t minIdxSize = std::numeric_limits<size_t>::max();
                        bool hasEqual = false;
                        for (const auto& [fieldName, fsc] : condByField)
                        {
                            if (hasEqual && !fsc.hasEqual)continue;
                            if (!hasEqual && fsc.hasEqual)
                            {
                                perfectIdx = indexTab[fieldNameTab.at(fieldName)].get();
                                perfectIdxField = fieldName;
                                minIdxSize = perfectIdx->size();
                                hasEqual = true;
                            }
                            else if (fsc.hasEqual && indexTab[fieldNameTab.at(fieldName)] && indexTab[fieldNameTab.at(fieldName)]->size() < minIdxSize)
                            {
                                perfectIdx = indexTab[fieldNameTab.at(fieldName)].get();
                                perfectIdxField = fieldName;
                                minIdxSize = perfectIdx->size();
                            }
                            else if (indexTab[fieldNameTab.at(fieldName)] && indexTab[fieldNameTab.at(fieldName)]->size() < minIdxSize)
                            {
                                perfectIdx = indexTab[fieldNameTab.at(fieldName)].get();
                                perfectIdxField = fieldName;
                                minIdxSize = perfectIdx->size();
                            }
                        }
                    }

                    // 使用最佳索引进行初步搜索
                    if (perfectIdx)
                    {
                        auto& fsc = condByField[perfectIdxField];
                        currentResult = eval_qeual_by_index(*perfectIdx, currentResult, perfectIdxField, fsc);
                        if (currentResult.empty())
                            impossiable = true;
                    }

                    if (impossiable) break;

                    // 其他字段如果有索引且是 equal 条件，则继续使用索引缩小结果集，否则使用无索引过滤
                    for (const auto& [fieldName, fsc] : condByField)
                    {
                        if (fieldName == perfectIdxField)continue;
                        if (indexTab[fieldNameTab.at(fieldName)] && fsc.hasEqual)
                        {
                            auto& idx = *indexTab[fieldNameTab.at(fieldName)];
                            currentResult = eval_qeual_by_index(idx, currentResult, fieldName, fsc);
                        }
                        else if (currentResult.empty())  // 首次查询且无索引，需全表扫描
                            for (ID id = 0; id < recordCount.load(std::memory_order::relaxed); ++id)
                            {
                                std::shared_lock rsl{ rowMtxs[id] };
                                if (std::get<INT>(tabData[id * fieldNameTab.size() + fieldNameTab.at(sysfldRowMark)]) & row_bit_mark::deleted_bit)continue;
                                if (eval_range_conditions(tabData[id * fieldNameTab.size() + fieldNameTab.at(fieldName)], fsc, fieldName)
                                    && eval_eq_deq_conditions(tabData[id * fieldNameTab.size() + fieldNameTab.at(fieldName)], fsc, fieldName))
                                    currentResult.insert(id);
                            }
                        else  // 对现有结果集进行过滤
                        {
                            bool dequalEvaled = false;
                            if (fsc.hasDequal && indexTab[fieldNameTab.at(fieldName)])
                            {
                                auto& idx = *indexTab[fieldNameTab.at(fieldName)];
                                currentResult = currentResult - eval_qeual_by_index(idx, currentResult, fieldName, fsc);
                                dequalEvaled = true;
                            }
                            for (auto it = currentResult.begin(); it != currentResult.end(); )
                            {
                                std::shared_lock rsl{ rowMtxs[*it] };
                                if ((!dequalEvaled && !eval_eq_deq_conditions(tabData[*it * fieldNameTab.size() + fieldNameTab.at(fieldName)], fsc, fieldName))
                                    || !eval_range_conditions(tabData[*it * fieldNameTab.size() + fieldNameTab.at(fieldName)], fsc, fieldName))
                                    it = currentResult.erase(it);
                                else
                                    ++it;
                            }
                        }

                        if (currentResult.empty()) { impossiable = true; break; }
                    }

                    if (impossiable) break;
                }
                default:
                    break;
                }
            }

            if (!currentResult.empty())finalResult = finalResult + currentResult;
            std::list<ID> result(finalResult.begin(), finalResult.end());
            result.sort();
            return result;
        }
        
        // 索引接口
        virtual void create_index(const std::string&, const field_specs& fieldSpecs) override// 创建指定名称、基于指定字段的索引
        {
            {
                if (fieldSpecs.size() != 1)
                    throw exceptions::common("Only single-field index is supported in simple_memory_table");
                std::unique_lock sl{ tableMtx };
                if (indexTab[fieldNameTab.at(fieldSpecs[0])])
                    throw exceptions::common("Index on field '" + fieldSpecs[0].name + "' already exists");

                auto& idx = indexTab[fieldNameTab.at(fieldSpecs[0])];
                idx = std::make_unique<index_impl>(*this, fieldTab[fieldNameTab.at(fieldSpecs[0])]);
            }
            auto& idx = indexTab[fieldNameTab.at(fieldSpecs[0])];
            for(auto& ent : *this)
            {
                if (!std::holds_alternative<NOTHING>(tabData[ent.id() * fieldTab.size() + fieldNameTab.at(fieldSpecs[0])]))
                    idx->insert(ent.id());
            }
        }

        virtual void drop_index(const std::string& name) override 
        {
            std::unique_lock ul{ tableMtx };
            indexTab[fieldNameTab.at(name)].reset();
        }

    private:// 由派生类实现：返回指向首条记录的迭代器 / 尾后迭代器
        virtual std::unique_ptr<entry> begin_impl() { return get_entry(0); }

        virtual std::unique_ptr<entry> end_impl() { return std::make_unique<data_entry_impl>(*this); }

    public:
        std::unique_ptr<entry> operator[](ID id) { return at(id); }

    public:     // 扩展管理接口
        ID reserve(ID recCount)
        {
            ID currentSpace = assistant::power_of_2_not_less_than_n(recordCount.load(std::memory_order::relaxed));
            if(currentSpace >= recCount)
                return currentSpace;
            std::unique_lock ul{ tableMtx };
            resize_data_storage(recCount);
        }

    public: // 表始终从 loader 构造
        simple_memory_table(std::unique_ptr<archivist::loader>&& ld)
            : loader(std::move(ld)), fieldTab(loader->fields()), fieldNameTab(build_field_name_table(fieldTab)){
            if (fieldTab[0] != sysfldRowMark || fieldTab[0].type != sysfldRowMark.type)
                throw exceptions::common("The first field must be the system field '$RowMark'");
            sync_all();
        }

        ~simple_memory_table() noexcept { flush_all(); }
            

    };
}



/*************** 合并自 PrintCenter.h ***************/
// #pragma once
// #include "framework.h"
// #include "pch.h"

// #include "background.h"
// #include "datetime.h"
// #include "string_utilities.h"
// #include "secretary_streambuf.h"

namespace HYDRA15::Union::secretary
{
    // 统一输出接口
    // 提供滚动消息、底部消息和写入文件三种输出方式
    // 提交消息之后，调用 notify() 方法通知后台线程处理，这在连续提交消息时可以解约开销
    class PrintCenter final :protected labourer::background
    {
        /***************************** 快速接口 *****************************/
    public:
        template<typename ...Args>
        static size_t println(Args ... args)
        {
            PrintCenter& instance = get_instance();
            std::stringstream ss;
            (ss << ... << args);
            size_t ret = instance.rolling(ss.str());
            instance.flush();
            return ret;
        }
        template<typename ... Args>
        static size_t printf(const std::string& fstr, Args...args) { return println(std::vformat(fstr, std::make_format_args(args...))); }
        static unsigned long long set(const std::string& str, bool forceDisplay = false, bool neverExpire = false);
        static void update(unsigned long long id, const std::string& str);
        static void remove(unsigned long long id);
        static void set_stick_btm(const std::string& str);
        static size_t fprint(const std::string& str);
        PrintCenter& operator<<(const std::string& content);    // 快速输出，滚动消息+文件+刷新

        /***************************** 公有单例 *****************************/
    protected:
        // 禁止外部构造
        PrintCenter();
        PrintCenter(const PrintCenter&) = delete;

        // 获取接口
    public:
        static PrintCenter& get_instance();

    public:
        ~PrintCenter();

        /***************************** 公 用 *****************************/
        // 类型
    private:
        using time_point = std::chrono::steady_clock::time_point;
        using milliseconds = std::chrono::milliseconds;

        // 全局配置
    private:
        static struct Config
        {
            static constexpr milliseconds refreshInterval = milliseconds(30000); // 最短刷新间隔

            static_uint btmMaxLines = 3;
            static constexpr milliseconds btmDispTimeout = milliseconds(1000);
            static constexpr milliseconds btmExpireTimeout = milliseconds(30000);
            
        }cfg;
        std::function<bool(char)> is_valid_with_ansi = [](char c) {return (c > 0x20 && c < 0x7F) || c == 0x1B; };


        // 辅助函数
    private:
        std::string clear_bottom_msg();    // 清除底部消息
        std::string print_rolling_msg(); // 输出滚动消息
        std::string print_bottom_msg();  // 输出底部消息
        std::string print_file_msg();    // 输出文件消息

        // 重定向时修改此变量
        std::shared_ptr<ostreambuf> pPCOutBuf;
        std::shared_ptr<std::ostream> pSysOutStream;
        std::function<void(const std::string&)> print;
        std::function<void(const std::string&)> printFile;

        // 是否启用ansi颜色
        bool enableAnsiColor = true;

        // 工作
    private:
        std::condition_variable sleepcv;
        std::mutex systemLock;
        std::atomic<bool> working = true;
        std::atomic<bool> forceRefresh = false;
        time_point lastRefresh = time_point::clock::now();
        virtual void work() noexcept override;

        // 高级接口
    public:
        void flush();  // 刷新
        void sync_flush();  // 同步刷新，后台线程刷新完成后才会返回
        void lock();   // 锁定，防止刷新
        void unlock(); // 解锁，允许刷新
        void fredirect(std::function<void(const std::string&)> fprintFunc);
        void enable_ansi_color(bool c);

        /***************************** 滚动消息相关 *****************************/
        // 类型定义
    private:
        using rollmsg_list = std::list<std::string>;

        // 数据
    private:
        rollmsg_list* pRollMsgLstFront = new rollmsg_list();
        rollmsg_list* pRollMsgLstBack = new rollmsg_list();
        std::mutex rollMsgLock;
        size_t rollMsgCount = 0;

        // 接口
    public:
        size_t rolling(const std::string& content);


        /***************************** 底部消息相关 *****************************/
       // 类型定义
    private:
        struct btmmsg_ctrlblock
        {
            time_point lastUpdate = time_point::clock::now();
            bool forceDisplay = false;
            bool neverExpire = false;
            std::string msg;
        };

    public:
        using ID = unsigned long long;
    private:
        using btmmsg_tab = std::unordered_map<ID, btmmsg_ctrlblock>;
        

        // 数据
    private:
        std::string stickBtmMsg;
        btmmsg_tab btmMsgTab;
        ID btmMsgNextID = 0;
        std::mutex btmMsgTabLock;
        size_t lastBtmLines = 0;

        // 工具函数
    private:
        ID find_next_ID();

        // 接口
    public:
        ID new_bottom(bool forceDisplay = false, bool neverExpire = false);
        void update_bottom(ID id, const std::string& content);
        bool check_bottom(ID id);
        void remove_bottom(ID id);
        void stick_btm(const std::string& str = std::string());


        /***************************** 写入文件相关 *****************************/
        // 类型定义
    private:
        using filemsg_list = std::list<std::string>;

        // 数据
    private:
        filemsg_list* pFMsgLstFront = new filemsg_list();
        filemsg_list* pFMsgLstBack = new filemsg_list();
        std::mutex fileMsgLock;
        size_t fileMsgCount = 0;

        // 接口
    public:
        size_t file(const std::string& content);
    };
}




/*************** 合并自 sfstream.h ***************/
// #pragma once
// #include "pch.h"
// #include "framework.h"

// #include "fstreams.h"
// #include "byteswap.h"
// #include "utilities.h"
// #include "iMutexies.h"

namespace HYDRA15::Union::archivist
{
    /***************************** 设 计 ****************************
    * 以 段 为文件的最小 IO 单位
    * 用户以 节 为文件分割单位，一个节由数个连续或不连续的段组成
    * 用户访问文件时，只需传入节名、节内偏移量和大小，程序自动管理段节映射
    * 多线程状态下只保护元数据，不保护实际文件数据
    * 继承自 bsfstream 的特性 ：可选多线程 io 
    *
    * **************************** 文件格式 ****************************
    * 根节（第 0 段开始）：
    *   16 "ArchivistSection" 标记        8 版本号              8 段大小
    *   8 最大段数        8 已用段数      8 根节段表指针        8 根节段数
    *   8 节表起始指针    8 节数          8 自定义头开始指针    8 自定义头长度
    *   32N 根节段表：
    *       8N 段 ID
    *   32N 节表：
    *       8 节名指针    8 节注释指针    8 段表指针    8 段数
    *   32N 节名、节注释、节段表数据
    *   32N 自定义头
    *
    * ********************************************************************/
    class sfstream
    {
    private:
        struct section
        {
            mutable labourer::atomic_shared_mutex asmtx;    // 只保护元数据，不保护实际数据
            std::string comment;
            std::deque<uint64_t> segIDs;
            section() = default;
            section(const section& oth) :comment(oth.comment), segIDs(oth.segIDs) {}
        };

    public:
        enum class segment_size_level : size_t {
            I = 4096, II = 64 * 1024, III = 256 * 1024,
            IV = 1 * 1024 * 1024, V = 4 * 1024 * 1024, VI = 64 * 1024 * 1024
        };

    private:
        static constexpr std::string_view headerMark{ "ArchivistSection" };
        static constexpr uint64_t headerVersion = 0x00010000;
        static constexpr size_t minFileSize = 96;

    private:
        assistant::bsfstream bsfs;

        const uint64_t segSize;
        uint64_t maxSegCount{ std::numeric_limits<uint64_t>::max() };
        uint64_t usedSegCount{ 0 };

        section rootSection;
        std::unordered_map<std::string, section> sections;
        std::vector<byte> customHeader;

        mutable labourer::atomic_shared_mutex asmtx;

    private:
        static uint64_t check_and_extract_segSize(const std::filesystem::path& p)
        {
            if (!std::filesystem::exists(p) || !std::filesystem::is_regular_file(p))
                throw exceptions::files::FileNotExist(p);

            assistant::bfstream bfs(p);

            // 检查必要的头大小
            if (bfs.size() < 96)
                throw exceptions::files::FormatNotSupported(p, "Archivist Sectioned files are expected");

            // 检查标记
            std::string mark{ bfs.read<char>(0,16).data(),16 };
            if (mark != headerMark)
                throw exceptions::files::FormatNotSupported(p, "Archivist Sectioned files are expected");

            // 检查版本号
            auto version = bfs.read<uint64_t>(16, 1);
            version[0] = assistant::byteswap::from_big_endian(version[0]);
            if (version[0] != headerVersion)
                throw exceptions::files::FormatNotSupported(p, "the version 0x00010000 is expected");

            // 提取基础数据
            return assistant::byteswap::from_big_endian(bfs.read<uint64_t>(24, 1)[0]);
        }

        static uint64_t calculate_sec_tab_size(const std::unordered_map<std::string, section>& sections)
        {
            uint64_t size = 0;
            for (const auto& [k, v] : sections)
                size += assistant::multiple_m_not_less_than_n(32, k.size() + 1)
                + assistant::multiple_m_not_less_than_n(32, v.comment.size() + 1)
                + assistant::multiple_m_not_less_than_n(32, v.segIDs.size() * 8)
                + 32;
            return size;
        }

        void expand(std::deque<uint64_t>& segIDs, uint64_t segCount)    // 将指定节 segIDs 扩展 segCount 个段
        {
            if (usedSegCount + segCount > maxSegCount)
                throw exceptions::files::FileFull(bsfs.data().file_path(), maxSegCount * segSize);
            for (uint64_t i = 0; i < segCount; i++)
                segIDs.push_back(usedSegCount++);
        }

        void flush_rootsec()   // 根节落盘
        {
            {   // 写入标记
                std::vector<char> mark(16, 0);
                assistant::memcpy(headerMark.data(), mark.data(), 16);
                bsfs.write(0, 0, mark);
            }

            // 写入基本数据
            std::vector<uint64_t> basicHeader(10, 0);
            basicHeader[0] = headerVersion;                                                         // 版本号
            basicHeader[1] = segSize;                                                               // 段大小
            basicHeader[2] = maxSegCount;                                                           // 最大段数
            basicHeader[3] = usedSegCount;                                                          // 已用段数
            basicHeader[4] = 96;                                                                    // 根节段表指针
            basicHeader[5] = rootSection.segIDs.size();                                             // 根节段数
            basicHeader[6] = 96 + assistant::multiple_m_not_less_than_n(32, basicHeader[5] * 8);    // 节表指针
            basicHeader[7] = sections.size();                                                       // 节数
            basicHeader[8] = basicHeader[6] + calculate_sec_tab_size(sections);                     // 自定义头指针
            basicHeader[9] = customHeader.size();                                                   // 自定义头大小

            {
                auto basicHeaderCpy = basicHeader;
                assistant::byteswap::to_big_endian_vector<uint64_t>(basicHeaderCpy);
                bsfs.write(0, 16, basicHeaderCpy);
            }

            {   // 计算并扩展根节
                uint64_t rootSecTotalSize = basicHeader[8] + assistant::multiple_m_not_less_than_n(32, basicHeader[9]);
                if (rootSecTotalSize > segSize * rootSection.segIDs.size())
                    expand(rootSection.segIDs, (rootSecTotalSize - segSize * rootSection.segIDs.size() - 1) / segSize + 1);
            }

            {   // 写入根节段表
                std::vector<uint64_t> segVec(assistant::multiple_m_not_less_than_n(4, rootSection.segIDs.size()), 0);
                for (size_t i = 0; i < rootSection.segIDs.size(); i++)
                    segVec[i] = rootSection.segIDs[i];
                assistant::byteswap::to_big_endian_vector(segVec);
                bsfs.write(rootSection.segIDs, basicHeader[4], segVec);
            }

            {
                // 写入节表
                uint64_t currentEntryPos = basicHeader[6];
                uint64_t currentDataPos = currentEntryPos + 32 * sections.size();
                for (const auto& [k, v] : sections)
                {
                    uint64_t nameSize = assistant::multiple_m_not_less_than_n(32, k.size() + 1);
                    uint64_t commSize = assistant::multiple_m_not_less_than_n(32, v.comment.size() + 1);
                    uint64_t segtSize = assistant::multiple_m_not_less_than_n(4, v.segIDs.size());

                    std::vector<uint64_t> entry(4, 0);
                    std::vector<char> name(nameSize, 0);
                    std::vector<char> comment(commSize, 0);
                    std::vector<uint64_t> segTab(segtSize, 0);

                    entry[0] = currentDataPos;
                    entry[1] = currentDataPos + nameSize;
                    entry[2] = currentDataPos + nameSize + commSize;
                    entry[3] = v.segIDs.size();
                    assistant::byteswap::to_big_endian_vector(entry);

                    assistant::memcpy(k.data(), name.data(), k.size());
                    assistant::memcpy(v.comment.data(), comment.data(), v.comment.size());

                    for (size_t i = 0; i < v.segIDs.size(); i++)
                        segTab[i] = v.segIDs[i];
                    assistant::byteswap::to_big_endian_vector(segTab);

                    bsfs.write(rootSection.segIDs, currentEntryPos, entry);
                    bsfs.write(rootSection.segIDs, currentDataPos, name);
                    bsfs.write(rootSection.segIDs, currentDataPos + nameSize, comment);
                    bsfs.write(rootSection.segIDs, currentDataPos + nameSize + commSize, segTab);

                    currentEntryPos += 32;
                    currentDataPos += nameSize + commSize + segtSize * 8;
                }
            }

            {   // 写入自定义头
                bsfs.write(rootSection.segIDs, basicHeader[8], customHeader);
            }
        }

        void sync_rootsec()   // 根节加载
        {
            // 读取基本数据
            auto basicHeader = bsfs.read<uint64_t>(0, 16, 10);
            assistant::byteswap::from_big_endian_vector<uint64_t>(basicHeader);

            uint64_t version = basicHeader[0];          // 版本号
            uint64_t segSizeInFile = basicHeader[1];    // 段大小
            maxSegCount = basicHeader[2];               // 最大段数
            usedSegCount = basicHeader[3];              // 已用段数
            uint64_t rootSegTabPtr = basicHeader[4];    // 根节段表指针
            uint64_t rootSegTabCount = basicHeader[5];  // 根节段数
            uint64_t secTabPtr = basicHeader[6];        // 节表指针
            uint64_t secCount = basicHeader[7];         // 节数
            uint64_t customHeaderPtr = basicHeader[8];  // 自定义头指针
            uint64_t customHeaderSize = basicHeader[9]; // 自定义头大小

            // 读取根节段表
            rootSection.segIDs.clear();
            rootSection.segIDs.push_back(0);
            if (rootSegTabPtr != 0 && rootSegTabCount != 0)
                while (rootSection.segIDs.size() < rootSegTabCount)
                {
                    auto segIDs = bsfs.read<uint64_t>(rootSection.segIDs, rootSegTabPtr, std::min((segSize * rootSection.segIDs.size() - rootSegTabPtr) / 8, rootSegTabCount));
                    assistant::byteswap::from_big_endian_vector<uint64_t>(segIDs);
                    rootSection.segIDs = std::deque<uint64_t>(segIDs.begin(), segIDs.end());
                }

            {   // 读取节表
                sections.clear();
                for (size_t i = 0; i < secCount; i++)
                {
                    auto entry = bsfs.read<uint64_t>(rootSection.segIDs, secTabPtr + 32 * i, 4);
                    assistant::byteswap::from_big_endian_vector<uint64_t>(entry);

                    std::string secName = bsfs.read_string(rootSection.segIDs, entry[0]);
                    std::string secComment = bsfs.read_string(rootSection.segIDs, entry[1]);

                    auto segTabData = bsfs.read<uint64_t>(rootSection.segIDs, entry[2], entry[3]);
                    assistant::byteswap::from_big_endian_vector<uint64_t>(segTabData);
                    std::deque<uint64_t> segIDs{ segTabData.begin(), segTabData.end() };

                    sections[secName].comment = secComment;
                    sections[secName].segIDs = segIDs;
                }
            }

            {   // 读取自定义头
                customHeader = bsfs.read<byte>(rootSection.segIDs, customHeaderPtr, customHeaderSize);
            }
        }

    public:
        template<typename T>
            requires std::is_trivial_v<T>
        std::vector<T> read(const std::string& secName, size_t pos, size_t count) const
        {
            std::shared_lock sl{ asmtx };

            if(!sections.contains(secName))
                throw exceptions::files::ContentNotFound(bsfs.data().file_path())
                .set("content type", "section")
                .set("section name", secName);
            
            const section& sec = sections.at(secName);
            std::shared_lock ssl{ sec.asmtx };
            return bsfs.read<T>(sec.segIDs, pos, count);
        }

        template<typename T>
            requires std::is_trivial_v<T>
        void write(const std::string& secName, size_t pos, const std::vector<T>& data)
        {
            std::shared_lock sl{ asmtx };

            if (!sections.contains(secName))
            {
                labourer::upgrade_lock ugl{ asmtx };
                sections.emplace(std::piecewise_construct, std::forward_as_tuple(secName), std::forward_as_tuple());
            }

            section& sec = sections.at(secName);
            std::shared_lock ssl{ sec.asmtx };
            if (sec.segIDs.size() * segSize < pos + data.size() * sizeof(T))
            {
                labourer::upgrade_lock ugl{ sec.asmtx };
                expand(sec.segIDs, (pos + data.size() * sizeof(T) - sec.segIDs.size() * segSize - 1) / segSize + 1);
            }
            return bsfs.write<T>(sec.segIDs, pos, data);
        }

    public:
        size_t sec_count() const { std::shared_lock sl{ asmtx }; return sections.size(); }

        std::list<std::string> sec_list() const
        {
            std::list<std::string> res;
            std::shared_lock sl{asmtx};
            for (const auto& [k, v] : sections)
                res.push_back(k);
            return res;
        }

        bool sec_contains(const std::string& secName) const { std::shared_lock sl{ asmtx }; return sections.contains(secName); }

        void set_sec_comment(const std::string& secName, const std::string& comment)
        {
            std::shared_lock sl{ asmtx };
            if (!sections.contains(secName))
                throw exceptions::files::ContentNotFound(bsfs.data().file_path())
                .set("content type", "section")
                .set("section name", secName);
            section& sec = sections.at(secName);
            std::unique_lock ul{ sec.asmtx };
            sec.comment = comment;
        }

        void new_section(const std::string& secName, uint64_t segCount ,const std::string& comment = "")
        {
            {
                std::unique_lock ul{ asmtx };
                if (sections.contains(secName))
                    throw exceptions::common::BadParameter("secName", secName, "a unique section name");
                sections[secName].comment = comment;
            }

            std::shared_lock sl{ asmtx };
            std::unique_lock sul{ sections[secName].asmtx };
            expand(sections.at(secName).segIDs, segCount);
        }

        uint64_t seg_size() const { return segSize; }

        uint64_t max_seg_count() const { std::shared_lock sl{ asmtx }; return maxSegCount; }

        void max_seg_count(uint64_t limit) { std::unique_lock ul{ asmtx }; maxSegCount = limit; }

        void flush() { std::unique_lock ul{ asmtx }; flush_rootsec(); }

        assistant::bfstream& data() { return bsfs.data(); }

        const assistant::bfstream& data() const { return bsfs.data(); }

        std::vector<byte>& custom_header() { return customHeader; } // 自定义头不受保护

        void clear()    // 只清空元数据，不清空实际数据
        {
            std::unique_lock ul{ asmtx };
            rootSection.segIDs.clear();
            rootSection.segIDs.push_back(0);
            sections.clear();
            customHeader.clear();
            usedSegCount = 1;
            flush_rootsec();
        }

    private:    // 仅允许从工厂方法构造
        sfstream(
            const std::filesystem::path& p,
            uint64_t segSize,
            unsigned int ioThreads
        )
            :segSize(segSize), bsfs(p, segSize, ioThreads)
        {

        }

        sfstream(sfstream&&) noexcept = default;

    public:
         sfstream() = delete;
         sfstream(const sfstream& oth)
             :bsfs(oth.bsfs), segSize(oth.segSize), maxSegCount(oth.maxSegCount), usedSegCount(oth.usedSegCount),
             rootSection(oth.rootSection), sections(oth.sections), customHeader(oth.customHeader) {
         }

        ~sfstream() { flush_rootsec(); }
        
    public: // 工厂构造方法
        // 构造方式 1：新建文件，若已有文件将报错
        static sfstream make(
            const std::filesystem::path& p,
            segment_size_level level,
            uint64_t maxSegs = std::numeric_limits<uint64_t>::max(),
            unsigned int ioThreads = 4
        ) {
            if (maxSegs == 0)throw exceptions::common::BadParameter("maxSegs", "0", "> 0");
            if (std::filesystem::exists(p) && std::filesystem::is_regular_file(p))
                throw exceptions::files::FileAreadyExist(p);

            // 构造并设置对象
            sfstream sfs(p, static_cast<uint64_t>(level), ioThreads);
            sfs.rootSection.segIDs.push_back(0);
            sfs.usedSegCount = 1, std::memory_order::relaxed;
            sfs.flush_rootsec();
            sfs.maxSegCount = maxSegs;
            return sfs;
        }

        static std::unique_ptr<sfstream> make_unique(
            const std::filesystem::path& p,
            segment_size_level level,
            uint64_t maxSegs = std::numeric_limits<uint64_t>::max(),
            unsigned int ioThreads = 4
        ) {
            if (maxSegs == 0)throw exceptions::common::BadParameter("maxSegs", "0", "> 0");
            if (!std::filesystem::exists(p) || !std::filesystem::is_regular_file(p))
                throw exceptions::files::FileAreadyExist(p);

            // 构造并设置对象
            auto psfs = std::unique_ptr<sfstream>(new sfstream(p, static_cast<uint64_t>(level), ioThreads));
            psfs->rootSection.segIDs.push_back(0);
            psfs->usedSegCount = 1, std::memory_order::relaxed;
            psfs->flush_rootsec();
            psfs->maxSegCount = maxSegs;
            return psfs;
        }

        // 构造方式 2：打开已有文件
        static sfstream make(const std::filesystem::path& p, unsigned int ioThreads = 4)
        {
            uint64_t segSize = check_and_extract_segSize(p);

            // 构造并设置对象
            sfstream sfs(p, segSize, ioThreads);
            sfs.sync_rootsec();
            return sfs;
        }

        static std::unique_ptr<sfstream> make_unique(const std::filesystem::path& p, unsigned int ioThreads = 4)
        {
            uint64_t segSize = check_and_extract_segSize(p);

            // 构造并设置对象
            auto psfs = std::unique_ptr<sfstream>(new sfstream(p, segSize, ioThreads));
            psfs->sync_rootsec();
            return psfs;
        }
    };
}



/*************** 合并自 log.h ***************/
// #pragma once
// #include "framework.h"
// #include "pch.h"

// #include "PrintCenter.h"

namespace HYDRA15::Union::secretary
{
    // 格式化日志字符串
    // 返回格式化后的字符串，用户需要自行处理输出
    class log
    {
        // 禁止构造
    private:
        log() = delete;
        log(const log&) = delete;
        ~log() = delete;

        // 私有数据
    private:
        static struct visualize
        {
            static_string info = "[ {0} | INFO ] [ {1} ] {2}";
            static_string warn = "[ {0} | WARN ] [ {1} ] {2}";
            static_string error = "[ {0} | ERROR ][ {1} ] {2}";
            static_string fatal = "[ {0} | FATAL ][ {1} ] {2}";
            static_string debug = "[ {0} | DEBUG ][ {1} ] {2}";
            static_string trace = "[ {0} | TRACE ][ {1} ] {2}";
        }vslz;

        static struct visualize_color
        {
            static_string info = "\033[0m[ {0} | INFO ] [ {1} ] {2}\033[0m";
            static_string warn = "\033[0m[ {0} | \033[33mWARN\033[0m ] [ {1} ] {2}\033[0m";
            static_string error = "\033[0m[ {0} | \033[35mERROR\033[0m ][ {1} ] {2}\033[0m";
            static_string fatal = "\033[0m[ {0} | \033[31mFATAL\033[0m ][ {1} ] \033[31m{2}\033[0m";
            static_string debug = "\033[0m[ {0} | \033[2mDEBUG\033[0m ][ {1} ] {2}\033[0m";
            static_string trace = "\033[0m[ {0} | \033[34mTRACE\033[0m ][ {1} ] {2}\033[0m";
        }vslzclr;
        
        // 公有接口
    public:
        static std::string info(const std::string& title, const std::string& content)
        {
            std::string str;
            std::string date = assistant::datetime::now_date_time("%Y-%m-%d %H:%M:%S");
            if (colourful)str = std::format(vslzclr.info.data(), date, title, content);
            else str = std::format(vslz.info.data(), date, title, content);
            if (print)print(str);
            return str;
        }

        static std::string warn(const std::string& title, const std::string& content)
        {
            std::string str;
            std::string date = assistant::datetime::now_date_time("%Y-%m-%d %H:%M:%S");
            if (colourful)str = std::format(vslzclr.warn.data(), date, title, content);
            else str = std::format(vslz.warn.data(), date, title, content);
            if (print)print(str);
            return str;
        }

        static std::string error(const std::string& title, const std::string& content)
        {
            std::string str;
            std::string date = assistant::datetime::now_date_time("%Y-%m-%d %H:%M:%S");
            if (colourful)str = std::format(vslzclr.error.data(), date, title, content);
            else str = std::format(vslz.error.data(), date, title, content);
            if (print)print(str);
            return str;
        }

        static std::string fatal(const std::string& title, const std::string& content)
        {
            std::string str;
            std::string date = assistant::datetime::now_date_time("%Y-%m-%d %H:%M:%S");
            if (colourful)str = std::format(vslzclr.fatal.data(), date, title, content);
            else str = std::format(vslz.fatal.data(), date, title, content);
            if (print)print(str);
            return str;
        }

        static std::string debug(const std::string& title, const std::string& content)
        {
            std::string str;
            std::string date = assistant::datetime::now_date_time("%Y-%m-%d %H:%M:%S");
            if (colourful)str = std::format(vslzclr.debug.data(), date, title, content);
            else str = std::format(vslz.debug.data(), date, title, content);
            if (print && enableDebug)print(str);
            return str;
        }

        static std::string trace(const std::string& title, const std::string& content)
        {
            std::string str;
            std::string date = assistant::datetime::now_date_time("%Y-%m-%d %H:%M:%S");
            if (colourful)str = std::format(vslzclr.trace.data(), date, title, content);
            else str = std::format(vslz.trace.data(), date, title, content);
            if (print && enableDebug)print(str);
            return str;
        }

        // 配置项
    public:
        inline static std::function<void(const std::string&)> print;
        static inline bool enableDebug = HYDRA15::Union::debug;
        static inline bool colourful = true;
    };
}



/*************** 合并自 single_loader.h ***************/
// #pragma once
// #include "pch.h"
// #include "framework.h"

// #include "archivist_interfaces.h"
// #include "sfstream.h"

// #include "lib_exceptions.h"

namespace HYDRA15::Union::archivist
{
    /***************************** 设 计 ****************************
    * 
    * 依托 sfstream 实现文件分节管理的特性，禁用多线程 IO
    * 固定页大小，页段自动对齐
    * 
    * **************************** 文件格式 ****************************
    * 
    * 自定义头：
    *   16 "ArchivistSingle\0" 标记           8 版本号        8 总记录数
    *   8 字段表指针        8 字段表条目数    8 索引表指针    8 索引表条目数
    *   8 当前数据包编号    24 保留
    *   字段表：
    *       32N 字段记录：
    *           8 字段名指针    8 字段注释指针    1 字段类型    7 标记    8 保留
    *       32N 字段名、字段注释数据
    *   索引表：
    *       32N 索引表记录：
    *           8 索引名指针    8 索引注释指针    8 索引字段 ID 指针    8 索引字段数
    *       32N 索引名、索引注释、索引字段 ID 数据
    * 
    * 表记录节 (table): 
    *   32N 表记录条目: 
    *       4 数据包编号    4 保留    8N 字段数据
    *       - 字段数据结构: 无数据填全 0 , INT 和 FLOAT 8 数据, 数组类型 高 4 指针 + 低 4 长度
    * 
    * 索引节 (index::${indexName})：
    *   32 头：
    *       8 索引记录数    24 保留
    *   8N 索引记录：
    *       8 行号
    * 
    * 数据包节 (data::${packID:08X}):
    *   32 头：
    *       4 已用数据大小    28 保留
    *   连续的数据
    *   
    *
    * ********************************************************************/

    class single_loader : public loader
    {
    private:
        static constexpr std::string_view headerMark{ "ArchivistSingle\0" };
        static constexpr uint64_t headerVersion = 0x00010000;
        static constexpr size_t headerSize = 96;
        static constexpr std::string_view tableSectionName{ "table" };
        static constexpr std::string_view indexSectionFmt{ "index::{}" };
        static constexpr std::string_view dataSectionFmt{ "data::{:08X}" };
        static constexpr uint64_t datPkgDatStartOffset = 32;
        static constexpr uint64_t idxDatStartOffset = 32;

    private:
        sfstream sfs;
        mutable std::shared_mutex smtx;     // 保护实际数据，全局拒绝并发写入

        const field_specs fieldSpecs;
        const ID pageSize;
        const ID rowSize;
        const std::pair<ID, ID> tabVer;
        ID totalRecords = 0;
        uint64_t currentPackID = 0;
        std::unordered_map<std::string, index_spec> indexMap;

    private:
        virtual std::pair<ID, ID> version() const override { return tabVer; }  // 返回上层table的版本号

        static std::tuple<sfstream, field_specs, std::pair<ID,ID>> check_and_extract_field_specs(const std::filesystem::path& p)
        {
            sfstream sfs = sfstream::make(p, 0);
            std::vector<byte> cstHdr = sfs.custom_header();
            uint64_t fieldTabPtr;
            uint64_t fieldEntCnt;
            std::pair<uint64_t, uint64_t> tabVer;
            {   // 检查文件头标记、提取数据
                if (cstHdr.size() < headerSize)
                    throw exceptions::files::FormatNotSupported(p, "Archivist Single database files are expected");

                std::string mark{ reinterpret_cast<char*>(cstHdr.data()),16 };
                uint64_t ver = 0;
                assistant::memcpy(cstHdr.data() + 16, reinterpret_cast<byte*>(&ver), sizeof(uint64_t));
                ver = assistant::byteswap::from_big_endian(ver);
                if (mark != std::string(headerMark.data(), 16) || ver != headerVersion)
                    throw exceptions::files::FormatNotSupported(p, "Archivist Single database files are expected");

                std::vector<uint64_t> hdrDat(8, 0);
                assistant::memcpy(cstHdr.data() + 32, reinterpret_cast<byte*>(hdrDat.data()), hdrDat.size() * sizeof(uint64_t));
                assistant::byteswap::from_big_endian_vector(hdrDat);
                fieldTabPtr = hdrDat[0];
                fieldEntCnt = hdrDat[1];
                tabVer.first = hdrDat[5];
                tabVer.second = hdrDat[6];
            }

            field_specs fieldSpecs(fieldEntCnt, field_spec{});
            for (uint64_t i = 0; i < fieldEntCnt; i++)
            {
                std::vector<uint64_t> entDat(2, 0);
                assistant::memcpy(cstHdr.data() + fieldTabPtr + 32 * i, reinterpret_cast<byte*>(entDat.data()), entDat.size() * sizeof(uint64_t));
                assistant::byteswap::from_big_endian_vector(entDat);
                uint64_t namePtr = entDat[0];
                uint64_t commPtr = entDat[1];

                field_spec fs{
                    std::string(reinterpret_cast<char*>(cstHdr.data() + namePtr)),
                    std::string(reinterpret_cast<char*>(cstHdr.data() + commPtr)),
                    static_cast<field_spec::field_type>(cstHdr[fieldTabPtr + 32 * i + 16])
                };
                assistant::memcpy(cstHdr.data() + fieldTabPtr + 32 * i + 17, fs.mark, 7);

                fieldSpecs[i] = fs;
            }

            return { sfs,fieldSpecs,tabVer };
        }

        static ID caculate_page_size(const field_specs& fieldSpecs, uint64_t segSize)
        {   // 行和节以最小公倍数对齐
            uint64_t rowSize = assistant::multiple_m_not_less_than_n(32, (fieldSpecs.size() + 1) * 8);
            uint64_t pageByteSize = std::lcm(rowSize, segSize);
            return pageByteSize / segSize;
        }

        static uint64_t caculate_field_spec_tab_size(const field_specs& fieldSpecs)
        {
            uint64_t size = 0;
            for (const auto& fs : fieldSpecs)
                size += assistant::multiple_m_not_less_than_n(32, fs.name.size() + 1)
                    + assistant::multiple_m_not_less_than_n(32, fs.comment.size() + 1)
                    + 32;
            return size;
        }

        static uint64_t caculate_index_tab_size(const std::unordered_map<std::string, index_spec>& indexMap)
        {
            uint64_t size = 0;
            for (const auto& [k, v] : indexMap)
                size += assistant::multiple_m_not_less_than_n(32, k.size() + 1)
                    + assistant::multiple_m_not_less_than_n(32, v.comment.size() + 1)
                    + assistant::multiple_m_not_less_than_n(32, v.fieldSpecs.size() * 8)
                    + 32;
            return size;
        }

        void sync_header()
        {
            auto cstHdr = sfs.custom_header();
            uint64_t idxTabPtr, idxEntCnt;
            {
                std::vector<uint64_t> hdrDat(9, 0);
                assistant::memcpy(cstHdr.data() + 24, reinterpret_cast<byte*>(hdrDat.data()), hdrDat.size() * sizeof(uint64_t));
                assistant::byteswap::from_big_endian_vector(hdrDat);
                totalRecords = hdrDat[0];
                idxTabPtr = hdrDat[3];
                idxEntCnt = hdrDat[4];
                currentPackID = hdrDat[5];
            }

            for (uint64_t i = 0; i < idxEntCnt; i++)
            {
                std::vector<uint64_t> idxEntDat(4, 0);
                assistant::memcpy(cstHdr.data() + idxTabPtr + 32 * i, reinterpret_cast<byte*>(idxEntDat.data()), idxEntDat.size() * sizeof(uint64_t));
                assistant::byteswap::from_big_endian_vector(idxEntDat);

                index_spec idx;
                idx.name = std::string{ reinterpret_cast<char*>(cstHdr.data() + idxEntDat[0]) };
                idx.comment = std::string{ reinterpret_cast<char*>(cstHdr.data() + idxEntDat[1]) };
                idx.fieldSpecs.reserve(idxEntDat[3]);
                
                std::vector<uint64_t> idxFdIDs(idxEntDat[3], 0);
                assistant::memcpy(cstHdr.data() + idxEntDat[2], reinterpret_cast<byte*>(idxFdIDs.data()), idxFdIDs.size() * sizeof(uint64_t));
                assistant::byteswap::from_big_endian_vector(idxFdIDs);
                for (const auto& id : idxFdIDs)
                    idx.fieldSpecs.push_back(fieldSpecs[id]);
                
                indexMap[idx] = idx;
            }
        }

        void flush_header()
        {
            size_t fieldSpecTabSize = caculate_field_spec_tab_size(fieldSpecs);
            size_t indexTabSize = caculate_index_tab_size(indexMap);
            size_t cstHdrSize = headerSize + fieldSpecTabSize + indexTabSize;
            std::vector<byte> cstHdr(cstHdrSize, 0);

            {   // 写入标记和版本号
                assistant::memcpy(reinterpret_cast<const byte*>(headerMark.data()), cstHdr.data(), 16);
                uint64_t ver = assistant::byteswap::to_big_endian(headerVersion);
                assistant::memcpy(reinterpret_cast<byte*>(&ver), cstHdr.data() + 16, sizeof(uint64_t));
            }

            {   // 写入基本数据
                std::vector<uint64_t> hdrDat{
                    totalRecords,
                    headerSize, fieldSpecs.size(),
                    headerSize + caculate_field_spec_tab_size(fieldSpecs),indexMap.size(),
                    currentPackID, tabVer.first, tabVer.second, 0
                };
                assistant::byteswap::to_big_endian_vector(hdrDat);
                assistant::memcpy(reinterpret_cast<const byte*>(hdrDat.data()), cstHdr.data() + 24, hdrDat.size() * sizeof(uint64_t));
            }
            {   // 写入字段表
                uint64_t currentDataPos = headerSize + 32 * fieldSpecs.size();
                for (size_t i = 0; i < fieldSpecs.size(); i++)
                {
                    const auto& fs = fieldSpecs[i];
                    uint64_t nameSize = assistant::multiple_m_not_less_than_n(32, fs.name.size() + 1);
                    uint64_t commSize = assistant::multiple_m_not_less_than_n(32, fs.comment.size() + 1);

                    std::vector<uint64_t> entry(2, 0);
                    std::vector<char> name(nameSize, 0);
                    std::vector<char> comment(commSize, 0);

                    entry[0] = currentDataPos;
                    entry[1] = currentDataPos + nameSize;
                    assistant::byteswap::to_big_endian_vector(entry);

                    assistant::memcpy(fs.name.data(), name.data(), fs.name.size());
                    assistant::memcpy(fs.comment.data(), comment.data(), fs.comment.size());

                    assistant::memcpy(reinterpret_cast<byte*>(entry.data()), cstHdr.data() + headerSize + 32 * i, entry.size() * sizeof(uint64_t));
                    cstHdr[headerSize + 32 * i + 16] = static_cast<byte>(fs.type);
                    assistant::memcpy(fs.mark, cstHdr.data() + headerSize + 32 * i + 17, 7);

                    assistant::memcpy(reinterpret_cast<byte*>(name.data()), cstHdr.data() + currentDataPos, nameSize);
                    assistant::memcpy(reinterpret_cast<byte*>(comment.data()), cstHdr.data() + currentDataPos + nameSize, commSize);

                    currentDataPos += nameSize + commSize;
                }
            }

            {   // 写入索引表
                uint64_t idxTabPtr = headerSize + fieldSpecTabSize;
                uint64_t currentDataPos = idxTabPtr + 32 * indexMap.size();
                size_t i = 0;
                for (const auto& [k, v] : indexMap)
                {
                    uint64_t nameSize = assistant::multiple_m_not_less_than_n(32, k.size() + 1);
                    uint64_t commSize = assistant::multiple_m_not_less_than_n(32, v.comment.size() + 1);
                    uint64_t idtSize = assistant::multiple_m_not_less_than_n(32, v.fieldSpecs.size() * 8);

                    std::vector<uint64_t> entry(4, 0);
                    std::vector<char> name(nameSize, 0);
                    std::vector<char> comment(commSize, 0);
                    std::vector<uint64_t> idt(idtSize / 8, 0);

                    entry[0] = currentDataPos;
                    entry[1] = currentDataPos + nameSize;
                    entry[2] = currentDataPos + nameSize + commSize;
                    entry[3] = v.fieldSpecs.size();
                    assistant::byteswap::to_big_endian_vector(entry);

                    assistant::memcpy(k.data(), name.data(), k.size());
                    assistant::memcpy(v.comment.data(), comment.data(), v.comment.size());

                    for (size_t j = 0; j < v.fieldSpecs.size(); j++)
                        idt[j] = static_cast<uint64_t>(std::distance(fieldSpecs.begin(),
                            std::find_if(fieldSpecs.begin(), fieldSpecs.end(),
                                [&](const field_spec& fs) { return fs == v.fieldSpecs[j]; })));
                    assistant::byteswap::to_big_endian_vector(idt);

                    assistant::memcpy(reinterpret_cast<byte*>(entry.data()), cstHdr.data() + idxTabPtr + 32 * i, entry.size() * sizeof(uint64_t));
                    assistant::memcpy(reinterpret_cast<byte*>(name.data()), cstHdr.data() + currentDataPos, nameSize);
                    assistant::memcpy(reinterpret_cast<byte*>(comment.data()), cstHdr.data() + currentDataPos + nameSize, commSize);
                    assistant::memcpy(reinterpret_cast<byte*>(idt.data()), cstHdr.data() + currentDataPos + nameSize + commSize, idtSize);

                    currentDataPos += nameSize + commSize + idtSize;
                    i++;
                }
            }

            // 文件头落盘
            sfs.custom_header() = cstHdr;
            sfs.flush();
        }

    public:     // 公共接口
        // 信息相关
        virtual size_t size() const override { std::shared_lock sl{ smtx }; return sfs.data().size(); }   // 返回完整的数据大小

        virtual ID tab_size() const override { std::shared_lock sl{ smtx }; return totalRecords; }    // 返回表行数
        
        virtual ID page_size() const override { return pageSize; }   // 返回页大小（以记录数计）
        
        virtual field_specs fields() const override { return fieldSpecs; } // 返回完整的字段表

        virtual void clear()             // 清空所有数据, 包括表数据和索引数据
        { 
            std::unique_lock ul{ smtx }; 
            totalRecords = 0; 
            currentPackID = 0; 
            indexMap.clear(); 
            sfs.clear(); 
            sfs.data().resize(0);
            sfs.flush();
        }


        // 表数据相关
        virtual page rows(ID pageID) const override    // 返回包含指定页号的页
        {
            std::shared_lock sl{ smtx };
            if(pageID * pageSize >= totalRecords)
                throw exceptions::common("Page out of range");

            page pg{ pageID,pageID * pageSize,std::min(pageSize, totalRecords - pageID * pageSize) };
            pg.data.resize(pg.count * fieldSpecs.size(), field{});

            // 读取数据
            for (ID r = 0; r < pg.count; r++)
            {
                std::vector<uint64_t> rowData = sfs.read<uint64_t>(tableSectionName.data(), (pg.start + r) * rowSize, fieldSpecs.size() + 1);
                assistant::byteswap::from_big_endian_vector(rowData);
                for (size_t f = 0; f < fieldSpecs.size(); f++)
                {
                    const auto& fs = fieldSpecs[f];
                    uint64_t datapackNO = rowData[0];
                    switch (fs.type)
                    {
                    case field_spec::field_type::INT:
                    {
                        INT val = std::bit_cast<INT>(rowData[f + 1]);
                        pg.data[r * fieldSpecs.size() + f] = field{ val };
                        break;
                    }
                    case field_spec::field_type::FLOAT:
                    {
                        FLOAT val = std::bit_cast<FLOAT>(rowData[f + 1]);
                        pg.data[r * fieldSpecs.size() + f] = field{ val };
                        break;
                    }
                    case field_spec::field_type::INTS:
                    {
                        uint64_t rdata = rowData[f + 1];
                        uint64_t dataPtr = (rdata >> 32) & 0xFFFFFFFF;
                        uint64_t dataLen = rdata & 0xFFFFFFFF;

                        if(dataLen == 0)
                        {
                            pg.data[r * fieldSpecs.size() + f] = NOTHING{};
                            break;
                        }

                        INTS vals = sfs.read<INT>(std::format(dataSectionFmt.data(), datapackNO), dataPtr, dataLen);
                        assistant::byteswap::from_big_endian_vector(vals);
                        pg.data[r * fieldSpecs.size() + f] = field{ vals };
                        break;
                    }
                    case field_spec::field_type::FLOATS:
                    {
                        uint64_t rdata = rowData[f + 1];
                        uint64_t dataPtr = (rdata >> 32) & 0xFFFFFFFF;
                        uint64_t dataLen = rdata & 0xFFFFFFFF;

                        if (dataLen == 0)
                        {
                            pg.data[r * fieldSpecs.size() + f] = NOTHING{};
                            break;
                        }

                        std::vector<uint64_t> rvals = sfs.read<uint64_t>(std::format(dataSectionFmt.data(), datapackNO), dataPtr, dataLen);
                        assistant::byteswap::from_big_endian_vector(rvals);
                        FLOATS vals(dataLen, 0);
                        assistant::memcpy(reinterpret_cast<byte*>(rvals.data()), reinterpret_cast<byte*>(vals.data()), vals.size() * sizeof(uint64_t));
                        pg.data[r * fieldSpecs.size() + f] = field{ vals };
                        break;
                    }
                    case field_spec::field_type::BYTES:
                    {
                        uint64_t rdata = rowData[f + 1];
                        uint64_t dataPtr = (rdata >> 32) & 0xFFFFFFFF;
                        uint64_t dataLen = rdata & 0xFFFFFFFF;

                        if (dataLen == 0)
                        {
                            pg.data[r * fieldSpecs.size() + f] = NOTHING{};
                            break;
                        }

                        BYTES vals = sfs.read<BYTE>(std::format(dataSectionFmt.data(), datapackNO), dataPtr, dataLen);
                        pg.data[r * fieldSpecs.size() + f] = field{ vals };
                        break;
                    }
                    default:
                        pg.data[r * fieldSpecs.size() + f] = field{ std::monostate{} };
                        break;
                    }
                }
            }
            return pg;
        }

        virtual void rows(const page& pg) override // 写入整页数据
        {
            std::unique_lock ul{ smtx };
            if (pg.start + pg.count > totalRecords)
                totalRecords = pg.start + pg.count;

            // 如果不包含当前数据包节则创建
            if(!sfs.sec_contains(std::format(dataSectionFmt.data(), currentPackID)))
            {
                // 初始化数据包节（将packUsedSize写入节头）
                std::vector<uint32_t> dpHdr(8,0);
                dpHdr[0] = datPkgDatStartOffset;
                assistant::byteswap::to_big_endian_vector(dpHdr);
                sfs.write<uint32_t>(std::format(dataSectionFmt.data(), currentPackID), 0, dpHdr);
            }

            uint32_t currentPackUsedSize = assistant::byteswap::from_big_endian(
                sfs.read<uint32_t>(std::format(dataSectionFmt.data(), currentPackID), 0, 1)[0]);
            for (const auto& rid : pg.modified)
            {
                // 统计此行所需数据包空间（仅包含数组类型）
                uint64_t requiredPackSize = 0;
                for (size_t f = 0; f < fieldSpecs.size(); f++)
                {
                    const auto& fs = fieldSpecs[f];
                    try
                    {
                        switch (fs.type)
                        {
                        case field_spec::field_type::INTS:
                        {
                            if(std::holds_alternative<NOTHING>(pg.data[(rid - pg.start) * fieldSpecs.size() + f]))
                                break;

                            const auto& vals = std::get<INTS>(pg.data[(rid - pg.start) * fieldSpecs.size() + f]);
                            requiredPackSize += vals.size() * sizeof(INT);
                            break;
                        }
                        case field_spec::field_type::FLOATS:
                        {
                            if (std::holds_alternative<NOTHING>(pg.data[(rid - pg.start) * fieldSpecs.size() + f]))
                                break;

                            const auto& vals = std::get<FLOATS>(pg.data[(rid - pg.start) * fieldSpecs.size() + f]);
                            requiredPackSize += vals.size() * sizeof(uint64_t);
                            break;
                        }
                        case field_spec::field_type::BYTES:
                        {
                            if (std::holds_alternative<NOTHING>(pg.data[(rid - pg.start) * fieldSpecs.size() + f]))
                                break;

                            const auto& vals = std::get<BYTES>(pg.data[(rid - pg.start) * fieldSpecs.size() + f]);
                            requiredPackSize += vals.size() * sizeof(BYTE);
                            break;
                        }
                        default:
                            break;
                        }
                    }   // 坏数据保护：存储的数据类型与字段所需类型不匹配时，落盘数据为空
                    catch (const std::bad_variant_access&) {}
                }

                // 如果一行的数据量超过单个数据包容量则报错
                if (requiredPackSize > std::numeric_limits<uint32_t>::max())
                    throw exceptions::common("A single row's data exceeds the capacity of a single data package");

                // 检查数据包空间是否充足，如不充足则创建新数据包
                if (currentPackUsedSize + requiredPackSize > std::numeric_limits<uint32_t>::max())
                {
                    // 保存旧的数据包已用大小
                    sfs.write<uint32_t>(std::format(dataSectionFmt.data(), currentPackID), 0,
                        std::vector<uint32_t>{ assistant::byteswap::to_big_endian(currentPackUsedSize) });
                    // 切换到新数据包
                    currentPackID++;
                    currentPackUsedSize = datPkgDatStartOffset;
                    // 初始化新数据包节（将packUsedSize写入节头）
                    std::vector<uint32_t> dpHdr(8, 0);
                    dpHdr[0] = datPkgDatStartOffset;
                    assistant::byteswap::to_big_endian_vector(dpHdr);
                    sfs.write<uint32_t>(std::format(dataSectionFmt.data(), currentPackID), 0, dpHdr);
                }

                // 准备行数据
                std::vector<uint64_t> rowData(rowSize / 8, 0);
                rowData[0] = currentPackID;
                for (size_t f = 0; f < fieldSpecs.size(); f++)
                {
                    const auto& fs = fieldSpecs[f];
                    try
                    {
                        switch (fs.type)
                        {
                        case field_spec::field_type::INT:
                        {
                            if(std::holds_alternative<NOTHING>(pg.data[(rid - pg.start) * fieldSpecs.size() + f]))
                                break;

                            rowData[f + 1] = std::bit_cast<uint64_t>(std::get<INT>(pg.data[(rid - pg.start) * fieldSpecs.size() + f]));
                            break;
                        }
                        case field_spec::field_type::FLOAT:
                        {
                            if (std::holds_alternative<NOTHING>(pg.data[(rid - pg.start) * fieldSpecs.size() + f]))
                                break;

                            rowData[f + 1] = std::bit_cast<uint64_t>(std::get<FLOAT>(pg.data[(rid - pg.start) * fieldSpecs.size() + f]));
                            break;
                        }
                        case field_spec::field_type::INTS:
                        {
                            if (std::holds_alternative<NOTHING>(pg.data[(rid - pg.start) * fieldSpecs.size() + f]))
                                break;

                            const auto& vals = std::get<INTS>(pg.data[(rid - pg.start) * fieldSpecs.size() + f]);
                            // 写入数据包
                            std::vector<INT> beVals = vals;
                            assistant::byteswap::to_big_endian_vector(beVals);
                            sfs.write<INT>(std::format(dataSectionFmt.data(), currentPackID), currentPackUsedSize, beVals);
                            // 更新行数据
                            rowData[f + 1] = ((static_cast<uint64_t>(currentPackUsedSize) << 32) & 0xFFFFFFFF00000000) | (vals.size() & 0xFFFFFFFF);
                            currentPackUsedSize += static_cast<uint32_t>(vals.size()) * sizeof(INT);
                            break;
                        }
                        case field_spec::field_type::FLOATS:
                        {
                            if (std::holds_alternative<NOTHING>(pg.data[(rid - pg.start) * fieldSpecs.size() + f]))
                                break;

                            const auto& vals = std::get<FLOATS>(pg.data[(rid - pg.start) * fieldSpecs.size() + f]);
                            // 写入数据包
                            std::vector<uint64_t> rvals(vals.size(), 0);
                            assistant::memcpy(reinterpret_cast<const byte*>(vals.data()), reinterpret_cast<byte*>(rvals.data()), sizeof(uint64_t) * vals.size());
                            assistant::byteswap::to_big_endian_vector(rvals);
                            sfs.write<uint64_t>(std::format(dataSectionFmt.data(), currentPackID), currentPackUsedSize, rvals);
                            // 更新行数据
                            rowData[f + 1] = ((static_cast<uint64_t>(currentPackUsedSize) << 32) & 0xFFFFFFFF00000000) | (vals.size() & 0xFFFFFFFF);
                            currentPackUsedSize += static_cast<uint32_t>(vals.size()) * sizeof(uint64_t);
                            break;
                        }
                        case field_spec::field_type::BYTES:
                        {
                            if (std::holds_alternative<NOTHING>(pg.data[(rid - pg.start) * fieldSpecs.size() + f]))
                                break;

                            const auto& vals = std::get<BYTES>(pg.data[(rid - pg.start) * fieldSpecs.size() + f]);
                            // 写入数据包
                            sfs.write<BYTE>(std::format(dataSectionFmt.data(), currentPackID), currentPackUsedSize, vals);
                            // 更新行数据
                            rowData[f + 1] = ((static_cast<uint64_t>(currentPackUsedSize) << 32) & 0xFFFFFFFF00000000) | (vals.size() & 0xFFFFFFFF);
                            currentPackUsedSize += static_cast<uint32_t>(vals.size()) * sizeof(BYTE);
                            break;
                        }
                        default:
                            break;
                        }
                    }   // 坏数据保护：存储的数据类型与字段所需类型不匹配时，落盘数据为空
                    catch (const std::bad_variant_access&) {}
                }
                // 写入行数据
                assistant::byteswap::to_big_endian_vector(rowData);
                sfs.write<uint64_t>(tableSectionName.data(), rid * rowSize, rowData);
            }

            // 更新当前数据包已用大小
            sfs.write<uint32_t>(std::format(dataSectionFmt.data(), currentPackID), 0,
                std::vector<uint32_t>{ assistant::byteswap::to_big_endian(currentPackUsedSize) });
        }

        // 索引相关
        virtual index_specs indexies() const override            // 返回完整的索引表信息
        {
            std::shared_lock sl{ smtx };
            index_specs idxSpecs;
            idxSpecs.reserve(indexMap.size());
            for (const auto& [k, v] : indexMap)
                idxSpecs.push_back(v);
            return idxSpecs;
        }

        virtual void index_tab(index idx) override                      // 保存索引表（包含创建）
        {
            std::unique_lock ul{ smtx };

            // 更新索引元数据
            indexMap[idx.spec] = idx.spec;

            // 写入索引数据头
            sfs.write<uint64_t>(std::format(indexSectionFmt.data(), idx.spec.name), 0,
                std::vector<uint64_t>{ assistant::byteswap::to_big_endian(static_cast<uint64_t>(idx.data.size())), 0, 0, 0 });

            // 写入索引数据
            assistant::byteswap::to_big_endian_range(idx.data);
            for (ID i = 0; i < idx.data.size(); i += pageSize)
            {
                std::vector<ID> idxPage(idx.data.begin() + i,
                    idx.data.begin() + i + std::min(pageSize, static_cast<ID>(idx.data.size() - i)));
                sfs.write<ID>(std::format(indexSectionFmt.data(), idx.spec.name), idxDatStartOffset + i * sizeof(ID), idxPage);
            }
        }

        virtual index index_tab(const std::string& idxName) const override  // 加载索引表
        {
            std::shared_lock sl{ smtx };
            
            // 如果索引不存在则报错
            if (!indexMap.contains(idxName))
                throw exceptions::common("Index not found: " + idxName);

            // 读取索引数据头
            uint64_t idxRecCnt = assistant::byteswap::from_big_endian(
                sfs.read<uint64_t>(std::format(indexSectionFmt.data(), idxName), 0, 1)[0]);

            // 读取并返回数据
            index idx;
            idx.spec = indexMap.at(idxName);
            for(ID i = 0;i<idxRecCnt;i+=pageSize)
                idx.data.append_range(
                    sfs.read<ID>(std::format(indexSectionFmt.data(), idxName), idxDatStartOffset + i * sizeof(ID),
                        std::min(pageSize, idxRecCnt - i))
                );
            assistant::byteswap::from_big_endian_range(idx.data);
            return idx;
        }

    public:     // 管理接口
        void flush() { std::unique_lock ul{ smtx }; flush_header(); }

        sfstream& data() { return sfs; }

        const sfstream& data() const { return sfs; }

    private:    // 仅允许工厂构造
        single_loader(const sfstream& sfs, const field_specs& fieldSpecs, const std::pair<ID, ID>& ver)
            : sfs(sfs), fieldSpecs(fieldSpecs), pageSize(caculate_page_size(fieldSpecs, sfs.seg_size())),
            rowSize(assistant::multiple_m_not_less_than_n(32, (fieldSpecs.size() + 1) * 8)), tabVer(ver) {
        }

        single_loader(single_loader&&) = default;

    public:
        single_loader(const single_loader& oth)
            :sfs(oth.sfs), fieldSpecs(oth.fieldSpecs), pageSize(oth.pageSize), rowSize(oth.rowSize),
            totalRecords(oth.totalRecords), indexMap(oth.indexMap), currentPackID(oth.currentPackID),
            tabVer(oth.tabVer) {
        }

        virtual ~single_loader() { flush_header(); }

    public:     // 工厂方法
        // 方式 1：创建新文件，如果文件已存在则报错
        static single_loader make(
            const std::filesystem::path& p,
            field_specs fieldSpecs,
            std::pair<ID,ID> tabVersion,
            sfstream::segment_size_level segSizeLevel = sfstream::segment_size_level::III,
            uint64_t maxSegs = std::numeric_limits<uint64_t>::max()
        ) {
            single_loader sl(sfstream::make(p, segSizeLevel, maxSegs, 0), fieldSpecs, tabVersion);
            sl.flush_header();
            return sl;
        }

        static std::unique_ptr<single_loader> make_unique(
            const std::filesystem::path& p,
            field_specs fieldSpecs,
            std::pair<ID,ID> tabVersion,
            sfstream::segment_size_level segSizeLevel = sfstream::segment_size_level::III,
            uint64_t maxSegs = std::numeric_limits<uint64_t>::max()
        ) {
            std::unique_ptr<single_loader> psl{ new single_loader(sfstream::make(p, segSizeLevel, maxSegs, 0), fieldSpecs, tabVersion) };
            psl->flush_header();
            return psl;
        }

        // 方式 2：打开已有的文件
        static single_loader make(const std::filesystem::path& p)
        {
            auto [sfs, fieldSpecs, tabver] = check_and_extract_field_specs(p);
            single_loader sl{ sfs,fieldSpecs,tabver };
            sl.sync_header();
            return sl;
        }

        static std::unique_ptr<single_loader> make_unique(const std::filesystem::path& p) 
        {
            auto [sfs, fieldSpecs, tabver] = check_and_extract_field_specs(p);
            std::unique_ptr<single_loader> psl{ new single_loader(sfs, fieldSpecs, tabver) };
            psl->sync_header();
            return psl;
        }
    };

    

}



/*************** 合并自 logger.h ***************/
// #pragma once
// #include "framework.h"
// #include "pch.h"

// #include "log.h"

namespace HYDRA15::Union::secretary
{
    // 日志输出代理
    class logger
    {
        const std::string title;

    public:
        logger() = delete;
        logger(const logger&) = default;
        logger(const std::string& title) :title(title) {}

#define logf(type) template<typename ... Args> std::string type(const std::string& fstr, Args...args) { return log::type(title, std::vformat(fstr, std::make_format_args(args...))); }

        logf(info);
        logf(warn);
        logf(error);
        logf(fatal);
        logf(debug);
        logf(trace);

#undef logf
    };

#ifndef UNION_CREATE_LOGGER
#define UNION_CREATE_LOGGER() HYDRA15::Union::secretary::logger{__func__}
#endif // !UNION_CREATE_LOGGER

}




/*************** 合并自 ScanCenter.h ***************/
// #pragma once
// #include "framework.h"
// #include "pch.h"

// #include "background.h"
// #include "PrintCenter.h"
// #include "logger.h"

namespace HYDRA15::Union::secretary
{
    // 统一的输入接口
    // 支持带提示词的输入
    // 支持队列化等待输入
    // 支持程序发送的伪输入
    // 从此处获取输入仅支持getline
    // 用户输入时如果没有程序等待，则可选将输入发送至指定接口，或者将输入加入输入队列等待后续线程认领
    class ScanCenter : protected labourer::background
    {
        /***************************** 快速接口 *****************************/
    public:
        // 获取输入，可选在输入前展示提示词 promt，指定的 id 将在程序发送伪输入时作为识别依据
        static std::string getline(std::string promt = " > ", unsigned long long id = 0);
        static std::future<std::string> getline_async(std::string promt = " > ", unsigned long long id = 0);    // 非同步获取输入
        static void setline(std::string line, unsigned long long id = 0);    // 伪输入

        /***************************** 公有单例 *****************************/
    private:
        ScanCenter();

    public:
        ~ScanCenter();
        static ScanCenter& get_instance();

        /***************************** 系 统 *****************************/
    private:
        PrintCenter& pc = PrintCenter::get_instance();
        secretary::logger lgr{ "ScanCenter" };

        // 后台线程
    private:
        std::atomic<bool> working = true;
        virtual void work() noexcept override;

    private:    
        // 用于重定向输入
        std::function<std::string()> sysgetline;
        std::shared_ptr<std::istream> pSysInStream;
        std::shared_ptr<istreambuf> pSCIstreamBuf;
        // 当没有后台线程等待时，输入会自动路由到此处
        std::function<void(const std::string&)> assign = nullptr; 
        // 系统锁
        std::mutex sysLock;
        std::condition_variable_any syscv;

        // 如果用户输入时后台没有线程等待，系统将自动将输入内容发送至由此指定的接口
        // 如果没有指定，则将输入加入输入队列，等待后续线程认领
    public:
        void set_assign(std::function<void(const std::string&)> a);

        /***************************** 输入管理 *****************************/
    private:
        struct getline_request
        {
            unsigned long long id;  // 标记线程的id，推荐使用线程id，如果需要伪输入的话可以自行指定
            std::string promt;
            std::promise<std::string> prms;
            bool operator==(unsigned long long i);
            bool operator==(const getline_request& oth);
        };
        struct putline_request
        {
            unsigned long long id;
            std::string line;
            bool operator==(unsigned long long i);
            bool operator==(const putline_request& oth);
        };
        std::list<getline_request> getlineQueue;
        std::list<putline_request> setlineQueue;
        std::shared_mutex queueLock;



    };
}

