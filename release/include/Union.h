// === Merged from: pch.h (precompiled header) ===
// // pch.h: 这是预编译标头文件。
// 下方列出的文件仅编译一次，提高了将来生成的生成性能。
// 这还将影响 IntelliSense 性能，包括代码完成和许多代码浏览功能。
// 但是，如果此处列出的文件中的任何一个在生成之间有更新，它们全部都将被重新编译。
// 请勿在此处添加要频繁更新的文件，这将使得性能优势无效。

#ifndef PCH_H
#define PCH_H

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
//#include <print>


#endif //PCH_H

// === Merged from: astring.h ===
#pragma once
// #include "pch.h"

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
                    return;
        }
    };
}

// === Merged from: framework.h ===
#pragma once

#define WIN32_LEAN_AND_MEAN             // 从 Windows 头文件中排除极少使用的内容


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
#define UNION_IEXPT_STACKTRACE_ENABLE 

// 默认线程池线程数目
#define UNION_DEFAULT_THREAD_COUNT std::thread::hardware_concurrency() / 2

namespace HYDRA15::Union
{
    // 全局 debug 变量
#ifdef _DEBUG
    inline bool debug = true;
#else 
    inline bool debug = false;
#endif
}

// === Merged from: background.h ===
#pragma once
// #include "pch.h"
// #include "framework.h"


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
        // 线程信息块和线程物品
    public:
        class thread_info
        {
        public:
            enum class state
            {
                undefined,
                idle,
                waiting,
                working,
                finishing
            }thread_state = state::undefined;
            std::chrono::steady_clock::time_point workStartTime;
            std::chrono::steady_clock::time_point lastCheckin;
        };

    private:
        class thread_ctrlblk
        {
        public:
            std::shared_ptr<std::thread> thread; // 线程对象
            std::thread::id thread_id; // 线程ID
            thread_info info; // 线程信息
        };

        class thread_info_guard
        {
            background::thread_info& thrInfo;
        public:
            thread_info_guard(background::thread_info& info);
            ~thread_info_guard();
        };

    private:
        std::barrier<> checkpoint;  //启动和结束同步
        std::list<thread_ctrlblk> threads; // 异步线程组
        void work_shell(thread_info& info); // 封装了启动与结束同步的工作函数

    public:
        virtual void work(thread_info& info) = 0;  // 重写此方法以异步执行

        // 启动同步和结束同步
    public:
        void start();
        void wait_for_end();

        // 构造函数，参数为异步线程数量，默认为1
    public:
        background(unsigned int bkgThrCount);
        background(background&&) = delete;
        background();
        virtual ~background();
        background(const background&) = delete;
        background& operator=(const background&) = delete;

        // 迭代器访问每一个线程信息
    public:
        class iterator
        {
            using list_iter = std::list<thread_ctrlblk>::iterator;
            list_iter it;
        public:
            iterator(list_iter iter);
            iterator& operator++();
            bool operator!=(const iterator& other) const;
            thread_info& operator*() const;
            std::thread::id get_id() const;
        };

        iterator begin();
        iterator end();
    };

}

// === Merged from: concepts.h ===
#pragma once
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
// === Merged from: iExceptionBase.h ===
#pragma once
// #include "pch.h"
// #include "framework.h"

// #include "astring.h"

namespace HYDRA15::Union::referee
{
	// 异常处理的基础
	// 相比标准库异常，额外记录了：
	//    - 描述字符串
	//    - 异常信息
    //    - （可选）调用栈
    using iException_code = unsigned int;
	class iExceptionBase :public std::exception
	{
		static_string baseWhatStrFormat = "iException: {0} ( 0x{1:08X} : 0x{2:08X} )";

        // 配置项
    public:
        inline static bool enableDebug = debug;

	protected:
		// what字符串缓存
		mutable std::string whatStr;

	public:
        // 记录的信息
        const iException_code libID;
        const iException_code exptCode;
        const std::string description;

		// 构造和析构
		iExceptionBase() noexcept = delete;
		iExceptionBase(const std::string& desp, const iException_code& id, const iException_code& code) noexcept;
		virtual ~iExceptionBase() noexcept = default;

		// what()
		virtual const char* what() const noexcept override;


		// 启用栈跟踪支持
#ifdef UNION_IEXPT_STACKTRACE_ENABLE
	private:
		static_string baseStackTraceFormat = "Stack Trace: \n{0}";
    protected:
        mutable std::string stackTraceStr;
	public:
        const std::stacktrace stackTrace;
        const char* stack_trace() const;
#endif // LIB_IEXPT_STACKTRACE_ENABLE
	};
}
// === Merged from: libID.h ===
#pragma once
// #include "pch.h"
// #include "framework.h"

// #include "astring.h"


namespace HYDRA15::Union::framework
{
    static_string libName = "HYDRA15.Union";
    static_string version = "ver.lib.beta.1.0.0";

    // 子系统代码
    static struct libID
    {
        static_uint Union = 0xA00;
        static_uint referee = 0xA01;
        static_uint labourer = 0xA02;
        static_uint archivist = 0xA03;
        static_uint expressman = 0xA04;
        static_uint secretary = 0xA05;
        static_uint assistant = 0xA06;
        static_uint commander = 0xA07;

    }libID;
}
// === Merged from: progress.h ===
#pragma once
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
        static std::string digital(std::string title, float progress);
        static std::string simple_bar(std::string title, float progress, unsigned int barWidth = 10, char barChar = '=');
    };

}
// === Merged from: secretary_streambuf.h ===
#pragma once
// #include "pch.h"
// #include "framework.h"


