#include "scan_common.h"

constexpr int TILE = 256;

__global__ void scan_v1_local(float* a, float* out, float* block_sums, int N) {
    __shared__ float tile[TILE];
    const int n = TILE;
    tile[threadIdx.x] = a[blockIdx.x * TILE + threadIdx.x];
    __syncthreads();
    for (int d = 1;d < n;d *= 2) {
        int i = threadIdx.x * 2 * d;
        if (i + 2 * d - 1 < n) {
            tile[i + 2 * d - 1] += tile[i + d - 1];
        }
        __syncthreads();
    }
    if (threadIdx.x == 0) block_sums[blockIdx.x] = tile[TILE - 1];
    __syncthreads();
    tile[TILE - 1] = 0;
    __syncthreads();
    for (int d = n / 2; d > 0;d /= 2) {
        int i = threadIdx.x * 2 * d;
        if (i + 2 * d - 1 < n) {
            float t = tile[i + d - 1];
            tile[i + d - 1] = tile[i + 2 * d - 1];
            tile[i + 2 * d - 1] = t + tile[i + 2 * d - 1];
        }
        __syncthreads();
    }
    out[blockIdx.x * TILE + threadIdx.x] = tile[threadIdx.x];
}

__global__ void scan_v1_offsets(float* block_sums, int nBlocks) {
    __shared__ float tile[1024];
    const int n = nBlocks;
    tile[threadIdx.x] = block_sums[threadIdx.x];
    __syncthreads();
    for (int d = 1;d < n;d *= 2) {
        int i = threadIdx.x * 2 * d;
        if (i + 2 * d - 1 < n) {
            tile[i + 2 * d - 1] += tile[i + d - 1];
        }
        __syncthreads();
    }
    if (threadIdx.x == 0) tile[n - 1] = 0;
    __syncthreads();
    for (int d = n / 2; d > 0;d /= 2) {
        int i = threadIdx.x * 2 * d;
        if (i + 2 * d - 1 < n) {
            float t = tile[i + d - 1];
            tile[i + d - 1] = tile[i + 2 * d - 1];
            tile[i + 2 * d - 1] = t + tile[i + 2 * d - 1];
        }
        __syncthreads();
    }
    block_sums[threadIdx.x] = tile[threadIdx.x];
}

__global__ void scan_v1_add(float* out, float* block_sums, int N) {
    out[blockIdx.x * TILE + threadIdx.x] += block_sums[blockIdx.x];
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
            scan_v1_local  <<<nBlocks, TILE>>>(a_d, out_d, block_sums_d, N);
            scan_v1_offsets<<<1, nBlocks>>>      (block_sums_d, nBlocks);
            scan_v1_add    <<<nBlocks, TILE>>>(out_d, block_sums_d, N);
        } else {
            // ← 你来写：Record(start) → 三个 kernel → Record(stop)
            //            → GetLastError → Synchronize → ElapsedTime → 取 min
            CUDA_CHECK(cudaEventRecord(start));
            scan_v1_local  <<<nBlocks, TILE>>>(a_d, out_d, block_sums_d, N);
            scan_v1_offsets<<<1, nBlocks>>>      (block_sums_d, nBlocks);
            scan_v1_add    <<<nBlocks, TILE>>>(out_d, block_sums_d, N);
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
