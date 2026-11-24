// ============================================================
// pdh_demo.cpp —— 用 PDH 读 GPU 利用率，并按进程名显示
//
// 对应 ROADMAP 第 17 周（ETW/PDH 数据采集）。
// 这是「GPU 监控工具」的雏形：任务管理器里 GPU 那一栏，底层就是这个。
//
// 输出示例：
//   13532  chrome.exe         3D     5.12%
//   18724  <unknown>          3D     0.85%
//
// 编译：build.bat 19-gpu-monitor
// ============================================================

// ---------- 头文件 ----------
#include <windows.h>     // Windows 基础：DWORD / BOOL / HANDLE / Sleep / wchar_t
#include <pdh.h>         // PDH 的函数：PdhOpenQuery / PdhCollectQueryData ...
#include <pdhmsg.h>      // PDH 的结构体和常量：PDH_FMT_COUNTERVALUE_ITEM_W / PDH_FMT_DOUBLE
#include <cstdio>        // printf
#include <vector>        // std::vector（用来装大小不定的缓冲）

// ============================================================
// Windows 的四个基本类型（看懂这张表，下面的代码就都能读了）
//
//   DWORD    = uint32_t      32 位无符号整数。名字来自 "Double WORD"（WORD = 16 位）
//   BOOL     = int           TRUE(1) / FALSE(0)
//   HANDLE   = void*         内核对象句柄。是一个「索引」，不是内存地址，不能解引用
//   LPCWSTR  = const wchar_t*   宽字符串指针（L=Long pointer, PC=Pointer to Const, W=Wide）
//
// 其他名字都是它们的别名：LPWSTR = wchar_t*，HKEY/HWND/HDC/PDH_HQUERY 都是 HANDLE 的别名。
//
// 关于 HANDLE：内核维护一张「句柄表」，每个进程一份。
//   OpenProcess(pid) 返回的是那张表里的一个索引，指向内核里的「进程对象」。
//   用完必须 CloseHandle —— 否则内核对象永远不释放（句柄泄漏，和内存泄漏一样严重）。
//
// 关于 A/W 后缀：所有涉及字符串的 Win32 API 都有两个版本
//   FooA  → 参数是 char*（ANSI）
//   FooW  → 参数是 wchar_t*（Wide）
//   头文件里定义宏：UNICODE 有定义时 Foo 展开成 FooW，否则展开成 FooA。
//   【本文件没有定义 UNICODE，所以不带后缀的名字会变成 A 版 —— 必须显式写 W】。
// ============================================================


// ============================================================
// process_name —— 把 pid 变成进程名（如 "chrome.exe"）
//
// 参数：
//   pid      要查的进程 ID
//   buf      调用方提供的内存，用来装结果
//   bufSize  buf 有多大（字符数）
// 返回：buf 本身（方便直接塞进 printf）；失败时 buf 里是 "<unknown>"
//
// 【为什么缓冲由调用方提供？】
//   这是 Win32 API 的固定模式：调用方准备内存，被调方往里写。
//   因为函数不知道你要多大的缓冲，而且不能返回函数内部的局部数组（函数返回就销毁了）。
// ============================================================
const wchar_t* process_name(DWORD pid, wchar_t* buf, DWORD bufSize) {

    // 先填一个默认值。这样后面任何一步失败，直接 return buf 就行，
    // 不用在每个失败分支都写一遍「填 unknown」。
    wcscpy_s(buf, bufSize, L"<unknown>");

    // ---------- ① 拿到进程的句柄 ----------
    // PROCESS_QUERY_LIMITED_INFORMATION = 权限级别：「我只查基本信息，不读写它的内存」
    //                                     这是最宽松的一档，普通用户对大部分进程都能成功
    // FALSE = 返回的句柄不继承给子进程
    HANDLE h = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);

    // 打不开的进程很常见：System 进程、受保护进程、权限比我们高的进程
    // 这不是错误，只是「查不了」，要优雅降级
    if (h == nullptr) return buf;

    // ---------- ② 查它的可执行文件路径 ----------
    // 「调用方准备缓冲」的典型写法：
    //   path = 我准备的缓冲，size = 我告诉 API 这块有多大
    //   API 往里写，并把「实际写了多少字符」回填到 size
    wchar_t path[MAX_PATH] = {0};
    DWORD   size = MAX_PATH;

    // 注意这里是 QueryFullProcessImageNameW —— 显式 W，理由见文件头的 A/W 说明
    // 结果类似：C:\Program Files\Google\Chrome\Application\chrome.exe
    BOOL ok = QueryFullProcessImageNameW(h, 0, path, &size);

    // ---------- ③ 关句柄 ----------
    // 【无论成败都要关】。这里放在 if (!ok) 之前，保证两条路都会执行到。
    // 如果提前 return，就会漏掉这一句 → 句柄泄漏。
    CloseHandle(h);

    if (!ok) return buf;

    // ---------- ④ 从完整路径里取文件名 ----------
    // 目标是 "chrome.exe"，不是整条路径（太长了，打印出来难看）
    // wcsrchr = wide 版的 strrchr，「从右往左找最后一个字符」
    const wchar_t* slash = wcsrchr(path, L'\\');     // 找最后一个反斜杠
    const wchar_t* name  = slash ? slash + 1 : path; // 反斜杠的下一个字符开始就是文件名
                                                     // 没有反斜杠说明本来就是纯文件名

    // _TRUNCATE = 如果名字比 bufSize 长，截断而不是报错
    wcsncpy_s(buf, bufSize, name, _TRUNCATE);

    return buf;
}