namespace HYDRA15::Union::secretary
{
    // AI 生成的代码
    // 自定义输出流缓冲区，自动将缓冲区内容传递给PrintCenter，用于重定向 std::cout
    class ostreambuf : public std::streambuf {
    public:
        explicit ostreambuf(std::function<void(const std::string&)> callback, std::size_t initial_size = 256, std::size_t max_size = 65536);

    protected:
        int_type overflow(int_type ch) override;

        std::streamsize xsputn(const char* s, std::streamsize n) override;

        int sync() override;

    private:
        std::vector<char> buffer_;
        std::size_t max_size_;
        std::mutex mtx_;
        std::function<void(const std::string&)> callback;

        void expand_buffer();

        void flush_buffer();
    };

    // AI生成的代码
    // 自定义输入流缓冲区，用于将输入重定向至 Command 类，用于重定向 std::cin
    class istreambuf : public std::streambuf 
    {
    private:
        std::string buffer_;   // 使用 std::string 作为缓冲区（自动管理内存）
        bool eof_ = false;     // 是否已到逻辑 EOF

        // 回调函数：用于获取一行输入
        std::function<std::string()> getline_callback;

        // 从 Command::getline() 获取一行并加载到 buffer_
        bool refill();

    public:
        // 构造时传入回调
        explicit istreambuf(std::function<std::string()> callback);

        // 重写 underflow：单字符读取时调用
        int underflow() override;

        // 1. 优化批量读取：重写 xsgetn
        std::streamsize xsgetn(char* s, std::streamsize count) override;

        // 2. 支持 cin.getline()：确保换行符被正确消费
        //    标准要求：getline 会读取直到 '\n' 或缓冲区满，并丢弃 '\n'
        //    我们的 underflow/xsgetn 已提供 '\n'，因此无需额外处理
        //    但需确保：当遇到 EOF 且无 '\n' 时，仍能正确终止
        //    —— 这由标准库的 istream::getline 逻辑处理，我们只需提供字符流

        // 可选：重写 showmanyc() 以提示可用字符数（非必需，但可优化）
        std::streamsize showmanyc() override;

    };
}
// === Merged from: write_first_mutex.h ===
#pragma once
// #include "pch.h"
// #include "framework.h"


namespace HYDRA15::Foundation::labourer
{
    // 写优先锁满足条件：
    //    - 使用 std::shared_lock 上锁视为读取操作，使用 std::unique_lock 上锁视为写入操作
    //    - 同一时间允许多个读取操作存在，允许一个写入操作存在，写入操作和读取操作不能同时存在
    //    - 当有写入线程等待时，停止新的读取线程的进入，待已有读取线程都退出后，允许写入线程进入
    //    - 当所有写入线程退出后，允许新的读取线程进入
    // ***** 包含AI编写的代码，并且未经过验证 *****
    class write_first_mutex
    {
        std::mutex mutex;
        std::condition_variable readCond;
        std::condition_variable writeCond;
        unsigned int activeReaders = 0;
        unsigned int waitingWriters = 0;
        bool activeWriters = false;

    public:
        write_first_mutex() = default;
        ~write_first_mutex() = default;

        void lock();
        void unlock();
        bool try_lock();

        void lock_shared();
        void unlock_shared();
        bool try_lock_shared();



    };
}
// === Merged from: shared_container_base.h ===
#pragma once
// #include "pch.h"
// #include "framework.h"

// #include "concepts.h"

namespace HYDRA15::Union::labourer
{
    // 在调用容器接口时上锁
    //    - 标准调用不使用任何锁操作
    //    - 允许多个共享调用同时进行，共享调用进行时禁止独占调用进行
    //    - 独占调用进行时禁止任何其他调用进行
    //    - 独占调用和共享调用的准入调度由锁的类型决定
    // 模板参数：
    //   - Container: 容器类型
    //   - Lock: 锁类型，必须满足 std::mutex 的接口
    // 调用使用示例：
    //   - 无重载成员函数：call(&Container::func, args...)
    //   - 有重载成员函数：call(static_cast<Ret (Container::*)(Args&&...)>(&Container::func), args...)，须在static_cast中指定函数指针重载的版本
    template<class container, class container_lock>
        requires framework::lockable<container_lock>
    class shared_container_base
    {
        container container;

    protected:
        container_lock containerLock;  // 将锁对象暴露给外部，以实现更高级的操作

    public:
        virtual ~shared_container_base() = default;

        // 标准调用
        template<typename F, typename... Args>
        decltype(auto) call(F&& f, Args&&... args)
        {
            return std::invoke(std::forward<F>(f), container, std::forward<Args>(args)...);
        }

        template<typename F, typename... Args>
        decltype(auto) static_call(F&& f, Args&&... args)
        {
            return std::invoke(std::forward<F>(f), std::forward<Args>(args)...);
        }

        // 锁调用
        template<typename F, typename... Args>
        decltype(auto) call_locked(F&& f, Args&&... args)
        {
            std::lock_guard<container_lock> guard(containerLock);
            return std::invoke(std::forward<F>(f), container, std::forward<Args>(args)...);
        }

        template<typename F, typename... Args>
        decltype(auto) static_call_locked(F&& f, Args&&... args)
        {
            std::lock_guard<container_lock> guard(containerLock);
            return std::invoke(std::forward<F>(f), std::forward<Args>(args)...);
        }

        // 共享调用
        template<typename F, typename... Args>
        decltype(auto) call_shared(F&& f, Args&&... args)
        {
            if constexpr (framework::shared_lockable<container_lock>)
                std::shared_lock<container_lock> guard(containerLock);
            else
                std::lock_guard<container_lock> guard(containerLock);
            return std::invoke(std::forward<F>(f), container, std::forward<Args>(args)...);
        }

