// ============================================================
// pdh_demo.cpp —— GPU 监控工具（利用率 + 显存，按进程）
//
// 对应 ROADMAP 第 17 周（ETW/PDH 数据采集）。
// 任务管理器里「GPU」那一栏，底层读的就是这两个计数器。
//
// 输出（每秒原地刷新一次，按任意键退出）：
//
//   PID    PROCESS                      GPU%          MEM
//   --------------------------------------------------------
//   4536   msedgewebview2.exe          4.58%      29.3 MB
//   8532   Code.exe                    2.90%      93.2 MB
//   13668  <unknown>                   1.49%     170.5 MB
//
// 编译：build.bat 19-gpu-monitor
// ============================================================

// ---------- 头文件 ----------
#define NOMINMAX               // ★ 必须在 windows.h 之前：阻止它定义 min/max 宏
                               //   否则 std::max 会被展开成 std::((a>b)?a:b) → 编译错误

#include <windows.h>     // Windows 基础：DWORD / BOOL / HANDLE / Sleep / wchar_t
#include <pdh.h>         // PDH 函数：PdhOpenQuery / PdhCollectQueryData ...
#include <pdhmsg.h>      // PDH 结构体：PDH_FMT_COUNTERVALUE_ITEM_W / PDH_FMT_DOUBLE
#include <cstdio>        // printf
#include <vector>        // std::vector（装「运行时才知道多大」的缓冲）
#include <algorithm>     // std::max
#include <map>           // std::map（按 pid 聚合）
#include <conio.h>       // _kbhit / _getch（检测按键）

// ============================================================
// Windows 的四个基本类型（看懂这张表，下面的代码就都能读了）
//
//   DWORD    = uint32_t         32 位无符号整数（名字来自 "Double WORD"，WORD=16 位）
//   BOOL     = int              TRUE(1) / FALSE(0)
//   HANDLE   = void*            内核对象句柄。是「索引」，不是地址，不能解引用
//   LPCWSTR  = const wchar_t*   宽字符串指针
//
// 其他名字都是它们的别名：
//   LPWSTR = wchar_t*   |   HKEY / HWND / HDC / PDH_HQUERY / PDH_HCOUNTER 都是 HANDLE 的别名
//
// 【关于 HANDLE】
//   内核维护一张「句柄表」，每个进程一份。OpenProcess(pid) 返回的是表里的索引，
//   指向内核里的「进程对象」。用完必须关（CloseHandle），否则内核对象永不释放。
//   但注意：不是所有句柄都要你关——GetStdHandle 返回的是系统给的对象，不归你管。
//
// 【关于 A/W 后缀】
//   所有涉及字符串的 Win32 API 都有两个版本：
//     FooA → 参数是 char*      FooW → 参数是 wchar_t*
//   头文件用 UNICODE 宏决定不带后缀的名字展开成哪个。
//   本项目的 CMakeLists 已定义 UNICODE，所以不写 W 也会走宽字符版；
//   但代码里仍然显式写 W，读起来更清楚。
// ============================================================


// ============================================================
// process_name —— 把 pid 变成进程名（如 "chrome.exe"）
//
// 参数：pid / 调用方提供的缓冲 / 缓冲有多长
// 返回：buf 本身（方便直接塞进 printf）；失败时 buf 里是 "<unknown>"
//
// 【为什么缓冲由调用方提供？】
//   Win32 的固定模式：调用方准备内存，被调方往里写。
//   不能返回函数内部的局部数组（函数一返回就销毁了）。
// ============================================================
const wchar_t* process_name(DWORD pid, wchar_t* buf, DWORD bufSize) {

    // 先填默认值。这样后面任何一步失败，直接 return buf 就行，
    // 不用在每个分支都重复写一遍「填 unknown」。
    wcscpy_s(buf, bufSize, L"<unknown>");

    // ---------- ① 拿到进程的句柄 ----------
    // PROCESS_QUERY_LIMITED_INFORMATION = 「我只查基本信息，不读写它的内存」
    //                                     最宽松的一档，普通用户对大部分进程都能成功
    // FALSE = 返回的句柄不继承给子进程
    HANDLE h = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);

    // 打不开很常见：System 进程、受保护进程、权限比我们高的进程。
    // 这不是错误，只是「查不了」——优雅降级，不影响其他行。
    if (h == nullptr) return buf;

    // ---------- ② 查它的可执行文件路径 ----------
    // 「调用方准备缓冲」的典型写法：
    //   path = 我准备的缓冲，size = 我告诉 API 这块有多大
    //   API 往里写，并把「实际写了多少字符」回填到 size
    wchar_t path[MAX_PATH] = {0};
    DWORD   size = MAX_PATH;

    // 结果类似：C:\Program Files\Google\Chrome\Application\chrome.exe
    BOOL ok = QueryFullProcessImageNameW(h, 0, path, &size);

    // ---------- ③ 关句柄 ----------
    // 【无论成败都要关】。放在 if (!ok) 之前，保证两条路都会执行到。
    // 如果先 return 了，就会漏掉这一句 → 句柄泄漏。
    CloseHandle(h);

    if (!ok) return buf;

    // ---------- ④ 从完整路径里取文件名 ----------
    // 只要 "chrome.exe"，不要整条路径（太长了，打印出来占位置）
    // wcsrchr = 从右往左找字符
    const wchar_t* slash = wcsrchr(path, L'\\');      // 最后一个反斜杠
    const wchar_t* name  = slash ? slash + 1 : path;  // 它后面就是文件名

    // _TRUNCATE = 太长了就截断，不要报错
    wcsncpy_s(buf, bufSize, name, _TRUNCATE);

    return buf;
}


