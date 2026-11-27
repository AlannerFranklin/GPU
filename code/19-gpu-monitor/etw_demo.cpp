#define NOMINMAX
#include <Windows.h>
#include <evntrace.h>
#include <evntcons.h>
#include <cstdio>
#include <cstdlib>
#include <vector>

void WINAPI on_event(PEVENT_RECORD ev) {
    USHORT id = ev->EventHeader.EventDescriptor.Id;   // 事件 ID
    if (id == 1 || id == 2) {                          // 1=进程启动 2=进程退出
        printf("%s pid=%lu\n",
               id == 1 ? "start" : "stop",
               ev->EventHeader.ProcessId);
    }
}

BOOL WINAPI on_ctrl(DWORD type) {
    if (type != CTRL_C_EVENT && type != CTRL_CLOSE_EVENT) return FALSE;

    // handler 在另一个线程上跑，不能碰主线程的局部变量 —— 自己准备缓冲
    const int BUFSIZE = sizeof(EVENT_TRACE_PROPERTIES) + 2 * MAX_PATH * sizeof(wchar_t);
    std::vector<BYTE> buf(BUFSIZE);
    auto* p = reinterpret_cast<EVENT_TRACE_PROPERTIES*>(buf.data());
    ZeroMemory(p, BUFSIZE);
    p->Wnode.BufferSize  = BUFSIZE;
    p->LoggerNameOffset  = sizeof(EVENT_TRACE_PROPERTIES);

    printf("\n[ctrl-c] stopping session...\n");
    ControlTraceW(0, L"GpuMonitorSession", p, EVENT_TRACE_CONTROL_STOP);
    return TRUE;
}

int main(int argc, char** argv) {
    // 存活秒数可传参（验证时可给 60，方便在另一个窗口查 logman）
    int seconds = argc > 1 ? atoi(argv[1]) : 15;
    const wchar_t* SESSION_NAME = L"GpuMonitorSession";

    const int BUFSIZE = sizeof(EVENT_TRACE_PROPERTIES) + 2 * MAX_PATH * sizeof(wchar_t);
    std::vector<BYTE> propsBuf(BUFSIZE);
    auto* props = reinterpret_cast<EVENT_TRACE_PROPERTIES*>(propsBuf.data());

    auto fill_props = [&] () {
        ZeroMemory(props, BUFSIZE);
        props->Wnode.BufferSize = BUFSIZE;
        props->Wnode.Flags = WNODE_FLAG_TRACED_GUID;
        props->LogFileMode = EVENT_TRACE_REAL_TIME_MODE;
        props->LoggerNameOffset = sizeof(EVENT_TRACE_PROPERTIES);
    };

    fill_props();
    ControlTrace(0, SESSION_NAME, props, EVENT_TRACE_CONTROL_STOP);

    fill_props();
    TRACEHANDLE session = 0;
    ULONG err = StartTraceW(&session, SESSION_NAME, props);
    if (err != ERROR_SUCCESS) {
        printf("StartTrace failed: %lu\n", err);
        return 1;
    }

    printf("session start\n");

    static const GUID KERNEL_PROCESS =
        { 0x22FB2CD6, 0x0E7B, 0x422B, { 0xA0, 0xC7, 0x2F, 0xAD, 0x1F, 0xD0, 0xE7, 0x16 } };

    ULONG err2 = EnableTraceEx2(
        session,
        &KERNEL_PROCESS,
        EVENT_CONTROL_CODE_ENABLE_PROVIDER,
        TRACE_LEVEL_INFORMATION,
        0,
        0,
        0,
        nullptr
    );

    if (err2 != ERROR_SUCCESS) {
        printf("EnableTraceEx2 failed: %lu\n", err2);
        fill_props();
        ControlTraceW(session, SESSION_NAME, props, EVENT_TRACE_CONTROL_STOP);   // ← 先停
        return 1;
    }

    printf("provider enabled, %ds\n", seconds);

    // --- 自检：用 QUERY 确认会话此刻真的存在（不依赖第二个窗口的权限）---
    // QUERY 不会修改会话，只是把当前状态填进 props
    fill_props();
    ULONG q = ControlTraceW(session, SESSION_NAME, props, EVENT_TRACE_CONTROL_QUERY);
    if (q == ERROR_SUCCESS) {
        printf("session query: RUNNING  buffers=%lu free=%lu eventsLost=%lu\n",
               props->NumberOfBuffers, props->FreeBuffers, props->EventsLost);
    } else {
        printf("session query failed: %lu\n", q);
    }

        // ---- 第 4 步：接上消费者 ----
    // 这个结构体告诉 OpenTrace「怎么消费」
    EVENT_TRACE_LOGFILEW logfile = {};
    logfile.LoggerName          = (LPWSTR)SESSION_NAME;   // 消费哪个会话
    logfile.ProcessTraceMode    = PROCESS_TRACE_MODE_REAL_TIME      // 实时
                                | PROCESS_TRACE_MODE_EVENT_RECORD;  // 新式回调
    logfile.EventRecordCallback = on_event;               // 事件来了调谁

    TRACEHANDLE trace = OpenTraceW(&logfile);
    if (trace == INVALID_PROCESSTRACE_HANDLE) {
        printf("OpenTrace failed: %lu\n", GetLastError());
        fill_props();
        ControlTraceW(session, SESSION_NAME, props, EVENT_TRACE_CONTROL_STOP);
        return 1;
    }
    printf("trace opened, waiting for events...\n");

    SetConsoleCtrlHandler(on_ctrl, TRUE);

    ProcessTrace(&trace, 1, nullptr, nullptr);

    printf("ProcessTrace returned\n");
    CloseTrace(trace);


    fill_props();
    ControlTraceW(session, SESSION_NAME, props, EVENT_TRACE_CONTROL_STOP);
    printf("session stop\n");

    return 0;
}