        template<typename F, typename... Args>
        decltype(auto) static_call_shared(F&& f, Args&&... args)
        {
            if constexpr (framework::shared_lockable<container_lock>)
                std::shared_lock<container_lock> guard(containerLock);
            else
                std::lock_guard<container_lock> guard(containerLock);
            return std::invoke(std::forward<F>(f), std::forward<Args>(args)...);
        }

        // 独占调用
        template<typename F, typename... Args>
        decltype(auto) call_unique(F&& f, Args&&... args)
        {
            if constexpr (framework::shared_lockable<container_lock>)
                std::unique_lock<container_lock> guard(containerLock);
            else
                std::lock_guard<container_lock> guard(containerLock);
            return std::invoke(std::forward<F>(f), container, std::forward<Args>(args)...);
        }

        template<typename F, typename... Args>
        decltype(auto) static_call_unique(F&& f, Args&&... args)
        {
            if constexpr (framework::shared_lockable<container_lock>)
                std::unique_lock<container_lock> guard(containerLock);
            else
                std::lock_guard<container_lock> guard(containerLock);
            return std::invoke(std::forward<F>(f), std::forward<Args>(args)...);
        }

        // 直接上锁
        auto lock() { return containerLock.lock(); }
        auto unlock() { return containerLock.unlock(); }
        auto try_lock() { return containerLock.try_lock(); }

        auto lock_shared()
        {
            if constexpr (framework::shared_lockable<container_lock>)
                return containerLock.lock_shared();
            else
                return containerLock.lock();
        }
        auto unlock_shared()
        {
            if constexpr (framework::shared_lockable<container_lock>)
                return containerLock.unlock_shared();
            else
                return containerLock.unlock();
        }
        auto try_lock_shared()
        {
            if constexpr (framework::shared_lockable<container_lock>)
                return containerLock.try_lock_shared();
            else
                return containerLock.try_lock();
        }
    };


}
// === Merged from: assistant_exception.h ===
#pragma once
// #include "framework.h"
// #include "pch.h"

// #include "iExceptionBase.h"
// #include "libID.h"

namespace HYDRA15::Union::exceptions
{
    class assistant : public referee::iExceptionBase
    {
    public:
        static struct iException_codes
        {
            static_uint assistant = 0x0000;
            static_uint DateTimeInvalidTimeZone = 0x001;
            static_uint UtilityInvalidChar = 0x002;
            static_uint PropretiesParseFaild = 0x003;
        }iExptCodes;

    private:
        static struct vslz
        {
            static_string assistant = "Unknown Assistant Exception";
            static_string DateTimeInvalidTimeZone = "DateTime: Invalid Time Zone";
            static_string UtilityInvalidChar = "Invalid character detected";
            static_string PropretiesParseFaild = "Faild to parse propreties.";
        }vslz;

    public:
        assistant(
            const std::string& desp = vslz.assistant.data(),
            const referee::iException_code& code = iExptCodes.assistant
        ) noexcept;
        assistant() = delete;
        virtual ~assistant() noexcept = default;

        // 快速创建异常
        static assistant make_exception(const referee::iException_code& exptCode = iExptCodes.assistant) noexcept;

        static assistant DateTimeInvalidTimeZone() noexcept;
        static assistant UtilityInvalidChar() noexcept;
        static assistant PropretiesParseFaild() noexcept;
    };
}
// === Merged from: expressman_exception.h ===
#pragma once
// #include "framework.h"
// #include "pch.h"

// #include "iExceptionBase.h"
// #include "libID.h"

namespace HYDRA15::Union::exceptions
{
    class expressman : public referee::iExceptionBase
    {
    public:
        static struct iException_codes
        {
            static_uint expressman = 0x0000;

            // 接口
            static_uint InterfaceUnknownExpt = 0xA00;
            static_uint InterfaceExtensionFunctionNotImplemented = 0xA01;
            static_uint InterfaceInvalidFrameSize = 0xA02;
            static_uint InterfaceDataTooLarge = 0xA03;
            static_uint InterfaceIncompleteData = 0xA04;
            static_uint InterfaceIllegalType = 0xA05;

            // 工厂
            static_uint FactoryUnknownExpt = 0xB00;
            static_uint FactoryContaminatedData = 0xB01;
            static_uint FactoryUnknownClass = 0xB02;

            // mail
            static_uint BasicMailUnknownException = 0xC00;
            static_uint BasicMailEmptyCollector = 0xC01;
            static_uint BasicMailRequirementNotMet = 0xC02;
        }iExptCodes;

    private:
        static struct vslz
        {
            static_string expressman = "Unknown Expressman Exception";

            // 接口
            static_string InterfaceUnknownExpt = "Unknown Interface exception";
            static_string InterfaceExtensionFunctionNotImplemented = "This extension function is not implemented";
            static_string InterfaceInvalidFrameSize = "Prama maxFrameSize is invalid";
            static_string InterfaceDataTooLarge = "The data to be packaged is too large";
            static_string InterfaceIncompleteData = "The obtained data is incomplete";
            static_string InterfaceIllegalType = "Input type not allowed";

            // 工厂
            static_string FactoryUnknownExpt = "Unknown Factory Exception";
            static_string FactoryContaminatedData = "The packet list contains archives from more than one class";
            static_string FactoryUnknownClass = "The constructorof specific class is not registered";

            // mail
            static_string BasicMailUnknownException = "Unknown expressman basic mail exception";
            static_string BasicMailEmptyCollector = "The specified collector does not exist";
            static_string BasicMailRequirementNotMet = "The requirement is not satisfied";
        }vslz;

    public:
        expressman(
            const std::string& desp = vslz.expressman.data(),
            const referee::iException_code& code = iExptCodes.expressman
        ) noexcept;
        expressman() = delete;
        virtual ~expressman() noexcept = default;

