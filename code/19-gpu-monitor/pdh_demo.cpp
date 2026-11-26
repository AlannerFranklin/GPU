#define NOMINMAX
#include <Windows.h>
#include <cstdio>
#include <Pdh.h>
#include <PdhMsg.h>
#include <vector>
#include <conio.h>
#include <map>
#include <algorithm>

int main() {

    PDH_HQUERY   query   = nullptr;    // 「查询」句柄。是一个 HANDLE 的别名
    PDH_HCOUNTER counter = nullptr;    // 「计数器」句柄。也是 HANDLE 的别名
    PDH_HCOUNTER GPU = nullptr;

    DWORD bufSize   = 0;               // 缓冲要多大（字节），API 会回填
    DWORD itemCount = 0;               // 这一轮有多少个实例，API 会回填
    DWORD memBufSize = 0;
    DWORD memItemCount = 0;


    PdhOpenQueryW(nullptr, 0, &query);
    PdhAddEnglishCounterW(query, L"\\GPU Engine(*)\\Utilization Percentage", 0, &counter);
    PdhAddEnglishCounterW(query, L"\\GPU Process Memory(*)\\Local Usage", 0, &GPU);

    PdhCollectQueryData(query);
    while (true) {

        Sleep(1000);
        PdhCollectQueryData(query);
        PdhGetFormattedCounterArrayW(counter, PDH_FMT_DOUBLE, &bufSize, &itemCount, nullptr);
        PdhGetFormattedCounterArrayW(GPU, PDH_FMT_DOUBLE, &memBufSize, &memItemCount, nullptr);

        std::vector<BYTE> buf(bufSize);
        std::vector<BYTE> memBuf(memBufSize);
        auto* items = reinterpret_cast<PDH_FMT_COUNTERVALUE_ITEM_W*>(buf.data());
        auto* memItems = reinterpret_cast<PDH_FMT_COUNTERVALUE_ITEM_W*>(memBuf.data());
        PdhGetFormattedCounterArrayW(counter, PDH_FMT_DOUBLE, &bufSize, &itemCount, items);
        PdhGetFormattedCounterArrayW(GPU, PDH_FMT_DOUBLE, &memBufSize, &memItemCount, memItems);

        printf("this round: %lu items\n", itemCount);

        std::map<DWORD, double> by_pid;
        for (DWORD i = 0;i < itemCount;i++) {
            const wchar_t* p = wcsstr(items[i].szName, L"pid_");
            if (!p) continue;
            DWORD pid = (DWORD)_wtoi(p + 4);

            auto it = by_pid.find(pid);
            if (it == by_pid.end()) by_pid[pid] = items[i].FmtValue.doubleValue;
            else it->second = std::max(it->second, items[i].FmtValue.doubleValue);
        }

        std::map<DWORD, double> gpu;
        for (DWORD i = 0; i < memItemCount; i++) {
            const wchar_t* g = wcsstr(memItems[i].szName, L"pid_");
            if (!g) continue;
            DWORD pid = (DWORD)_wtoi(g + 4);

            gpu[pid] += memItems[i].FmtValue.doubleValue;    // ← 求和就这么写
        }

        for (auto& kv : by_pid) {
            DWORD pid = kv.first;
            double util = kv.second;
            double mem  = gpu.count(pid) ? gpu[pid] : 0.0;

            printf("pid=%lu  util=%.2f%%  mem=%.1f MB\n",
                   pid, util, mem / (1024.0 * 1024.0));
        }
        if (_kbhit()) { _getch(); break; }
    }
    PdhCloseQuery(query);

    return 0;
}