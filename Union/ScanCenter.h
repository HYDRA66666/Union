#pragma once
#include "framework.h"
#include "pch.h"

#include "secretary_exception.h"
#include "background.h"
#include "utility.h"
#include "registry.h"
#include "PrintCenter.h"
#include "logger.h"

namespace HYDRA15::Union::secretary
{
    // 统一的输入接口
    // 支持带提示词的输入
    // 支持队列化等待输入
    // 支持程序发送的伪输入
    // 从此处获取输入仅支持getline
    class ScanCenter : protected labourer::background
    {
        /***************************** 快速接口 *****************************/
    public:
        static std::string getline(std::string promt = vslz.promt.data(), unsigned long long id = 0);
        static std::future<std::string> getline_async(std::string promt = vslz.promt.data(), unsigned long long id = 0);
        static void setline(std::string line, unsigned long long id = 0);    // 伪输入

        /***************************** 公有单例 *****************************/
    private:
        ScanCenter(bool waitForSignal);

    public:
        ~ScanCenter();
        static ScanCenter& get_instance(bool waitForSignal = false);

        /***************************** 系 统 *****************************/
    private:
        static struct visualize
        {
            static_string promt = " > ";
        }vslz;
    private:
        PrintCenter& pc = PrintCenter::get_instance();
        secretary::logger lgr{ "ScanCenter" };

        std::atomic<bool> working = false;

        virtual void work(thread_info& info) override;

    private:    
        // 用于重定向输入
        std::function<std::string()> sysgetline = []() {std::string line; std::getline(std::cin, line); return line; };
        std::string defaultPromt = " > ";
        // 当没有后台线程等待时，输入会自动路由到此处
        std::function<void(const std::string&)> sysassign = nullptr;  

    public:
        void set_getline(std::function<std::string()> g);
        void set_defaultPromt(const std::string& promt);
        void set_assign(std::function<void(const std::string&)> a);

    public:
        void start();

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