        // 快速创建异常
        static expressman make_exception(const referee::iException_code& exptCode = iExptCodes.expressman) noexcept;

        // 接口
        static expressman InterfaceUnknownExpt() noexcept;
        static expressman InterfaceExtensionFunctionNotImplemented() noexcept;
        static expressman InterfaceInvalidFrameSize() noexcept;
        static expressman InterfaceDataTooLarge() noexcept;
        static expressman InterfaceIncompleteData() noexcept;
        static expressman InterfaceIllegalType() noexcept;

        // 工厂
        static expressman FactoryUnknownExpt() noexcept;
        static expressman FactoryContaminatedData() noexcept;
        static expressman FactoryUnknownClass() noexcept;

        // basic mail
        static expressman BasicMailUnknownException() noexcept;
        static expressman BasicMailEmptyCollector() noexcept;
        static expressman BasicMailRequirementNotMet() noexcept;
    };
}
// === Merged from: labourer_exception.h ===
#pragma once
// #include "framework.h"
// #include "pch.h"

// #include "iExceptionBase.h"
// #include "libID.h"

namespace HYDRA15::Union::exceptions
{
    class labourer : public referee::iExceptionBase
    {
    public:
        static struct iException_codes
        {
            // 线程池
            static_uint threadLake = 0xA00;
            static_uint threadQueueFull = 0xA01;
            static_uint emptyTask = 0xA02;
        }iExptCodes;

    private:
        static struct visualize
        {
            // 线程池
            static_string thrdLakeExpt = "ThreadLake Exception";
            static_string thrdLakeQueueFull = "ThreadLake: Task queue is full";
            static_string thrdLakeEmptyTask = "ThreadLake: Empty task";
        }vslz;

    public:
        labourer(
            const std::string& desp = vslz.thrdLakeExpt.data(),
            const referee::iException_code& code = iExptCodes.threadLake
        ) noexcept;
        labourer() = delete;
        virtual ~labourer() noexcept = default;


        // 快速创建异常
        static labourer make_exception(const referee::iException_code& exptCode) noexcept;

        static labourer TaskQueueFull() noexcept;
        static labourer EmptyTask() noexcept;

    };
}
// === Merged from: secretary_exception.h ===
#pragma once
#pragma once
// #include "framework.h"
// #include "pch.h"

// #include "iExceptionBase.h"
// #include "libID.h"

namespace HYDRA15::Union::exceptions
{
    class secretary : public referee::iExceptionBase
    {
    public:
        static struct iException_codes
        {
            static_uint secretary = 0x0000;

            // 打印中心
            static_uint printCenter = 0xA00;
            static_uint printBtmMsgFull = 0xA01;
            static_uint printBtmMsgNotFound = 0xA02;
        }iExptCodes;

    private:
        static struct visualize
        {
            static_string secretary = "Unknown Secretary Exception";

            // 打印中心
            static_string printCenter = "Unknown PrintCenter Exception";
            static_string printBtmMsgFull = "PrintCenter: Bottom message limit reached";
            static_string printBtmMsgNotFound = "PrintCenter: Bottom message ID not found";
        }vslz;

    public:
        secretary(
            const std::string& desp = vslz.secretary.data(),
            const referee::iException_code& code = iExptCodes.secretary
        ) noexcept;
        secretary() = delete;
        virtual ~secretary() noexcept = default;


        // 快速创建异常
        static secretary make_exception(const referee::iException_code& exptCode) noexcept;

        static secretary PrintCenterUnknown() noexcept;
        static secretary PrintCenterBtmMsgFull() noexcept;
        static secretary PrintCenterBtmMsgNotFound() noexcept;


    };
}
// === Merged from: datetime.h ===
#pragma once
// #include "framework.h"
// #include "pch.h"

// #include "assistant_exception.h"

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
        datetime();
        datetime(time_t t);
        datetime(const datetime&) = default;
        datetime& operator=(const datetime&) = default;
        ~datetime() = default;

        // 输出
    public:
        std::string date_time(std::string format = "%Y-%m-%d %H:%M:%S", int timeZone = localTimeZone) const;

        // 静态工具函数
    public:
        static datetime now();
        static std::string now_date_time(std::string format = "%Y-%m-%d %H:%M:%S", int timeZone = localTimeZone);
    };
}
// === Merged from: utility.h ===
#pragma once
// #include "framework.h"
// #include "pch.h"

// #include "assistant_exception.h"

namespace HYDRA15::Union::assistant
{
    std::string operator*(std::string str, size_t count);

    // 删除头尾的空字符，默认删除所有非可打印字符和空格
    inline std::string strip_front(
        const std::string& str,
        std::function<bool(char)> is_valid = [](char c) {return c > 0x20 && c < 0x7F; }
    );

    inline std::string strip_back(
        const std::string& str,
        std::function<bool(char)> is_valid = [](char c) {return c > 0x20 && c < 0x7F; }
    );

    std::string strip(     
        const std::string& str,
        std::function<bool(char)> is_valid = [](char c) {return c > 0x20 && c < 0x7F; }
    );

    // 删除字符串中所有的空字符，默认删除所有非可打印字符和空格
    std::string strip_all(  
        const std::string& str,
        std::function<bool(char)> is_valid = [](char c) {return c > 0x20 && c < 0x7F; }
    );

    // 删除字符串中的ansi颜色格式
    std::string strip_color(const std::string& str);

    // 删除字符串中所有的 ansi 转义串
    // ansi 转义串以 \0x1b 开始，任意字母结束
    std::string strip_ansi_secquence(const std::string& str);

