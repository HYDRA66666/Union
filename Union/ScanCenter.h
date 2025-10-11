#pragma once
#include "framework.h"
#include "pch.h"

#include "secretary_exception.h"
#include "background.h"
#include "utility.h"
#include "registry.h"
#include "PrintCenter.h"

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
        static std::string getline(std::string id, std::string promt);
        static std::future<std::string> async_getline(std::string id, std::string promt);

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
            static_string promtFormat = "[{}]{} > ";
        }vslz;
    private:
        PrintCenter& pc = PrintCenter::get_instance();

        std::atomic<bool> working = true;
        std::shared_mutex syslock;
        std::condition_variable_any syscv;

        virtual void work(thread_info& info) override;

    private:    // 用于重定向输入
        std::function<std::string()> sysgetline = []() {std::string line; std::getline(std::cin, line); return line; };
    public:
        void set_getline(std::function<std::string()> g);

    public:
        void lock();   // 锁定，防止输入
        void unlock(); // 解锁，允许输入

        /***************************** 输入管理 *****************************/
    private:
        struct input_ctrlblk
        {
            std::string id;
            std::string promt;
            std::promise<std::string> prms;
            operator bool() const;
        };

        std::list<input_ctrlblk> inputQueue;
        std::shared_mutex inputQueueMutex;

        std::string currentID;
        std::string currentPromt;

        std::function<void(std::string)> sysassign = nullptr;  // 当没有后台线程等待时，输入会自动路由到此处

    public:
        void set_assign(std::function<void(std::string)> a);

    };
}
