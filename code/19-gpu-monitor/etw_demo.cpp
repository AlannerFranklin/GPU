#define NOMINMAX
#include <Windows.h>
#include <evntrace.h>
#include <evntcons.h>
#include <cstdio>
#include <cstdlib>
#include <vector>

void WINAPI on_event(PEVENT_RECORD ev) {

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

    Sleep((DWORD)seconds * 1000);


    fill_props();
    ControlTraceW(session, SESSION_NAME, props, EVENT_TRACE_CONTROL_STOP);
    printf("session stop\n");

    return 0;
}