    // 检查字符串内容是否全部合法，如不合法则报错
    void check_content(
        const std::string& str,
        std::function<bool(char)> is_valid = [](char c) {return c > 0x20 && c < 0x7F; }
    );

    // 用给定的字符切分字符串
    std::list<std::string> split_by(const std::string& str, const std::string& delimiter = " ");

    std::list<std::string> split_by(const std::string& str, const std::list<std::string>& deliniters);


    //向控制台输出十六进制的原始数据和对应的ascii字符
    std::string hex_heap(const unsigned char* pBegin, unsigned int size, const std::string& title = "Hex Heap", unsigned int preLine = 32);
    
    // 解析 propreties 文件
    // 强制要求键值分隔符为 = ，unicode字符保持原样，
    std::unordered_map<std::string, std::string> parse_propreties(const std::string& ppts);

    // 内存拷贝
    template<typename T>
    void memcpy(const T* src, T* dest, size_t size)
    {
        for (size_t i = 0; i < size; i++)
            dest[i] = src[i];
    }


    // 打印多个参数到控制台
    template<typename ... Args>
    std::ostream& print(Args ... args)
    {
        return (std::cout << ... << args);
    }


}
// === Merged from: ThreadLake.h ===
#pragma once
// #include "pch.h"
// #include "framework.h"

// #include "Background.h"
// #include "labourer_exception.h"
// #include "concepts.h"


namespace HYDRA15::Union::labourer
{

    // 线程池
    class ThreadLake :background
    {
        // 任务和任务包定义
    public:
        struct package
        {
            std::function<void()> content;
            std::function<void()> callback;	// 任务完成后的回调
        };

    private:
        // 回调函数壳
        template<typename ret_type>
        void callback_shell(std::function<void(ret_type)> callback, std::shared_future<ret_type> sfut)
        {
            try { callback(sfut.get()); }
            catch (...) { return; }
        }

        //任务队列
    private:
        std::queue <package> taskQueue; //任务队列
        const size_t tskQueMaxSize = 0; //任务队列最大大小，0表示无限制
        std::mutex queueMutex;
        std::condition_variable queueCv;

        //后台任务
    private:
        bool working = false;
        virtual void work(background::thread_info& info) override;

        //接口
    public:
        ThreadLake(unsigned int threadCount, size_t tskQueMaxSize = 0);
        ThreadLake() = delete;
        ThreadLake(const ThreadLake&) = delete;
        ThreadLake(ThreadLake&&) = delete;
        virtual ~ThreadLake();

        //提交任务
        // 方法1：提交任务函数 std::function 和回调函数 std::function，推荐使用此方法
        template<typename ret_type>
        auto submit(const std::function<ret_type()>& content, const std::function<void(ret_type)>& callback = std::function<void(ret_type)>())
            -> std::shared_future<ret_type>
        {
            if (!content)
                throw exceptions::labourer::EmptyTask();

            auto pkgedTask = std::make_shared<std::packaged_task<ret_type()>>(content);
            auto sfut = pkgedTask->get_future().share();

            // 插入任务包
            {
                std::lock_guard<std::mutex> lock(queueMutex);
                if (tskQueMaxSize != 0 && taskQueue.size() >= tskQueMaxSize) // 队列已满
                    throw exceptions::labourer::TaskQueueFull();
                taskQueue.push(
                    {
                        std::function<void()>([pkgedTask] { (*pkgedTask)(); }),
                        callback ? [sfut, callback]() {callback(sfut.get()); } : std::function<void()>{}
                    }
                );
                queueCv.notify_one();
            }

            return sfut;
        }

        // 方法1特化的无返回值版本
        std::future<void> submit(const std::function<void()>& content, const std::function<void()>& callback = std::function<void()>{});

        //方法2：直接提交任务包
        void submit(const package& taskPkg);

        // 方法3：提交裸函数指针和参数，不建议使用此方法，仅留做备用
        template<typename F, typename ... Args>
        auto submit(F&& f, Args &&...args)
            -> std::future<std::invoke_result_t<F, Args...>>
        {
            using return_type = typename std::invoke_result<F, Args...>::type;

            auto pkgedTask =
                std::make_shared<std::packaged_task<return_type()>>(
                    std::bind(std::forward<F>(f), std::forward<Args>(args)...)
                );

            // 插入任务包
            {
                std::lock_guard<std::mutex> lock(queueMutex);
                if (tskQueMaxSize != 0 && taskQueue.size() >= tskQueMaxSize) // 队列已满
                    throw exceptions::labourer::TaskQueueFull();

                taskQueue.push(
                    {
                        std::function<void()>([pkgedTask] { (*pkgedTask)(); }),
                        std::function<void()>()
                    }
                );
                queueCv.notify_one();
            }

            return pkgedTask->get_future();
        }

        // 迭代器访问每一个线程信息
        using background::iterator;
        using background::begin;
        using background::end;
    };
}

// === Merged from: expressman_interfaces.h ===
#pragma once
// #include "framework.h"
// #include "pch.h"

// #include "concepts.h"
// #include "expressman_exception.h"
// #include "utility.h"

namespace HYDRA15::Union::expressman
{
    // 用于类型擦除的基础接口
    class notype
    {
    public:
        virtual ~notype() = default;

        virtual std::shared_ptr<notype> clone() const = 0;    // 类型擦除状态下的拷贝构造
        virtual constexpr const std::type_info& type() const noexcept = 0;    // 返回类型信息，基本上就是 typeid(class)

        // 扩展接口
        virtual std::string info() const;
        virtual size_t class_size() const;    // 获取类本身的大小，不包含对象管理的外部数据，基本上就是 size_of(class)
    };