// ============================================================
// main
// ============================================================
int main() {
    // PDH 的三个句柄（只有 query 需要显式关闭，两个 counter 随它一起释放）
    PDH_HQUERY   query      = nullptr;   // 一次「查询」，可容纳多个计数器
    PDH_HCOUNTER counter    = nullptr;   // 利用率计数器
    PDH_HCOUNTER memCounter = nullptr;   // 显存计数器

    // ---------- ① 打开查询 ----------
    if (PdhOpenQueryW(nullptr, 0, &query) != ERROR_SUCCESS) {
        printf("PdhOpenQuery failed\n");
        return 1;
    }

    // ---------- ② 加两个计数器 ----------
    // 路径格式：\对象(实例)\计数器
    //
    // 【为什么用 PdhAddEnglishCounterW 而不是 PdhAddCounterW？】
    //   中文 Windows 上计数器名可能被本地化。英文版 API 强制按英文名查，
    //   跨机器不会因为系统语言不同而失败。

    // 利用率：实例名形如
    //   pid_12536_luid_0x..._eng_0_engtype_3D      ← 有引擎
    if (PdhAddEnglishCounterW(query,
            L"\\GPU Engine(*)\\Utilization Percentage", 0, &counter) != ERROR_SUCCESS) {
        printf("PdhAddCounter failed\n");
        PdhCloseQuery(query);
        return 1;
    }

    // 显存：实例名形如
    //   pid_12904_luid_0x..._phys_0                ← 没有引擎
    // 单位是【字节】
    if (PdhAddEnglishCounterW(query,
            L"\\GPU Process Memory(*)\\Local Usage", 0, &memCounter) != ERROR_SUCCESS) {
        printf("Add mem counter failed\n");
        PdhCloseQuery(query);
        return 1;
    }

    // 控制台输出句柄（用于移动光标）。
    // 它和 PDH 无关，是系统给的，不需要 close。
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);

    // ---------- ③ 建基线，然后进主循环 ----------
    // 「Utilization Percentage」是【速率型】计数器：
    //   值 =（本次计数 − 上次计数）/ 时间间隔
    // 所以第一次采集只是建立基线，没有「上一次」可以相减，值无效。
    //
    // 【一次基线 + 之后每轮采一次】：上一轮就是这一轮的基线。
    PdhCollectQueryData(query);

    while (true) {
        Sleep(1000);                  // 等一个采样窗口
        PdhCollectQueryData(query);   // 采集本轮的值

        // ---------- ④ 取数据（两段式调用）----------
        // 因为用了通配符 *，实例数量是运行时才知道的（几十到几百个），
        // 所以必须用 Array 版：
        //   第 1 次传 nullptr → 只为了问「需要多大缓冲」→ 返回 ERROR_INSUFFICIENT_BUFFER
        //   第 2 次传已分配的缓冲 → 真正取到数据
        // 【第一次返回错误是正常的，别当失败处理。】

        // 利用率
        DWORD bufSize = 0, itemCount = 0;
        PdhGetFormattedCounterArrayW(counter, PDH_FMT_DOUBLE, &bufSize, &itemCount, nullptr);
        std::vector<BYTE> buf(bufSize);
        auto* items = reinterpret_cast<PDH_FMT_COUNTERVALUE_ITEM_W*>(buf.data());

        if (PdhGetFormattedCounterArrayW(counter, PDH_FMT_DOUBLE,
                &bufSize, &itemCount, items) != ERROR_SUCCESS) {
            printf("Get Data failed\n");
            PdhCloseQuery(query);
            return 1;
        }

        // 显存（同上四步）
        DWORD memBufSize = 0, memItemCount = 0;
        PdhGetFormattedCounterArrayW(memCounter, PDH_FMT_DOUBLE, &memBufSize, &memItemCount, nullptr);
        std::vector<BYTE> memBuf(memBufSize);
        auto* memItems = reinterpret_cast<PDH_FMT_COUNTERVALUE_ITEM_W*>(memBuf.data());
        PdhGetFormattedCounterArrayW(memCounter, PDH_FMT_DOUBLE,
                                     &memBufSize, &memItemCount, memItems);

        // ---------- ⑤ 按 pid 聚合 ----------
        //
        // 【两个计数器用【相反】的聚合规则 —— 这是本项目最重要的一个设计决策】
        //
        //   利用率 → 取 max
        //     3D 引擎和 Copy 引擎是【并行硬件】，可以同时干活。
        //     3D 用 40% + Copy 用 20%，不代表这个进程占了 60% —— 求和会超过 100%。
        //     取最大值 = 「这个进程最忙的那个引擎，忙了多久」。
        //
        //   显存 → 求和
        //     内存在多块 GPU 上是各自真实占用（pid 12904 在 0xAF9D 和 0xB2EC
        //     上各占一份），加起来才是这个进程用的总量。

        std::map<DWORD, double> by_pid;          // 利用率
        for (DWORD i = 0; i < itemCount; i++) {

            double util = items[i].FmtValue.doubleValue;
            if (util <= 0) continue;             // 绝大多数进程是 0，跳过

            // 从实例名里抠 pid：...pid_12536_luid_...
            const wchar_t* p = wcsstr(items[i].szName, L"pid_");
            if (!p) continue;                    // Win32 的指针可能是 nullptr，必须判
            DWORD pid = (DWORD)_wtoi(p + 4);     // p+4 跳过 "pid_" 这 4 个字符

            auto it = by_pid.find(pid);
            if (it == by_pid.end()) by_pid[pid] = util;              // 第一次见
            else it->second = std::max(it->second, util);            // 已有：取大的
        }

        std::map<DWORD, double> mem_by_pid;      // 显存
        for (DWORD i = 0; i < memItemCount; i++) {
            if (memItems[i].FmtValue.doubleValue <= 0) continue;

            const wchar_t* p = wcsstr(memItems[i].szName, L"pid_");
            if (!p) continue;
            DWORD pid = (DWORD)_wtoi(p + 4);

            mem_by_pid[pid] += memItems[i].FmtValue.doubleValue;     // ← 求和
        }

        // ---------- ⑥ 刷新显示 ----------
        // 不用 system("cls")（整屏重绘会闪）。正确做法是把光标移回左上角，
        // 让新内容覆盖旧内容。

        COORD home = {0, 0};
        SetConsoleCursorPosition(hOut, home);    // 光标回 (0,0)

        printf("%-6s %-24s %8s %12s\n", "PID", "PROCESS", "GPU%", "MEM");
        printf("--------------------------------------------------------\n");

        wchar_t name[64];
        for (auto& kv : by_pid) {
            DWORD  pid  = kv.first;              // map 按 key 排序，所以 pid 是升序
            double util = kv.second;
            // 这个进程可能在 by_pid 里、但不在 mem_by_pid 里（只算不占显存）
            double mem  = mem_by_pid.count(pid) ? mem_by_pid[pid] : 0.0;

            // %-6lu   整数左对齐占 6 格
            // %-24ls  宽字符串左对齐占 24 格
            // %7.2f%% 两位小数 + 字面百分号
            // %9.1f MB 一位小数
            printf("%-6lu %-24ls %7.2f%% %9.1f MB\n",
                   pid,
                   process_name(pid, name, 64),
                   util,
                   mem / (1024.0 * 1024.0));     // 字节 → MB
        }

        // ---------- ⑦ 清掉上一帧的残字 ----------
        // SetConsoleCursorPosition 只移动光标、不清屏。
        // 如果这一帧比上一帧短，下面的旧内容就会留着（表现为「重复的行」）。
        // 所以打印完之后，从当前光标位置开始用空格填满接下来若干行。
        CONSOLE_SCREEN_BUFFER_INFO info;
        if (GetConsoleScreenBufferInfo(hOut, &info)) {
            DWORD written = 0;
            FillConsoleOutputCharacterW(hOut, L' ',
                                        info.dwSize.X * 10,    // 填 10 行的宽度
                                        info.dwCursorPosition, // 从光标当前位置开始
                                        &written);
        }

        // ---------- ⑧ 退出检查 ----------
        // _kbhit() 返回「现在有没有按键」，不阻塞、不消耗按键
        // _getch() 把那个按键读走，防止它留在缓冲区里
        if (_kbhit()) { _getch(); break; }
    }

    // ---------- ⑨ 清理 ----------
    // PdhCloseQuery 会同时释放 query 和它里面的两个 counter
    PdhCloseQuery(query);
    return 0;
}
