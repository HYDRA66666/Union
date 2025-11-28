#include "pch.h"
#include "Union/simple_memory_table.h"

#include <atomic>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <memory>
#include <mutex>
#include <random>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

using namespace HYDRA15::Union::archivist;
using namespace HYDRA15::Union;

// 内存 loader（用于测试）
class inmem_loader : public loader {
private:
    field_specs ftab;
    ID pageSz;
    std::deque<field> storage; // 每行连续 ftab.size() 个 field
    std::atomic<ID> recCount{ 0 };

public:
    inmem_loader(const field_specs& fields, ID page_size = 16)
        : ftab(fields), pageSz(page_size) {
    }

    virtual std::pair<ID, ID> version() const override { return simple_memory_table::version; }

    virtual size_t size() const override { return static_cast<size_t>(recCount.load()); }
    virtual ID tab_size() const override { return recCount.load(); }
    virtual ID page_size() const override { return pageSz; }
    virtual field_specs fields() const override { return ftab; }
    virtual void clear() override { storage.clear(); recCount.store(0); }

    virtual page rows(ID pageNo) const override {
        page pg;
        pg.no = pageNo;
        pg.start = pageNo * pageSz;
        ID cur = recCount.load();
        if (pg.start >= cur) { pg.count = 0; return pg; }
        pg.count = std::min(pageSz, cur - pg.start);
        pg.data.clear();
        size_t startElem = static_cast<size_t>(pg.start) * ftab.size();
        size_t elemCount = static_cast<size_t>(pg.count) * ftab.size();
        if (startElem + elemCount <= storage.size()) {
            for (size_t i = 0; i < elemCount; ++i) pg.data.push_back(storage[startElem + i]);
        }
        else {
            // 防止竞态导致越界：安全返回可用范围
            size_t avail = storage.size() > startElem ? storage.size() - startElem : 0;
            for (size_t i = 0; i < std::min(avail, elemCount); ++i) pg.data.push_back(storage[startElem + i]);
            // 不足则补 NOTHING
            for (size_t i = pg.data.size(); i < elemCount; ++i) pg.data.push_back(NOTHING{});
        }
        return pg;
    }

    virtual void rows(const page& pg) override {
        ID neededRecords = std::max(recCount.load(), pg.start + pg.count);
        if (neededRecords > recCount.load()) {
            size_t newSize = static_cast<size_t>(assistant::power_of_2_not_less_than_n(static_cast<size_t>(neededRecords)));
            storage.resize(newSize * ftab.size());
            recCount.store(neededRecords);
        }
        size_t startElem = static_cast<size_t>(pg.start) * ftab.size();
        for (size_t i = 0; i < pg.data.size(); ++i)
            storage[startElem + i] = pg.data[i];
    }

    virtual index_specs indexies() const override { return {}; }
    virtual void index_tab(index) override {}
    virtual index index_tab(const std::string&) const override { return index{}; }
};

// 小工具：构造测试字段表
static field_specs make_test_fields() {
    return field_specs{
        simple_memory_table::sysfldRowMark,
        field_spec{ "f1", "int field", field_spec::field_type::INT },
        field_spec{ "f2", "float field", field_spec::field_type::FLOAT },
        field_spec{ "f3", "ints field", field_spec::field_type::INTS },
        field_spec{ "f4", "floats field", field_spec::field_type::FLOATS },
        field_spec{ "f5", "bytes field", field_spec::field_type::BYTES }
    };
}