    /***************************** 数据传输相关 *****************************/
// 交换数据用的帧格式
// 一帧大小为 4KB ，其中 96B 头，4000B数据
// 数据区集中排布在末尾，因此可以放弃传输尾部没有有效数据的部分
    struct packet
    {
        using byte = uint8_t;
        using uint = uint16_t;
        static constexpr size_t maxClassNameSize = 88;
        static constexpr size_t maxFrameDataSize = 4000;

        struct header_t
        {
            uint frameNo = 0;
            uint frameTotal = 0;
            uint serialNo = 0;
            uint dataLength = 0;
            char className[maxClassNameSize] = {};
        }header;
        byte data[maxFrameDataSize] = {};
    };

    // 可将对象内部的全部数据打包成标准的帧，用于数据交换和数据持久化
    class packable
    {
    public:
        using datablock = std::vector<packet::byte>;
        using datablocks = std::list<datablock>;
        using class_name_array = std::array<char, packet::maxClassNameSize>;
        using objects = std::list<std::shared_ptr<packable>>;
    protected:
        // 用户仅需实现 将数据按照自己的设计打包成连续的二进制数据
        // 将内存数据打包成帧的工作将交由系统完成
        virtual datablocks packing() const = 0;
        virtual std::any unpacking(const datablocks&) = 0;
        virtual constexpr class_name_array class_name_pack() const = 0;    // 返回的类名称将填入className字段，作为类的唯一标识符，在重构对象时使用
        virtual packet::uint serialNo_fetch_and_increase() const = 0;     // 用于标记对象实例的序列号，对于类而言应当是全局静态的

    public:
        virtual ~packable() = default;

        std::list<packet> pack(size_t maxFrameSize = sizeof(packet)) const;
        std::any unpack(const std::list<packet>& archives);

        // 扩展接口
        virtual size_t obj_size() const;    // 返回对象必需数据的总大小，为对象大小和被对象管理的数据大小之和
    };
    packable::objects unpack(const std::list<packet>& archives, std::function<packable::objects(const packable::datablocks&)> constructor);
    std::string extract_name(const packet& arch);

    // 远程数据传输的代理
    // 用于隐藏各种传输方式的底层细节
    class agent
    {
    public:
        virtual bool send(const std::list<packet>& archives) const = 0;
        virtual std::list<packet> recv() const = 0;
        virtual std::list<packet> try_recv() const = 0;

        virtual ~agent() = default;
    };


    /***************************** 数据传递相关 *****************************/
    // 可被投递的接口
    // 模板参数为地址类型
    template<framework::hash_key A>
    class postable : virtual public notype
    {
    public:
        virtual A origin() const { return std::string(); }  // 指示源地址
        virtual A destination() const = 0;   // 指示目标地址

        // 可选：指定路由路径时使用
        // 使得指示目的地的指针在路由路径列表中向后移动一位，如果成功返回true，
        // 否则，如不存在路由路径或者已经达到最终目的地，返回false
        virtual bool next_route() const { return false; }

        virtual ~postable() = default;
    };


    // 实现此接口的类可以接收 postable 或其子类
    // 用于在多级路由中抹除层级差异
    // 模板参数为地址类型
    template<framework::hash_key A>
    class collector
    {
    public:
        virtual unsigned int post(const std::shared_ptr<const postable<A>>& pkg) = 0;    

        virtual ~collector() = default;
    };

    
}
// === Merged from: PrintCenter.h ===
#pragma once
// #include "framework.h"
// #include "pch.h"

// #include "secretary_exception.h"
// #include "background.h"
// #include "datetime.h"
// #include "utility.h"
// #include "secretary_streambuf.h"

namespace HYDRA15::Union::secretary
{
    // 统一输出接口
    // 提供滚动消息、底部消息和写入文件三种输出方式
    // 提交消息之后，调用 notify() 方法通知后台线程处理，这在连续提交消息时可以解约开销
    class PrintCenter final :labourer::background
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
        static bool update(unsigned long long id, const std::string& str);
        static bool remove(unsigned long long id);
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
            static_string btmMoreFormat = " ... and {0} more";
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
        virtual void work(background::thread_info&) override;

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

// === Merged from: basic_mailbox.h ===
#pragma once
// #include "framework.h"
// #include "pch.h"

// #include "concepts.h"
// #include "expressman_interfaces.h"

namespace HYDRA15::Union::expressman
{
    // 用于收取并存储 package 的容器
    template<framework::hash_key A>
    class basic_mailbox :virtual public collector<A>
    {
    protected:
        std::list<std::shared_ptr<const postable<A>>> lst;
        std::mutex lk;
        std::condition_variable cv;
        
    public:
        basic_mailbox() = default;
        basic_mailbox(const basic_mailbox&) = delete;
        basic_mailbox(basic_mailbox&&) = delete;
        virtual ~basic_mailbox() = default;

        virtual unsigned int post(const std::shared_ptr<const postable<A>>& pkg) override
        {
            std::unique_lock ul(lk);
            lst.push_back(pkg);
            return 1;
        }

        std::shared_ptr<const postable<A>> fetch()
        {
            std::unique_lock ul(lk);
            while (lst.empty())
                cv.wait(ul);
            std::shared_ptr<const postable<A>> ptr = lst.front();
            lst.pop_front();
            return ptr;
        }
        std::list<std::shared_ptr<const postable<A>>> fetch_all()
        {
            std::unique_lock ul(lk);
            while (lst.empty())
                cv.wait(ul);
            std::list<std::shared_ptr<const postable<A>>> ppkgs;
            lst.swap(ppkgs);
            return ppkgs;
        }

