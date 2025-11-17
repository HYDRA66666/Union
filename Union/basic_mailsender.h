#pragma once
#pragma once
#include "framework.h"
#include "pch.h"

#include "concepts.h"
#include "expressman_interfaces.h"
#include "background.h"
#include "factory.h"

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
        std::atomic_bool working;

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
        virtual ~basic_mailsender() { working.store(false, std::memory_order_release); cv.notify_all(); wait_for_end(); }

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

        virtual void work(thread_info& info) noexcept override
        {
            // 第一层 map 为 类名->数据包列表 的映射，第二层 map 为 序列号->数据包列表 的映射
            std::unordered_map<std::string, std::unordered_map<packet::uint, std::list<packet>>> cache;
            while (working.load(std::memory_order_acquire))
            {
                std::shared_lock slk(smt);
                cv.wait_for(slk, queryInterval);
                while ((pEmployer.expired() || !pFactory ) && working.load(std::memory_order_acquire))
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
                        catch (exceptions::expressman& e)
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