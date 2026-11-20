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

float bench_copy(void* dst, const void* src, size_t bytes, cudaMemcpyKind kind) {
    cudaEvent_t start = 0, stop = 0;
    CUDA_CHECK(cudaEventCreate(&start));
    CUDA_CHECK(cudaEventCreate(&stop));

    float min_ms = 1e12f, ms = 0;

    for (int rep = 0; rep < 4;rep++) {
        if (rep == 0) {
            CUDA_CHECK(cudaMemcpyAsync(dst, src, bytes, kind, 0));
        } else {
            CUDA_CHECK(cudaEventRecord(start));
            CUDA_CHECK(cudaMemcpyAsync(dst, src, bytes, kind, 0));
            CUDA_CHECK(cudaEventRecord(stop));
            CUDA_CHECK(cudaEventSynchronize(stop));    // ← 就是这一句
            CUDA_CHECK(cudaEventElapsedTime(&ms, start, stop));
            min_ms = std::min(ms, min_ms);
        }
    }
    CUDA_CHECK(cudaEventDestroy(start));
    CUDA_CHECK(cudaEventDestroy(stop));
    return min_ms;
}

int main() {
    cudaDeviceProp prop;
    CUDA_CHECK(cudaGetDeviceProperties(&prop, 0));
    printf("asyncEngineCount = %d\n", prop.asyncEngineCount);

    const size_t MAX_BYTES = 64ull * 1024 *1024;
    float* h = nullptr;
    float* d = nullptr;

    CUDA_CHECK(cudaMallocHost(&h, MAX_BYTES));
    CUDA_CHECK(cudaMalloc(&d, MAX_BYTES));

    for (size_t i = 0;i < MAX_BYTES / sizeof(float);i++) h[i] = 1.0f;
    const size_t sizes_mb[] = {1, 4, 16, 64};

    printf("%8s %12s %12s\n", "size", "H2D GB/s", "D2H GB/s");

    for (size_t mb : sizes_mb) {
        size_t bytes = mb * 1024 * 1024;
        float t_h2d = bench_copy(d, h, bytes, cudaMemcpyHostToDevice);
        float t_d2h = bench_copy(h, d, bytes, cudaMemcpyDeviceToHost);
        double gb_h2d = (double)bytes / 1e9 / (t_h2d * 1e-3);
        double gb_d2h = (double)bytes / 1e9 / (t_d2h * 1e-3);

        printf("%6zu MB %12.1f %12.1f\n", mb, gb_h2d, gb_d2h);

    }

    CUDA_CHECK(cudaFreeHost(h));
    CUDA_CHECK(cudaFree(d));

    return 0;
}