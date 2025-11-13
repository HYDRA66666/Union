#include "pch.h"
#include "sfstream.h"

namespace HYDRA15::Union::archivist
{
    void sfstream::seg_io_threadpool::work(thread_info& info)
    {
        assistant::bfstream bfs(path);
        info.thread_state = thread_info::state::idle;
        while (working.load(std::memory_order_relaxed))
        {
            info.thread_state = thread_info::state::idle;
            seg_io_mission mis = std::move(queue.pop());
            info.thread_state = thread_info::state::working;
            switch (mis.oper)
            {
                using enum seg_io_mission::operation;
            case nothing:
                break;
            case read:
                try{ mis.readPrms.set_value(std::move(bfs.read(segSize * mis.segID, segSize))); }
                catch (...) { mis.readPrms.set_value(std::unexpected(std::current_exception())); }
                break;
            case write:
                try { bfs.write(segSize * mis.segID, std::move(mis.writePrms)); }
                catch(...){}
                break;
            }
        }
    }

    sfstream::seg_io_threadpool::seg_io_threadpool(const std::filesystem::path path, uint64_t segSize, unsigned int thrs)
        : path(path), background(thrs), segSize(segSize)
    {
        background::start();
    }

    sfstream::seg_io_threadpool::~seg_io_threadpool()
    {
        working.store(false, std::memory_order_relaxed);
        queue.notify_exit();
    }

    auto sfstream::seg_io_threadpool::submit_read(uint64_t id) -> std::future<read_ret>
    {
        seg_io_mission mis;
        mis.segID = id;
        mis.oper = seg_io_mission::operation::read;
        auto fut = mis.readPrms.get_future();
        queue.push(std::move(mis));
        return fut;
    }

    void sfstream::seg_io_threadpool::submit_write(uint64_t id, const std::vector<byte>& data)
    {
        seg_io_mission mis;
        mis.segID = id;
        mis.oper = seg_io_mission::operation::write;
        mis.writePrms = data;
        queue.push(std::move(mis));
    }

    std::vector<byte> sfstream::read(const std::list<uint64_t>& segIDs)
    {
        std::list<std::future<seg_io_threadpool::read_ret>> futLst;
        for (const auto& item : segIDs)
            futLst.push_back(std::move(
                segIOThreadPool.submit_read(item)
            ));

        std::vector<byte> res(segSize * segIDs.size(), 0);
        size_t i = 0;
        for (auto& item : futLst)
        {
            auto expc = item.get();
            if (!expc)
                std::rethrow_exception(expc.error());
            std::vector<byte> seg = std::move(expc.value());
            assistant::memcpy(res.data() + i * segSize, seg.data(), seg.size());
            i++;
        }

        return res;
    }

    void sfstream::write(const std::list<uint64_t>& segIDs, const std::vector<byte>& data)
    {
        size_t i = 0;
        for (const auto& item : segIDs)
        {
            std::vector<byte> seg(segSize, 0);
            assistant::memcpy(data.data() + segSize * i, seg.data(), std::min(data.size() - segSize * i, segSize));
            segIOThreadPool.submit_write(item, seg);
            i++;
        }
    }

    uint64_t sfstream::expand(uint64_t newSegCnt)
    {
        uint64_t segStart = usedSegs.fetch_add(newSegCnt, std::memory_order_relaxed);
        std::vector<byte> emptySeg(segSize, 0);

        for (uint64_t i = segStart; i < segStart + newSegCnt; i++)
            segIOThreadPool.submit_write(i, emptySeg);

        return segStart;
    }
}