        size_t size() { return lst.size(); }
        bool empty() { return lst.empty(); }
        void clear()
        {
            std::unique_lock ul(lk);
            return lst.clear();
        }
    };
}
// === Merged from: basic_mailrouter.h ===
#pragma once
// #include "framework.h"
// #include "pch.h"

// #include "concepts.h"
// #include "expressman_interfaces.h"
// #include "background.h"

namespace HYDRA15::Union::expressman
{
    // 用于根据地址投递到下一站
    // 支持单播与组播。单播地址和组播地址的类型相同，但是不应当重合
    template<framework::hash_key A>
    class basic_mailrouter :virtual public collector<A>
    {
    public:
        using router_map = std::unordered_map<A, std::weak_ptr<collector<A>>>;
        using group_router_map = std::unordered_map<A, std::function<std::generator<A>()>>;
        

    protected:
        router_map rmap; // 路由表，地址 -> 下一站
        group_router_map grmap;  // 组播路由表，组地址 -> 地址列表
        std::shared_mutex smt;

    public:
        basic_mailrouter() = default;
        basic_mailrouter(router_map rm, group_router_map grm = group_router_map()) :rmap(rm), grmap(grm) {  }
        basic_mailrouter(const basic_mailrouter&) = delete;
        basic_mailrouter(basic_mailrouter&&) = delete;
        virtual ~basic_mailrouter() = default;

        virtual unsigned int post(const std::shared_ptr<const postable<A>>& pkg) override
        {
            std::shared_lock slk(smt);
            pkg->next_route();
            unsigned int posted = 0;
            A dest = pkg->destination();

            // 匹配组播路由
            const auto& grit = grmap.find(dest);    
            if (grit != grmap.end())
                for (const auto& i : (grit->second)())
                {
                    const auto& rit = rmap.find(i);
                    if (rit != rmap.end() && !rit->second.expired())
                    {
                        rit->second.lock()->post(std::dynamic_pointer_cast<const postable<A>>(pkg->clone()));  // 组播路由创建新的对象
                        posted++;
                    }
                }

            // 匹配单播路由
            const auto& rit = rmap.find(dest);
            if (rit != rmap.end() && !rit->second.expired())
            {
                rit->second.lock()->post(pkg);
                posted++;
            }
            return posted;
        }

        // 修改路由表接口
        bool add(A addr, const std::weak_ptr<collector<A>>& pc)  // 添加路由表项
        {
            std::unique_lock ul(smt);
            if (rmap.contains(addr))
                return false;
            rmap[addr] = pc;
            return true;
        }
        bool add(A addr, std::function<std::generator<A>()> gener) // 添加组播路由表项
        {
            std::unique_lock ul(smt);
            if (grmap.contains(addr))
                return false;
            grmap[addr] = gener;
            return true;
        }
        bool add(A addr, const std::list<A>& lst)   // 通过列表添加组播路由表项
        {
            std::unique_lock ul(smt);
            if (grmap.contains(addr))
                return false;
            grmap[addr] = [lst = lst]() -> std::generator<A> {for (const auto& i : lst)co_yield i; };
            return true;
        }
        void cleanup()  // 清理失效的路由表项
        {
            std::unique_lock ul(smt);
            auto it = rmap.begin();
            while (it != rmap.end())
                if (it->second.expired())
                    it = rmap.erase(it);
                else
                    it++;
        }

        // 高级管理接口
        router_map& fetch_rmap() { return rmap; }   // 直接获取整个路由表
        group_router_map& fetch_grmap() { return grmap; }   // 直接获取整个路由表
        auto lock() { return smt.lock(); }
        auto unlock() { return smt.unlock(); }
        auto try_lock() { return smt.try_lock(); }
    };
}
// === Merged from: factory.h ===
#pragma once
#pragma once
// #include "framework.h"
// #include "pch.h"

// #include "concepts.h"
// #include "expressman_exception.h"
// #include "expressman_interfaces.h"

namespace HYDRA15::Union::expressman
{
    // 用于从数据包构造对象的工厂
    // 应当在使用可打包对象之前向工厂注册构造函数
    class factory
    {
    public:
        using constructor = std::function<packable::objects(packable::datablocks)>;
        using constructor_tab = std::unordered_map<std::string, constructor>;

    private:
        constructor_tab ct;
        std::shared_mutex smt;

    public:
        packable::objects build(const std::list<packet>& archlst);    // 构造对象，给定的列表中只能由一个类，并且需要有完整的数据

        void regist(std::string name, const std::function<packable::objects(packable::datablocks)>& constructor);  // 注册构造函数
        bool unregist(std::string name);    // 移除构造函数
        bool contains(std::string name);    // 检查构造函数
    };
}
// === Merged from: log.h ===
#pragma once
// #include "framework.h"
// #include "pch.h"

// #include "secretary_exception.h"
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
            static_string info = "\033[0m[ {0} | INFO ] [ {1} ] {2}\033[0m";
            static_string warn = "\033[0m[ {0} | \033[33mWARN\033[0m ] [ {1} ] {2}\033[0m";
            static_string error = "\033[0m[ {0} | \033[35mERROR\033[0m ][ {1} ] {2}\033[0m";
            static_string fatal = "\033[0m[ {0} | \033[31mFATAL\033[0m ][ {1} ] \033[31m{2}\033[0m";
            static_string debug = "\033[0m[ {0} | \033[2mDEBUG\033[0m ][ {1} ] {2}\033[0m";
            static_string trace = "\033[0m[ {0} | \033[34mTRACE\033[0m ][ {1} ] {2}\033[0m";
        }vslz;

        
        
        // 公有接口
    public:
        static std::string info(const std::string& title, const std::string& content);
        static std::string warn(const std::string& title, const std::string& content);
        static std::string error(const std::string& title, const std::string& content);
        static std::string fatal(const std::string& title, const std::string& content);
        static std::string debug(const std::string& title, const std::string& content);
        static std::string trace(const std::string& title, const std::string& content);

        // 配置项
    public:
        inline static std::function<void(const std::string&)> print;
        static inline bool enableDebug = HYDRA15::Union::debug;
    };
}
// === Merged from: basic_mailsender.h ===
#pragma once
#pragma once
// #include "framework.h"
// #include "pch.h"

