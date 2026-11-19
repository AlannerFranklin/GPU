#pragma once
#include "scan_common.h"

// v1 / v2 / v3 共用的常量和辅助 kernel。
// 之前三份 .cu 各抄一遍，改一处要改三处，已合并到这里。

constexpr int TILE        = 256;    // 每个 block 的线程数（也是 v1/v2 每 block 的元素数）
constexpr int BLOCK_ELEMS = 1024;   // v3 每个 block 覆盖的元素数 = TILE * 4

// ------------------------------------------------------------
// 对 block_sums 做一次 exclusive scan（单 block，最多 1024 个）
// 调用时 block 大小传 nBlocks：<<<1, nBlocks>>>
// ------------------------------------------------------------
__global__ void scan_offsets(float* block_sums, int nBlocks) {
    __shared__ float tile[1024];
    const int n = nBlocks;

    tile[threadIdx.x] = block_sums[threadIdx.x];
    __syncthreads();

    for (int d = 1; d < n; d *= 2) {                 // 上扫
        int i = threadIdx.x * 2 * d;
        if (i + 2 * d - 1 < n) {
            tile[i + 2 * d - 1] += tile[i + d - 1];
        }
        __syncthreads();
    }

    if (threadIdx.x == 0) tile[n - 1] = 0;           // 根清零
    __syncthreads();

    for (int d = n / 2; d > 0; d /= 2) {             // 下扫
        int i = threadIdx.x * 2 * d;
        if (i + 2 * d - 1 < n) {
            float t = tile[i + d - 1];
            tile[i + d - 1]   = tile[i + 2 * d - 1];
            tile[i + 2 * d - 1] = t + tile[i + 2 * d - 1];
        }
        __syncthreads();
    }

    block_sums[threadIdx.x] = tile[threadIdx.x];
}

// ------------------------------------------------------------
// 把块偏移加回每个元素
//   elemsPerBlock：v1/v2 传 TILE(256)，v3 传 BLOCK_ELEMS(1024)
//
// 用「线程跨步」而不是「每线程连续几个」：同一个 warp 在每一轮读的是连续
// 32 个 float（128 字节 = 1 个 cache line），合并访存。
// ------------------------------------------------------------
__global__ void scan_add(float* out, float* block_sums, int elemsPerBlock) {
    int base    = blockIdx.x * elemsPerBlock;
    float delta = block_sums[blockIdx.x];

    for (int i = threadIdx.x; i < elemsPerBlock; i += blockDim.x) {
        out[base + i] += delta;
    }
}
