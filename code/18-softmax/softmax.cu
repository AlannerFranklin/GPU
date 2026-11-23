#include "softmax_common.h"

__device__ float block_sum(float v, float* buf) {
    int lane = threadIdx.x % 32;
    int warp = threadIdx.x / 32;
    int nwarp = blockDim.x / 32;

 
    for (int off = 16;off > 0;off /= 2) {
        v += __shfl_down_sync(0xffffffff, v, off);
    }
    if (lane == 0) buf[warp] = v;

    __syncthreads();

    if (warp == 0) {
        v = (lane < nwarp) ? buf[lane] : 0.0f;
        for (int off = nwarp / 2;off > 0;off /= 2) {
            v += __shfl_down_sync(0xffffffff, v, off);
        }
        if (lane == 0) buf[0] = v;
    }

    __syncthreads();

    return buf[0];
}

__device__ float block_max(float v, float* buf) {
    int lane = threadIdx.x % 32;
    int warp = threadIdx.x / 32;
    int nwarp = blockDim.x / 32;

    
    for (int off = 16;off > 0;off /= 2) {
        v = fmaxf(v, __shfl_down_sync(0xffffffff, v, off));
    }
    if (lane == 0) buf[warp] = v;
    

    __syncthreads();

    if (warp == 0) {
        v = (lane < nwarp) ? buf[lane] : -INFINITY;
        for (int off = nwarp / 2;off > 0;off /= 2) {
            v = fmaxf(v, __shfl_down_sync(0xffffffff, v, off));
        }
        if (lane == 0) buf[0] = v;
    }

    __syncthreads();

    return buf[0];
}

__global__ void softmax(float* x, float* out,int N) {
    __shared__ float buf[32];

    float* row = x + blockIdx.x * N;

    float m = -INFINITY;
    for (int i = threadIdx.x;i < N;i += blockDim.x) 
        m = fmaxf(m, row[i]);
    m = block_max(m, buf);

    __syncthreads();

    float s = 0.0f;
    for (int i = threadIdx.x;i < N;i += blockDim.x) 
        s += expf(row[i] - m);
    s = block_sum(s, buf);
    
    __syncthreads();

    float inv = 1.0f / s;
    for (int i = threadIdx.x;i < N;i += blockDim.x) 
        out[blockIdx.x * N + i] = expf(row[i] - m) * inv;
}

int main(int argc, char** argv) {
    int M = 16384;                                  // 行数
    int N = argc > 1 ? atoi(argv[1]) : 1024;        // 每行多少元素
    run_benchmark(softmax, dim3(256), dim3(M), M, N);
    return 0;
}
