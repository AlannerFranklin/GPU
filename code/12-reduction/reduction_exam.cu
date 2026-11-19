#include "reduction_common.h"

constexpr int K = 4;

__global__ void reduction_exam(float* a, float* out, int N) {
    __shared__ float tile[256];
    int base = blockIdx.x * (blockDim.x * K);
    float sum = 0;
    for (int i = 0;i < K;i++) {
        sum += a[base + i * blockDim.x + threadIdx.x];
    }
    tile[threadIdx.x] = sum;
    __syncthreads();
    for (int s = blockDim.x / 2;s > 32;s /= 2) {
        if (threadIdx.x < s) {  
            tile[threadIdx.x] += tile[threadIdx.x + s];
        }
        __syncthreads();
    }
    if (threadIdx.x < 32) {
        float v = tile[threadIdx.x];
        for (int off = 16; off > 0;off /= 2) {
            v += __shfl_down_sync(0xffffffff, v, off);
        }
        if (threadIdx.x == 0) out[blockIdx.x] = v;
    }
}

int main(int argc, char** argv) {
    int N = argc > 1 ? atoi(argv[1]) : 1024;
    run_benchmark(reduction_exam, dim3(256), dim3(N / (256 * K)), N);
    return 0;
}