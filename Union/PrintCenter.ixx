export module HYDRA15.Union.PrintCenter;

import std;
import HYDRA15.Union.AtomicMutex;
import HYDRA15.Union.exceptions;

namespace HYDRA15::Union
{
    /**
    * @brief 输出中心
    * 提供滚动消息、底部消息两种输出方式
    * 默认输出到标准输出，支持重定向到其他输出流（如文件）
    */
    export class PrintCenter
    {
    public:
        using printer = void(const std::string&);
    public:
        template<typename ...Args>
        static void println(Args ... args) { std::stringstream ss; (ss << ... << args); instance.msg_enqueue(ss.str()); }
        template<typename ... Args>
        static void printf(const std::string& fstr, Args...args) { instance.msg_enqueue(std::vformat(fstr, std::make_format_args(args...))); }
        static void set(const std::string& str) { instance.msg_stick(str); }
        static void remove() { instance.msg_stick(""); }

    public:
        static PrintCenter& get_instance() { static PrintCenter instance; return instance; }
        // 修改此配置以重定向消息输出方式
        std::atomic<printer*> sysPrint{ &PrintCenter::default_print };
        // 修改此配置以调整输出刷新策略
        std::atomic<std::chrono::milliseconds> refreshInterval = std::chrono::milliseconds(100); // 刷新间隔
        
    private:
        template<typename T, size_t queueSize = 256 * 1024>
        class fixed_queue
        {
        public:
            using iterator = T*;
        public:
            void push_back(T&& item)
            {
                size_t idx = current.fetch_add(1, std::memory_order::relaxed);
                if (idx >= queueSize) throw queue_full_exception();
                data[idx] = std::forward<T>(item);
            }
            void push_back(const T& item)
            {
                size_t idx = current.fetch_add(1, std::memory_order::relaxed);
                if (idx >= queueSize) throw queue_full_exception();
                data[idx] = item;
            }
            void clear() { current.store(0, std::memory_order::relaxed); }
            bool empty() const { return current.load(std::memory_order::relaxed) == 0; }
            iterator begin() { return data; }
            iterator end() { return data + std::min(current.load(std::memory_order::relaxed), queueSize); }

        private:
            T data[queueSize];
            std::atomic<size_t> current{ 0 };

        public:
            class queue_full_exception : public exception
            {
            public:
                queue_full_exception() : exception("queue full") {};
            };
        };

        template<typename T>
        class dynamic_queue
        {
        public:
            void push_back(const T& item)
            {
                std::lock_guard lg(mutex);
                data.push_back(item);
            }
            void push_back(T&& item)
            {
                std::lock_guard lg(mutex);
                data.push_back(std::forward<T>(item));
            }
            void clear() { data.clear(); }
            bool empty() const { std::lock_guard lg(mutex); return data.empty(); }
            auto begin() { return data.begin(); }
            auto end() { return data.end(); }

        private:
            std::deque<T> data;
            mutable AtomicMutex mutex;
        };

        using msg_queue = dynamic_queue<std::string>;

    private:
        static inline PrintCenter& instance{ get_instance() };
        // 消息
        AtomicMutex queueMutex;
        std::unique_ptr<msg_queue> rollingFront = std::make_unique<msg_queue>();
        std::unique_ptr<msg_queue> rollingBack = std::make_unique<msg_queue>();
        std::string bottomMessage;
        std::atomic_bool bottomUpdated = false;
        // 工作线程
        std::atomic_bool working = true;
        std::latch startLatch{ 1 };
        std::jthread worker{ &PrintCenter::work, this };

        // 工具函数
        void msg_enqueue(const std::string& msg) { std::lock_guard lg(queueMutex); rollingFront->push_back(msg); }
        void msg_stick(const std::string& msg)
        {
            std::lock_guard lg(queueMutex);
            bottomMessage = msg;
            bottomUpdated.store(true, std::memory_order::relaxed);
        }
        static void default_print(const std::string& str)
        {
            std::cout << str;
        }

        void work()
        {
            startLatch.wait();
            while (working || !rollingFront->empty() || !rollingBack->empty())
            {
                if (rollingFront->empty() && rollingBack->empty() && !bottomUpdated.load(std::memory_order::relaxed))
                {
                    std::this_thread::sleep_for(refreshInterval.load(std::memory_order::relaxed));
                    continue;
                }
                std::string btm;
                {
                    std::lock_guard lg(queueMutex);
                    rollingFront.swap(rollingBack);
                    btm = bottomMessage;
                    bottomUpdated.store(false, std::memory_order::relaxed);
                }
                std::stringstream ss;
                ss << "\r\033[2K";
                for (const auto& item : *rollingBack)
                    ss << item << "\n";
                ss << btm;
                rollingBack->clear();
                sysPrint.load(std::memory_order::relaxed)(ss.str());
            }
        }

        PrintCenter() { startLatch.count_down(); }
        ~PrintCenter() { working = false; }
        PrintCenter(const PrintCenter&) = delete;
        PrintCenter(PrintCenter&&) = delete;

        
    };

    
}
