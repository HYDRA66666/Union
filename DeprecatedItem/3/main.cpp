#define _CRT_SECURE_NO_WARNINGS
#include "pch.h"


void fwrite_test()
{
    FILE* fp = std::fopen("datafile.adb", "wb+");
    std::vector<char> dat(512 * 1024 * 1024);
    for (auto& i : dat)
        i = std::rand();

    auto tLastRefresh = std::chrono::steady_clock::now();
    size_t lastI = 0;
    size_t total = 64LL * 1024 * 1024 * 1024;
    for (size_t i = 0; i < total; i += dat.size())
    {
        std::fwrite(dat.data(), sizeof(char), dat.size(), fp);
        if (i % 1024 * dat.size() == 0)
        {
            auto now = std::chrono::steady_clock::now();
            auto per = std::chrono::duration_cast<std::chrono::duration<double>>(now - tLastRefresh).count();
            std::cout << std::format("{:.4f} MB/s, {:.2f}%", (i - lastI) / 1024 / 1024 / per, (double)i / total) << std::endl;
            lastI = i;
            tLastRefresh = now;
        }
    }
}

void ofstream_test()
{
    std::ofstream fp("datafile.adb", std::ios::binary);
    std::vector<char> dat(512 * 1024 * 1024);
    for (auto& i : dat)
        i = std::rand();

    auto tLastRefresh = std::chrono::steady_clock::now();
    size_t lastI = 0;
    size_t total = 64LL * 1024 * 1024 * 1024;
    for (size_t i = 0; i < total; i += dat.size())
    {
        fp.write(dat.data(), dat.size());
        if (i % 1024 * dat.size() == 0)
        {
            auto now = std::chrono::steady_clock::now();
            auto per = std::chrono::duration_cast<std::chrono::duration<double>>(now - tLastRefresh).count();
            std::cout << std::format("{:.4f} MB/s, {:.2f}%", (i - lastI) / 1024 / 1024 / per, (double)i / total) << std::endl;
            lastI = i;
            tLastRefresh = now;
        }
    }
}

int main()
{
    std::vector <std::shared_ptr< std::thread >> vec(8);
    for (auto& i : vec)
    {
        i = std::make_shared<std::thread>(fwrite_test);
        i->detach();
    }
    std::this_thread::sleep_for(std::chrono::seconds(100));
}
