#pragma once
#include <cstdio>
#include <cstdlib>
#include <cmath>
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

// 前缀和的 harness。和 reduction 的区别：
//   输出是 N 个 float（每元素一个），不是 grid.x 个。
//   对拍是「逐元素比 CPU 串行前缀和」，不是「全加起来比总和」。
//
// 注意：当前各版本每个 block 独立扫自己那 block.x 个元素，块之间不传递。
// 所以 CPU 参考也要按同样的分段来算，否则对不上。
inline void run_benchmark(void (*kernel)(float*, float*, int),
                          dim3 block, dim3 grid, int N) {

    float* a_h   = new float[N];
    float* out_h = new float[N];

    for (int i = 0; i < N; i++) {
        a_h[i] = float(i % 7) - 3.0f;
    }

    float* a_d   = nullptr;
    float* out_d = nullptr;
    CUDA_CHECK(cudaMalloc((void**)&a_d,   N * sizeof(float)));
    CUDA_CHECK(cudaMalloc((void**)&out_d, N * sizeof(float)));
    CUDA_CHECK(cudaMemcpy(a_d, a_h, N * sizeof(float), cudaMemcpyHostToDevice));

    cudaEvent_t start = 0;
    cudaEvent_t stop  = 0;
    CUDA_CHECK(cudaEventCreate(&start));
    CUDA_CHECK(cudaEventCreate(&stop));

    float min_time = 1e12f;
    float ms = 0;

    for (int rep = 0; rep < 4; rep++) {
        if (rep == 0) kernel<<<grid, block>>>(a_d, out_d, N);
        else {
            CUDA_CHECK(cudaEventRecord(start));
            kernel<<<grid, block>>>(a_d, out_d, N);

            CUDA_CHECK(cudaEventRecord(stop));
            CUDA_CHECK(cudaGetLastError());
            CUDA_CHECK(cudaEventSynchronize(stop));
            CUDA_CHECK(cudaEventElapsedTime(&ms, start, stop));
            min_time = std::min(ms, min_time);
        }
    }

    printf("min_ms=%.6f\n", min_time);

    CUDA_CHECK(cudaEventDestroy(start));
    CUDA_CHECK(cudaEventDestroy(stop));
    CUDA_CHECK(cudaMemcpy(out_h, out_d, N * sizeof(float), cudaMemcpyDeviceToHost));

    // 逐元素对拍：按 block.x 分段，每段独立算排除式前缀和
    const int chunk = (int)block.x;
    int bad = 0;
    float worst_err = 0.0f;
    int worst_at = -1;

    for (int base = 0; base + chunk <= N; base += chunk) {
        float run = 0.0f;
        for (int i = 0; i < chunk; i++) {
            float expect = run;          // 排除式：自己不算
            run += a_h[base + i];

            float err = fabsf(out_h[base + i] - expect);
            float tol = 1e-3f * (1.0f + fabsf(expect));
            if (err > tol) {
                bad++;
                if (err > worst_err) { worst_err = err; worst_at = base + i; }
            }
        }
    }

    if (bad == 0) {
        printf("scan check: OK (N=%d, chunk=%d, %d 个 chunk)\n", N, chunk, N / chunk);
    } else {
        printf("scan check: FAILED  %d 个元素不符，最大误差 %.4f @ index %d\n",
               bad, worst_err, worst_at);
    }

    delete[] a_h;
    delete[] out_h;
    CUDA_CHECK(cudaFree(a_d));
    CUDA_CHECK(cudaFree(out_d));
}
