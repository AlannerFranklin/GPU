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

inline void run_benchmark(void (*kernel)(float*, float*, float*, int),
                          dim3 block, dim3 grid, int N) {

    float* a_h = new float[N * N];
    float* b_h = new float[N * N];
    float* c_h = new float[N * N];

    for (int i = 0;i < N * N;i++) {
        a_h[i] = i % 7;
        b_h[i] = i % 5;
    }
    float* a_d = nullptr;
    float* b_d = nullptr;
    float* c_d = nullptr;

    CUDA_CHECK(cudaMalloc((void**)&a_d, N * N * sizeof(float)));
    CUDA_CHECK(cudaMalloc((void**)&b_d, N * N * sizeof(float)));
    CUDA_CHECK(cudaMalloc((void**)&c_d, N * N * sizeof(float)));


    CUDA_CHECK(cudaMemcpy(a_d, a_h, N * N * sizeof(float), cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(b_d, b_h, N * N * sizeof(float), cudaMemcpyHostToDevice));

    cudaEvent_t start = 0;
    cudaEvent_t stop = 0;
    CUDA_CHECK(cudaEventCreate(&start));
    CUDA_CHECK(cudaEventCreate(&stop));

    float min_time = 1e12f;
    float ms = 0;

    for (int rep = 0; rep < 4;rep++) {
        if (rep == 0) kernel<<<grid, block>>>(a_d, b_d, c_d, N);
        else {
            CUDA_CHECK(cudaEventRecord(start));
            kernel<<<grid, block>>>(a_d, b_d, c_d, N);

            CUDA_CHECK(cudaEventRecord(stop));
            CUDA_CHECK(cudaGetLastError());
            CUDA_CHECK(cudaEventSynchronize(stop));
            CUDA_CHECK(cudaEventElapsedTime(&ms, start, stop));
            min_time = std::min(ms, min_time);     
        }
    }

    CUDA_CHECK(cudaMemcpy(c_h, c_d, N * N * sizeof(float), cudaMemcpyDeviceToHost));
            
    printf("min_ms=%.6f\n", min_time);

    int rr[3] = {0, 0, N - 1};
    int cc[3] = {0, 1, N - 1};
    for (int t = 0;t < 3;t++) {
        double ref = 0;
        for(int k = 0;k < N;k++) {
            ref += double(a_h[rr[t] * N + k]) * double(b_h[k * N + cc[t]]);
        }
        printf("c_h=%.6f, ref=%.6f\n", c_h[rr[t] * N + cc[t]], ref);
    }

    CUDA_CHECK(cudaEventDestroy(start));
    CUDA_CHECK(cudaEventDestroy(stop));

    delete[] a_h;
    delete[] b_h;
    delete[] c_h;

    CUDA_CHECK(cudaFree(a_d));
    CUDA_CHECK(cudaFree(b_d));
    CUDA_CHECK(cudaFree(c_d));
    return;
}
