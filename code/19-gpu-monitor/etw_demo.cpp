#define NOMINMAX
#include <Windows.h>
#include <evntrace.h>
#include <evntcons.h>
#include <cstdio>
#include <vector>

void WINAPI on_event(PEVENT_RECORD ev) {

}

int main() {
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

    printf("provider enabled, 15s\n");

    Sleep(15000);


    fill_props();
    ControlTraceW(session, SESSION_NAME, props, EVENT_TRACE_CONTROL_STOP);
    printf("session stop\n");

    return 0;
}