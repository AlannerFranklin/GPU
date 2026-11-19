#include "scan_common.h"
#include "scan_kernels.h"

__global__ void scan_v2_local(float* a, float* out, float* block_sums, int N) {
    __shared__ float warp_sums[8];
    int lane = threadIdx.x % 32;
    int warp = threadIdx.x / 32;

    float x = a[blockIdx.x * TILE + threadIdx.x];

    for (int off = 1; off < 32;off *= 2) {
        float y = __shfl_up_sync(0xffffffff, x, off);
        if (lane >= off) x += y;
    }
    if (lane == 31) {
        warp_sums[warp] = x;                          // ← 原版是存 warp_sums
    }
    __syncthreads();
    if (threadIdx.x == 0) { float run = 0; for (int k = 0;k < 8;k++) {float t = warp_sums[k]; warp_sums[k] = run; run += t;}}
    __syncthreads();
    x += warp_sums[warp];                             // ← 块内包含式
    float prev = __shfl_up_sync(0xffffffff, x, 1);
    if (lane == 0) prev = warp_sums[warp];

    if (threadIdx.x == TILE - 1) block_sums[blockIdx.x] = x;
    out[blockIdx.x * TILE + threadIdx.x] = prev;         // ← 原版输出（错的：包含式）

    //out[blockIdx.x * TILE + threadIdx.x] = x - a[blockIdx.x * TILE + threadIdx.x];
}

int main(int argc, char** argv) {
    int N = argc > 1 ? atoi(argv[1]) : 262144;
    int nBlocks = N / TILE;

    if (N % TILE != 0 || nBlocks > 1024) {
        printf("N 必须是 %d 的倍数，且 N/%d <= 1024\n", TILE, TILE);
        return 1;
    }

    // ---------- 主机 + 设备内存 ----------
    float* a_h   = new float[N];
    float* out_h = new float[N];
    for (int i = 0; i < N; i++) a_h[i] = float(i % 7) - 3.0f;

    float* a_d          = nullptr;
    float* out_d        = nullptr;
    float* block_sums_d = nullptr;

    CUDA_CHECK(cudaMalloc((void**)&a_d,          N * sizeof(float)));
    CUDA_CHECK(cudaMalloc((void**)&out_d,        N * sizeof(float)));
    CUDA_CHECK(cudaMalloc((void**)&block_sums_d, nBlocks * sizeof(float)));
    CUDA_CHECK(cudaMemcpy(a_d, a_h, N * sizeof(float), cudaMemcpyHostToDevice));

    // ---------- 计时 ----------
    cudaEvent_t start = 0;
    cudaEvent_t stop  = 0;
    CUDA_CHECK(cudaEventCreate(&start));
    CUDA_CHECK(cudaEventCreate(&stop));

    float min_time = 1e12f;
    float ms = 0;

    for (int rep = 0; rep < 4; rep++) {
        if (rep == 0) {
            // ← 你来写：预热，三个 kernel 各启动一次，不计时
            scan_v2_local  <<<nBlocks, TILE>>>(a_d, out_d, block_sums_d, N);
            scan_offsets   <<<1, nBlocks>>>      (block_sums_d, nBlocks);
            scan_add       <<<nBlocks, TILE>>>(out_d, block_sums_d, TILE);
        } else {
            // ← 你来写：Record(start) → 三个 kernel → Record(stop)
            //            → GetLastError → Synchronize → ElapsedTime → 取 min
            CUDA_CHECK(cudaEventRecord(start));
            scan_v2_local  <<<nBlocks, TILE>>>(a_d, out_d, block_sums_d, N);
            scan_offsets   <<<1, nBlocks>>>      (block_sums_d, nBlocks);
            scan_add       <<<nBlocks, TILE>>>(out_d, block_sums_d, TILE);
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

    // ---------- 对拍 ----------
    // ← 你来写：全局排除式前缀和，逐元素比
    float run = 0.0f;
    int bad = 0;
    for (int i = 0;i <  N;i++) {
        float expect = run;
        run += a_h[i];
        float err = fabsf(out_h[i] - expect);
        float tol = 1e-3f * (1.0f + fabsf(expect));
        if (err > tol) {
            bad++;
        }
    }
    if (bad == 0) printf("OK\n");
    else printf("FAILED\n");
    printf("bad_count=%d\n", bad);

    // ---------- 释放 ----------
    delete[] a_h;
    delete[] out_h;
    CUDA_CHECK(cudaFree(a_d));
    CUDA_CHECK(cudaFree(out_d));
    CUDA_CHECK(cudaFree(block_sums_d));

    return 0;
}
