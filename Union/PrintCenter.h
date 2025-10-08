#pragma once
#include "framework.h"
#include "pch.h"

#include "secretary_exception.h"
#include "background.h"
#include "datetime.h"
#include "utility.h"
#include "registry.h"

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

        // 需要重定向时修改此变量
        std::function<void(const std::string&)> print = [](const std::string& str) {std::cout << str; };
        std::function<void(const std::string&)> printFile;

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
        void redirect(std::function<void(const std::string&)> printFunc);
        void fredirect(std::function<void(const std::string&)> fprintFunc);

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
        using btmmsg_tab = archivist::int_registry<btmmsg_ctrlblock>;
    public:
        using ID = btmmsg_tab::uint_index;

        // 数据
    private:
        std::string stickBtmMsg;
        btmmsg_tab btmMsgTab;
        std::mutex btmMsgTabLock;
        size_t lastBtmLines = 0;

        // 接口
    public:
        ID new_bottom(bool forceDisplay = false, bool neverExpire = false);
        void update_bottom(ID id, const std::string& content);
        bool check_bottom(ID id);
        void remove_bottom(ID id);
        void set_stick_btm(const std::string& str = std::string());


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
