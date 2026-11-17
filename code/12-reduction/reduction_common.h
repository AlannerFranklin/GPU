#pragma once
#include <cstdio>
#include <cstdlib>
#include <cuda_runtime.h>
#include <cmath>
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

inline void run_benchmark(void (*kernel)(float*, float*, int),
                          dim3 block, dim3 grid, int N) {

    float* a_h = new float[N];
    for (int i = 0;i < N;i++) {
        a_h[i] = float(i % 7) - 3.0f;
    }
    float* out_h = new float[grid.x];
    float* a_d = nullptr;
    float* out_d = nullptr;
    CUDA_CHECK(cudaMalloc((void**)&a_d, N * sizeof(float)));
    CUDA_CHECK(cudaMalloc((void**)&out_d, grid.x * sizeof(float)));

    CUDA_CHECK(cudaMemcpy(a_d, a_h, N * sizeof(float), cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemset(out_d, 0, grid.x * sizeof(float)));

    cudaEvent_t start = 0;
    cudaEvent_t stop = 0;
    CUDA_CHECK(cudaEventCreate(&start));
    CUDA_CHECK(cudaEventCreate(&stop));

    float min_time = 1e12f;
    float ms = 0;

    for (int rep = 0; rep < 4; rep++) {
        if (rep == 0) kernel<<<grid, block>>>(a_d, out_d, N);      // 预热，不计时
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

    CUDA_CHECK(cudaMemcpy(out_h, out_d, grid.x * sizeof(float), cudaMemcpyDeviceToHost));

    double ref = 0;
    for (int i = 0; i < N; i++) ref += (double)a_h[i];    // 主机独立算一遍

    float sum = 0;
    for (int i = 0; i < grid.x; i++) sum += out_h[i];      // GPU 结果汇总

    double rel_err = fabs((double)sum - ref) / fabs(ref);

    printf("sum=%.4f ref=%.4f rel_err=%.2e\n", sum, ref, rel_err);

    delete[] a_h;
    delete[] out_h;

    CUDA_CHECK(cudaFree(a_d));
    CUDA_CHECK(cudaFree(out_d));

    return;
}