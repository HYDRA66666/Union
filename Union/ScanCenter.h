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
        static std::string getline(std::string promt);
        static std::string getline();

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

    private:    // 用于重定向输入
        std::function<std::string()> sysgetline = []() {std::string line; std::getline(std::cin, line); return line; };
    public:
        void set_getline(std::function<std::string()> g);

    private:    // 用于设置默认提示词
        std::string defaultPromt;
    public:
        void set_defaultPromt(const std::string& promt);

    public:
        void start();

        /***************************** 输入管理 *****************************/
    private:
        std::string line;
        std::shared_mutex inputMutex;
        std::shared_mutex inputLineMutex;
        std::condition_variable_any inputcv;
        std::atomic<bool> isWaiting;

        std::function<void(const std::string&)> sysassign = nullptr;  // 当没有后台线程等待时，输入会自动路由到此处

    public:
        void set_assign(std::function<void(const std::string&)> a);

    };
}