// #include "concepts.h"
// #include "expressman_interfaces.h"
// #include "background.h"
// #include "archivist_interfaces.h"
// #include "factory.h"

namespace HYDRA15::Union::expressman
{
    // 将消息序打包成数据包后发送至远程
    // 要求数据包也实现了 packable 接口
    template<framework::hash_key A>
    class basic_mailsender : virtual public collector<A>, public labourer::background
    {
    protected:
        std::shared_ptr<factory> pFactory = nullptr;
        std::shared_ptr<agent> pRemoteAgent = nullptr;   // 用于发送数据的代理
        std::weak_ptr<collector<A>> pEmployer = nullptr;          // 接收远程数据的雇主
        std::shared_mutex smt;
        std::condition_variable_any cv;
        std::chrono::microseconds queryInterval = std::chrono::microseconds(1000);
        bool working;

    public:
        basic_mailsender() = delete;
        basic_mailsender(
            const std::shared_ptr<agent>& pra, 
            const std::shared_ptr<collector<A>>& pe = nullptr,
            const std::shared_ptr<factory> pf = nullptr
        )
            :pRemoteAgent(pra), pEmployer(pe), pFactory(pf), labourer::background(1)
        {
            start();
        }
        basic_mailsender(const basic_mailsender&) = delete;
        basic_mailsender(basic_mailsender&&) = delete;
        virtual ~basic_mailsender() { working = false; cv.notify_all(); wait_for_end(); }

        virtual unsigned int post(const std::shared_ptr<const postable<A>>& pkg) override
        {
            std::shared_lock slk(smt);
            if (pRemoteAgent = nullptr)
                return false;
            std::shared_ptr<const packable> pp = std::dynamic_pointer_cast<const packable>(pkg);
            if (pp == nullptr)
                throw exceptions::expressman::BasicMailRequirementNotMet();
            return pRemoteAgent->send(pp->pack());
        }

        void set_remote_agent(const std::shared_ptr<agent>& pra) { std::unique_lock ulk(smt); pRemoteAgent = pra; }
        void set_employer(const std::weak_ptr<collector<A>>& pr) { std::unique_lock ulk(smt); pEmployer = pr; cv.notify_all(); }
        void set_factory(const std::shared_ptr<factory> pf) { std::unique_lock ulk(smt); pFactory = pf; cv.notify_all(); }

        virtual void work(thread_info& info)
        {
            // 第一层 map 为 类名->数据包列表 的映射，第二层 map 为 序列号->数据包列表 的映射
            std::unordered_map<std::string, std::unordered_map<packet::uint, std::list<packet>>> cache;
            while (working)
            {
                std::shared_lock slk(smt);
                cv.wait_for(slk, queryInterval);
                while ((pEmployer.expired() || !pFactory ) && working)
                    cv.wait(slk);

                // 接收数据
                std::list<packet> lst = pRemoteAgent->try_recv();
                for (const auto& i : lst)
                    cache[extract_name(i)][i.header.serialNo].push_back(i);

                // 解析数据
                std::shared_ptr<collector<A>> pE = pEmployer.lock();
                if (!pE)
                    continue;
                for (const auto& [name, clst] : cache)
                    for (const auto& [serNo, lst] : clst)
                    {
                        try
                        {
                            packable::objects objs = pFactory->build(lst);
                            for (const auto& pobj : objs)
                            {
                                std::shared_ptr<const postable<A>> p = std::dynamic_pointer_cast<const postable<A>>(pobj);
                                if (p == nullptr)
                                    continue;
                                pE->post(p);
                            }
                            cache[name].erase(serNo);
                        }
                        catch (exceptions::archivist& e)
                        {
                            if (e.exptCode == e.iExptCodes.InterfaceIncompleteData)
                                continue;
                            throw e;
                        }
                    }
            }
        }
    };
}
// === Merged from: logger.h ===
#pragma once
// #include "framework.h"
// #include "pch.h"

// #include "secretary_exception.h"
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
        logger(const std::string&);

        std::string info(const std::string& content);
        std::string warn(const std::string& content);
        std::string error(const std::string& content);
        std::string fatal(const std::string& content);
        std::string debug(const std::string& content);
        std::string trace(const std::string& content);

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

// === Merged from: ScanCenter.h ===
#pragma once
// #include "framework.h"
// #include "pch.h"

// #include "secretary_exception.h"
// #include "background.h"
// #include "utility.h"
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
        static std::string getline(std::string promt = vslz.promt.data(), unsigned long long id = 0);
        static std::future<std::string> getline_async(std::string promt = vslz.promt.data(), unsigned long long id = 0);    // 非同步获取输入
        static void setline(std::string line, unsigned long long id = 0);    // 伪输入

        /***************************** 公有单例 *****************************/
    private:
        ScanCenter();

    public:
        ~ScanCenter();
        static ScanCenter& get_instance();

        /***************************** 系 统 *****************************/
    private:
        static struct visualize
        {
            static_string promt = " > ";
        }vslz;
    private:
        PrintCenter& pc = PrintCenter::get_instance();
        secretary::logger lgr{ "ScanCenter" };

        // 后台线程
    private:
        std::atomic<bool> working = true;
        virtual void work(thread_info& info) override;

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
