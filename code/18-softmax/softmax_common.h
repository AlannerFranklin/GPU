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

// ------------------------------------------------------------
// softmax 的 harness。
//
// 和前面几个的根本差别：**输出是一片，不是标量也不是少数几格**。
// 所以对拍要做两件事：
//   ① 逐元素 —— 抽若干行，和 CPU 算的 softmax 比
//   ② 每行总和 = 1 —— 这是 softmax 的定义性质，对所有行都查（很便宜）
//
// 造数据时刻意在 [-30, 90] 之间取值：exp(90) 会溢出 float，
// 只有「先减最大值」的写法才能算对。**如果 kernel 忘了减 max，
// 这里会直接得到 NaN，一眼就能看出来。**
// ------------------------------------------------------------
inline void run_benchmark(void (*kernel)(float*, float*, int),
                          dim3 block, dim3 grid, int M, int N) {

    const int total = M * N;

    float* h_x   = new float[total];
    float* h_out = new float[total];

    // 值域 [-30, 90]：exp(90) = 1.2e39 > float 上限 3.4e38
    for (int i = 0; i < total; i++) {
        h_x[i] = (float)((i % 13) * 10 - 30);
    }

    float* d_x   = nullptr;
    float* d_out = nullptr;
    CUDA_CHECK(cudaMalloc((void**)&d_x,   (size_t)total * sizeof(float)));
    CUDA_CHECK(cudaMalloc((void**)&d_out, (size_t)total * sizeof(float)));
    CUDA_CHECK(cudaMemcpy(d_x, h_x, (size_t)total * sizeof(float), cudaMemcpyHostToDevice));

    cudaEvent_t start = 0;
    cudaEvent_t stop  = 0;
    CUDA_CHECK(cudaEventCreate(&start));
    CUDA_CHECK(cudaEventCreate(&stop));

    float min_time = 1e12f;
    float ms = 0;

    for (int rep = 0; rep < 4; rep++) {
        if (rep == 0) {
            kernel<<<grid, block>>>(d_x, d_out, N);
        } else {
            CUDA_CHECK(cudaEventRecord(start));
            kernel<<<grid, block>>>(d_x, d_out, N);
            CUDA_CHECK(cudaEventRecord(stop));
            CUDA_CHECK(cudaGetLastError());
            CUDA_CHECK(cudaEventSynchronize(stop));
            CUDA_CHECK(cudaEventElapsedTime(&ms, start, stop));
            min_time = std::min(ms, min_time);
        }
    }

    CUDA_CHECK(cudaEventDestroy(start));
    CUDA_CHECK(cudaEventDestroy(stop));
    CUDA_CHECK(cudaMemcpy(h_out, d_out, (size_t)total * sizeof(float), cudaMemcpyDeviceToHost));

    // ---------- 检查 ①：每行总和 = 1（全部行都查）----------
    int bad_sum = 0;
    float worst_sum_err = 0.0f;
    for (int r = 0; r < M; r++) {
        double s = 0.0;
        for (int c = 0; c < N; c++) s += h_out[r * N + c];
        float err = fabsf((float)s - 1.0f);
        if (err > 1e-3f) { bad_sum++; if (err > worst_sum_err) worst_sum_err = err; }
    }

    // ---------- 检查 ②：逐元素，抽 64 行比 ----------
    int bad_elem = 0;
    float worst_elem_err = 0.0f;
    int checked_rows = 0;

    for (int r = 0; r < M; r += (M > 64 ? M / 64 : 1)) {
        checked_rows++;

        // CPU 参考：同样的「先减最大值」
        float mx = -INFINITY;
        for (int c = 0; c < N; c++) mx = fmaxf(mx, h_x[r * N + c]);

        double sum = 0.0;
        for (int c = 0; c < N; c++) sum += expf(h_x[r * N + c] - mx);

        for (int c = 0; c < N; c++) {
            float expect = expf(h_x[r * N + c] - mx) / (float)sum;
            float err = fabsf(h_out[r * N + c] - expect);
            if (err > 1e-5f) {
                bad_elem++;
                if (err > worst_elem_err) worst_elem_err = err;
            }
        }
    }

    // ---------- 带宽 ----------
    // 最少要走 DRAM 的字节：读一遍 x + 写一遍 out
    double bytes = 2.0 * total * sizeof(float);
    double gbps  = bytes / (min_time * 1e-3) / 1e9;

    printf("min_ms=%.6f  GB/s=%.1f\n", min_time, gbps);
    printf("row_sum : %s  (%d/%d 行不符，最大误差 %.2e)\n",
           bad_sum == 0 ? "OK" : "FAILED", bad_sum, M, worst_sum_err);
    printf("element : %s  (%d 个元素不符，最大误差 %.2e，抽了 %d 行)\n",
           bad_elem == 0 ? "OK" : "FAILED", bad_elem, worst_elem_err, checked_rows);

    delete[] h_x;
    delete[] h_out;
    CUDA_CHECK(cudaFree(d_x));
    CUDA_CHECK(cudaFree(d_out));
}
