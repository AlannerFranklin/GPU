// bank_conflict.cu —— shared memory 的 bank conflict 微基准
//
// 两个 kernel 做同样多的 shared 读，唯一区别是「落在哪些 bank 上」：
//
//   conflict_free   读 tile[threadIdx.x]        → bank = tx，铺满 0..31，无冲突
//   conflict_32way  读 tile[threadIdx.x * 32]   → bank 恒 = 0，32 个不同地址 → 32 路冲突
//
// 预期：32way 比 free 慢一个数量级左右（慢多少取决于瓶颈是否完全在 shared 上）。
//
// 编译运行：
//   build.bat 16-histogram
//   .\build\bank_conflict.exe
//
// 注意 volatile：不加的话编译器会把循环里的重复 shared 读提到循环外
// （读到寄存器里反复用），那就测不出任何东西了。

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

constexpr int TILE_SIZE = 8192;     // 256 线程 × 32 = 8192，冲突版要用满

__global__ void conflict_free(float* out, int iters) {
    __shared__ float tile[TILE_SIZE];
    volatile float* t = tile;                 // 强制每次真读

    t[threadIdx.x] = (float)threadIdx.x;      // 只需要前 256 个
    __syncthreads();

    float sum = 0.0f;
    for (int k = 0; k < iters; k++) {
        sum += t[threadIdx.x];                // bank = tx % 32 → 0..31 无冲突
    }
    out[blockIdx.x] = sum;                    // 防止整个循环被优化掉
}

__global__ void conflict_32way(float* out, int iters) {
    __shared__ float tile[TILE_SIZE];
    volatile float* t = tile;

    t[threadIdx.x * 32] = (float)threadIdx.x; // 用得到的位置先初始化
    __syncthreads();

    float sum = 0.0f;
    for (int k = 0; k < iters; k++) {
        sum += t[threadIdx.x * 32];           // (tx*32) % 32 恒 = 0 → 32 路冲突
    }
    out[blockIdx.x] = sum;
}

// 返回 min_ms
static float bench(void (*kernel)(float*, int), dim3 grid, dim3 block, int iters, float* d_out) {
    cudaEvent_t start = 0, stop = 0;
    CUDA_CHECK(cudaEventCreate(&start));
    CUDA_CHECK(cudaEventCreate(&stop));

    float min_ms = 1e12f, ms = 0;

    for (int rep = 0; rep < 4; rep++) {
        if (rep == 0) {
            kernel<<<grid, block>>>(d_out, iters);
        } else {
            CUDA_CHECK(cudaEventRecord(start));
            kernel<<<grid, block>>>(d_out, iters);
            CUDA_CHECK(cudaEventRecord(stop));
            CUDA_CHECK(cudaGetLastError());
            CUDA_CHECK(cudaEventSynchronize(stop));
            CUDA_CHECK(cudaEventElapsedTime(&ms, start, stop));
            min_ms = std::min(ms, min_ms);
        }
    }

    CUDA_CHECK(cudaEventDestroy(start));
    CUDA_CHECK(cudaEventDestroy(stop));
    return min_ms;
}

int main() {
    const int grid  = 1024;
    const int block = 256;
    const int iters = 10000;

    float* d_out = nullptr;
    CUDA_CHECK(cudaMalloc((void**)&d_out, grid * sizeof(float)));

    float ms_free = bench(conflict_free,  dim3(grid), dim3(block), iters, d_out);
    float ms_32   = bench(conflict_32way, dim3(grid), dim3(block), iters, d_out);

    // 总共的 shared 读次数（线程数 × 每个线程的迭代数）
    double total_loads = (double)grid * block * iters;

    printf("conflict_free   min_ms = %8.4f    %.1f G loads/s\n",
           ms_free, total_loads / (ms_free * 1e-3) / 1e9);
    printf("conflict_32way  min_ms = %8.4f    %.1f G loads/s\n",
           ms_32,   total_loads / (ms_32   * 1e-3) / 1e9);
    printf("---- 慢 %.2f 倍 ----\n", ms_32 / ms_free);

    CUDA_CHECK(cudaFree(d_out));
    return 0;
}
