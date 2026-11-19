#pragma once
#include <cstdio>
#include <cstdlib>
#include <cuda_runtime.h>
#include <algorithm>

#define CUDA_CHECK(call)                                        \
    do {                                                        \
        cudaError_t err_ = (call);                              \
        if (err_ != cudaSuccess) {                              \
            printf("[CUDA error] %s:%d\n  %s (code=%d)\n",      \
                __FILE__, __LINE__,                             \
                cudaGetErrorString(err_), (int)err_);           \
            exit(1);                                            \
        }                                                       \
    } while(0)

constexpr int NUM_BINS = 256;

// ------------------------------------------------------------
// histogram 的 harness。和 reduction / scan 的关键差别：
//
//   histogram 用 atomicAdd 累加到 out，而 harness 会 launch 4 次
//   （预热 1 + 计时 3）。**必须每次 launch 前清零**，否则结果 ×4。
//   这正是 2026-11-18 晚 reduction_v6 踩过的坑。
//
//   memset 放在 Record(start) 之前——流内顺序保证它不被计入计时。
//
// 指标：GB/s（每秒处理多少字节输入），不是 GFLOPS——histogram 不做浮点运算。
// ------------------------------------------------------------
inline void run_benchmark(void (*kernel)(int*, int*, int),
                          dim3 block, dim3 grid, int N) {

    int* a_h   = new int[N];
    int* out_h = new int[NUM_BINS];

    // 造数据：值域 [0, 255]，7 与 256 互质 → 各 bin 均匀分布
    for (int i = 0; i < N; i++) {
        a_h[i] = (i * 7 + 3) % NUM_BINS;
    }

    // 主机端参考结果
    int* ref = new int[NUM_BINS]();
    for (int i = 0; i < N; i++) ref[a_h[i]]++;

    int* a_d   = nullptr;
    int* out_d = nullptr;
    CUDA_CHECK(cudaMalloc((void**)&a_d,   N * sizeof(int)));
    CUDA_CHECK(cudaMalloc((void**)&out_d, NUM_BINS * sizeof(int)));
    CUDA_CHECK(cudaMemcpy(a_d, a_h, N * sizeof(int), cudaMemcpyHostToDevice));

    cudaEvent_t start = 0;
    cudaEvent_t stop  = 0;
    CUDA_CHECK(cudaEventCreate(&start));
    CUDA_CHECK(cudaEventCreate(&stop));

    float min_time = 1e12f;
    float ms = 0;

    for (int rep = 0; rep < 4; rep++) {
        // ★ 每次 launch 前清零，且在 start 事件之前 → 不进计时窗口
        CUDA_CHECK(cudaMemset(out_d, 0, NUM_BINS * sizeof(int)));

        if (rep == 0) {
            kernel<<<grid, block>>>(a_d, out_d, N);          // 预热
        } else {
            CUDA_CHECK(cudaEventRecord(start));
            kernel<<<grid, block>>>(a_d, out_d, N);
            CUDA_CHECK(cudaEventRecord(stop));
            CUDA_CHECK(cudaGetLastError());
            CUDA_CHECK(cudaEventSynchronize(stop));
            CUDA_CHECK(cudaEventElapsedTime(&ms, start, stop));
            min_time = std::min(ms, min_time);
        }
    }

    CUDA_CHECK(cudaEventDestroy(start));
    CUDA_CHECK(cudaEventDestroy(stop));
    CUDA_CHECK(cudaMemcpy(out_h, out_d, NUM_BINS * sizeof(int), cudaMemcpyDeviceToHost));

    // 校验
    int bad = 0;
    for (int i = 0; i < NUM_BINS; i++) {
        if (out_h[i] != ref[i]) {
            if (bad < 5) {
                printf("  bin[%d]  期望 %d  实际 %d\n", i, ref[i], out_h[i]);
            }
            bad++;
        }
    }

    double gbps = (double)N * sizeof(int) / (min_time * 1e-3) / 1e9;

    printf("min_ms=%.6f  GB/s=%.1f  %s\n",
           min_time, gbps, bad == 0 ? "OK" : "FAILED");
    if (bad) printf("  %d 个 bin 不符\n", bad);

    delete[] a_h;
    delete[] out_h;
    delete[] ref;
    CUDA_CHECK(cudaFree(a_d));
    CUDA_CHECK(cudaFree(out_d));
}
