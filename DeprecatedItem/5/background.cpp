#include "pch.h"
#include "background.h"

namespace HYDRA15::Union::labourer
{
    void background::work_shell(thread_info& info)
    {
        thread_info_guard tig(info);
        // 等待启动信号
        checkpoint.arrive_and_wait();
        // 执行工作
        info.workStartTime = std::chrono::steady_clock::now();
        work(info);
        // 等待所有线程完成工作
        auto t = checkpoint.arrive();
    }

    void background::start()
    {
        checkpoint.arrive_and_wait();
    }

    void background::wait_for_end()
    {
        checkpoint.arrive_and_wait();
    }

    background::background(unsigned int bkgThrCount)
        : checkpoint(bkgThrCount + 1)
    {
        threads.resize(bkgThrCount);
        for (auto& i : threads)
        {
            i.thread = std::make_shared<std::thread>(&background::work_shell, this, std::ref(i.info));
            i.thread_id = i.thread->get_id();
        }
    }

    background::background()
        :background(1)
    {

    }

    background::~background()
    {
        for (auto& i : threads)
            i.thread->detach();
    }


    background::iterator::iterator(list_iter iter)
        :it(iter)
    {

    }

    background::iterator& background::iterator::operator++()
    {
        it++;
        return *this;
    }

    bool background::iterator::operator!=(const iterator& other) const
    {
        return it != other.it;
    }

    background::thread_info& background::iterator::operator*() const
    {
        return it->info;
    }

    std::thread::id background::iterator::get_id() const
    {
        return it->thread_id;
    }

    background::iterator background::begin()
    {
        return iterator(threads.begin());
    }

    background::iterator background::end()
    {
        return iterator(threads.end());
    }

    background::thread_info_guard::thread_info_guard(background::thread_info& info)
        :thrInfo(info)
    {
        thrInfo.thread_state = background::thread_info::state::undefined;
    }

    background::thread_info_guard::~thread_info_guard()
    {
        thrInfo.thread_state = background::thread_info::state::finishing;
    }
}