int main(int argc, char** argv) {
    // 运行时长（秒），默认 15s
    int duration_seconds = 150;
    if (argc > 1) {
        try { duration_seconds = std::max(1, std::stoi(argv[1])); }
        catch (...) { duration_seconds = 15; }
    }

    // 全局计数器（所有线程在完成一次操作后 +1）
    std::atomic<uint64_t> global_ops{ 0 };

    // 创建表（使用内存 loader）
    auto ftab = make_test_fields();
    std::unique_ptr<loader> ld = std::make_unique<inmem_loader>(ftab, 16);
    simple_memory_table table(std::move(ld));

    // 先插入 100 条初始记录
    const int initial_count = 10000;
    for (int i = 0; i < initial_count; ++i) {
        auto e = table.create();
        std::unique_lock wlock{ *e }; // 按照库示例对 entry 加写锁
        e->set("f1", INT{ i });
        e->set("f2", FLOAT{ static_cast<FLOAT>(i) + 0.5 });
        e->set("f3", INTS{ i, i * 2 });
        e->set("f4", FLOATS{ static_cast<FLOAT>(i) + 0.1, static_cast<FLOAT>(i) + 0.2 });
        std::string s = "row" + std::to_string(i);
        BYTES b(s.begin(), s.end());
        e->set("f5", b);
        global_ops.fetch_add(1, std::memory_order_relaxed); // 初始插入也计入
    }

    // 控制变量
    std::atomic<bool> stopFlag{ false };

    // 读取线程函数（不停随机读单行）
    auto reader_fn = [&](int thread_index) {
        std::mt19937_64 rng(static_cast<uint64_t>(std::chrono::steady_clock::now().time_since_epoch().count()) + thread_index);
        while (!stopFlag.load(std::memory_order_relaxed)) {
            ID sz = table.size();
            if (sz == 0) { std::this_thread::sleep_for(std::chrono::milliseconds(1)); continue; }
            std::uniform_int_distribution<ID> dist(0, sz - 1);
            ID idx = dist(rng);
            auto ent = table.at(idx);
            std::shared_lock sl{ *ent };
                // 读取若干字段（仅作为负载）
            if (!std::holds_alternative<NOTHING>(ent->at("f1")))
            {
                volatile INT v1 = std::get<INT>(ent->at("f1"));
                (void)v1;
            }
                
            if (!std::holds_alternative<NOTHING>(ent->at("f2")))
            {
                volatile FLOAT v2 = std::get<FLOAT>(ent->at("f2"));
                (void)v2;
            }
            global_ops.fetch_add(1, std::memory_order_relaxed);
        }
        };

    // 写入线程函数（每 10ms 插入一条新数据）
    auto writer_fn = [&]() {
        int local_cnt = 0;
        while (!stopFlag.load(std::memory_order_relaxed)) {
            auto e = table.create();
            std::unique_lock wlock{ *e };
            int val = initial_count + (++local_cnt);
            e->set("f1", INT{ val });
            e->set("f2", FLOAT{ static_cast<FLOAT>(val) + 0.5 });
            e->set("f3", INTS{ val, val * 2 });
            e->set("f4", FLOATS{ static_cast<FLOAT>(val) + 0.1, static_cast<FLOAT>(val) + 0.2 });
            std::string s = "row" + std::to_string(val);
            BYTES b(s.begin(), s.end());
            e->set("f5", b);
            global_ops.fetch_add(1, std::memory_order_relaxed);
            //std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        };

    // 扫描线程函数（不停做一次全表扫描，每次扫描完成计数+1）
    auto scanner_fn = [&]() {
        while (!stopFlag.load(std::memory_order_relaxed)) {
                for (auto it = table.begin(); it != table.end(); ++it) {
                    entry& ent = *it;
                    std::shared_lock sl{ ent };
                        // 访问一个字段以产生负载
                    if (!std::holds_alternative<NOTHING>(ent.at("f1")))
                    {
                        volatile INT v = std::get<INT>(ent.at("f1")); 
                        (void)v;
                    }

                    if (stopFlag.load(std::memory_order_relaxed)) break;
                }
                global_ops.fetch_add(1, std::memory_order_relaxed); // 一次完整扫描完成

        }
        };

    // 启动线程：5 个读，1 个写，1 个扫
    const int reader_count = 5;
    const int writer_count = 5;
    const int scanner_count = 1;
    std::vector<std::thread> threads;
    for (int i = 0; i < reader_count; ++i) threads.emplace_back(reader_fn, i);
    for (int i = 0; i < writer_count; ++i) threads.emplace_back(writer_fn);
    for (int i = 0; i < scanner_count; ++i) threads.emplace_back(scanner_fn);

    // 主线程：每 1s 读取计数器计算 QPS 并打印
    uint64_t last_total = global_ops.load(std::memory_order_relaxed);
    auto start = std::chrono::steady_clock::now();
    for (int s = 0; s < duration_seconds; ++s) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        uint64_t now_total = global_ops.load(std::memory_order_relaxed);
        auto now = std::chrono::steady_clock::now();
        uint64_t delta = now_total - last_total;
        last_total = now_total;
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - start).count();
        std::cout << "[t=" << elapsed << "s] ops/sec = " << delta << "  (total ops=" << now_total << ")\n";
    }

    // 停止线程并 join
    stopFlag.store(true, std::memory_order_relaxed);
    for (auto& th : threads) if (th.joinable()) th.join();

    std::cout << "Test finished. total ops = " << global_ops.load() << std::endl;
    return 0;
}