// ============================================================
// main
// ============================================================
int main() {
    // PDH 的两个句柄：
    //   query   = 一次「查询」，可以往里加多个计数器
    //   counter = 查询里的一个具体计数器
    PDH_HQUERY   query   = nullptr;
    PDH_HCOUNTER counter = nullptr;

    // ---------- ① 打开查询 ----------
    // 第一个参数是数据源（nullptr = 本机实时数据）
    if (PdhOpenQueryW(nullptr, 0, &query) != ERROR_SUCCESS) {
        printf("PdhOpenQuery failed\n");
        return 1;
    }

    // ---------- ② 加计数器 ----------
    // 路径格式：\对象(实例)\计数器
    //   \GPU Engine(*)                  ← 对象名 + 通配符实例（所有进程、所有引擎）
    //   \Utilization Percentage         ← 计数器名
    //
    // 【为什么用 PdhAddEnglishCounterW 而不是 PdhAddCounterW？】
    //   中文 Windows 上，计数器名可能被本地化成「GPU 引擎」之类。
    //   英文版 API 强制按英文名查，跨机器不会因为语言不同而失败。
    if (PdhAddEnglishCounterW(query,
            L"\\GPU Engine(*)\\Utilization Percentage", 0, &counter) != ERROR_SUCCESS) {
        printf("PdhAddCounter failed\n");
        PdhCloseQuery(query);
        return 1;
    }

    // ---------- ③ 采样两次 ----------
    // 「Utilization Percentage」是【速率型】计数器：
    //   值 = （本次计数 - 上次计数）/ 时间间隔
    // 所以第一次采样只是在建基线，没有「上一次」可以相减，值无效。
    //
    // 【规矩：至少采两次，用第二次以后的值。】
    PdhCollectQueryData(query);   // 第 1 次：建基线
    Sleep(1000);                  // 等 1 秒（采样的时间窗口）
    PdhCollectQueryData(query);   // 第 2 次：这次才有值

    // ---------- ④ 取数据（两段式调用） ----------
    // 因为用了通配符 *，实例数量是运行时才知道的（几十到几百个），
    // 所以不能用「取单个值」的 API，必须用 Array 版：
    //   第 1 次传 nullptr → 只为了问「需要多大缓冲」，返回 ERROR_INSUFFICIENT_BUFFER
    //   第 2 次传已分配的缓冲 → 真正取数据
    //
    // 【第一次返回错误是正常的，别当失败处理。】
    DWORD bufSize = 0, itemCount = 0;
    PdhGetFormattedCounterArrayW(counter, PDH_FMT_DOUBLE, &bufSize, &itemCount, nullptr);

    // 按 API 告诉我们的尺寸分配缓冲
    std::vector<BYTE> buf(bufSize);
    auto* items = reinterpret_cast<PDH_FMT_COUNTERVALUE_ITEM_W*>(buf.data());

    if (PdhGetFormattedCounterArrayW(counter, PDH_FMT_DOUBLE,
            &bufSize, &itemCount, items) != ERROR_SUCCESS) {
        printf("Get Data failed\n");
        PdhCloseQuery(query);
        return 1;
    }

    // ---------- ⑤ 遍历所有实例 ----------
    for (DWORD i = 0; i < itemCount; i++) {

        // 只关心真正在干活的。绝大多数进程的 GPU 利用率是 0。
        if (items[i].FmtValue.doubleValue > 1e-5) {

            // items[i].szName 是一条完整路径，长这样：
            //
            //   \GPU Engine(pid_12536_luid_0x00000000_0x0000AF9D_phys_0_eng_0_engtype_3D)\Utilization Percentage
            //               └─┬──┘ └──────────────┬──────────────┘ └─┬─┘ └─┬─┘ └────┬────┘
            //              哪个进程          哪块 GPU（LUID）      第几块  第几个   什么引擎
            //                                                        物理卡  引擎
            //
            // 所以「实例名本身就是迷你数据库」，用字符串查找把字段抠出来。

            // 抠 pid："pid_" 后面跟着数字，_wtoi 会自动读到非数字为止
            const wchar_t* p = wcsstr(items[i].szName, L"pid_");
            int pid = p ? _wtoi(p + 4) : -1;      // p+4 = 跳过 "pid_" 这 4 个字符

            // 抠引擎类型："engtype_" 后面一直读到 ')' 为止
            const wchar_t* e = wcsstr(items[i].szName, L"engtype_");

            // 任何一个找不到就跳过这一行。
            // 【Win32 的指针返回值经常是 nullptr，不检查就是随机崩溃。】
            if (!p || !e) continue;

            const wchar_t* end = wcschr(e + 8, L')');   // e+8 = 跳过 "engtype_"
            int len = (int)(end - (e + 8));              // 两个指针相减 = 中间有多少个字符

            wchar_t eng[32] = {0};
            wcsncpy_s(eng, 32, e + 8, len);

            // 进程名的缓冲。注意这是【另一块】内存，和上面 PDH 的 buf 完全无关——
            // 之前把 buf 传进来是个 bug，会覆盖 PDH 的数据。
            wchar_t name[64];

            // %-6d    = 整数，左对齐，占 6 格
            // %-24ls  = 宽字符串（l + s），左对齐，占 24 格
            // %-20ls  = 同上，20 格
            // %.2f%%  = 两位小数 + 一个百分号（%% 才是字面的 %）
            printf("%-6d %-24ls %-20ls %.2f%%\n",
                pid,
                process_name(pid, name, 64),
                eng,
                items[i].FmtValue.doubleValue);
        }
    }

    // ---------- ⑥ 清理 ----------
    // PdhCloseQuery 会同时释放 query 和它里面的所有 counter
    PdhCloseQuery(query);
    return 0;
}
