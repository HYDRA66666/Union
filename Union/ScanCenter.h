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
