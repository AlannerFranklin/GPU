#include "reduction_common.h"

__global__ void reduction_v3(float* a, float* out, int N) {
    __shared__ float tile[256];
    int base = blockIdx.x * (blockDim.x * 4);
    float sum = 0.0;
    for (int i = 0;i < 4;i++) {
        sum += a[base + i * blockDim.x + threadIdx.x];
    }
    tile[threadIdx.x] = sum;
    __syncthreads();
    
    for (int s = blockDim.x / 2;s > 32; s /= 2) {
        if (threadIdx.x < s) {
            tile[threadIdx.x] += tile[threadIdx.x + s];
        }
        __syncthreads();
    }
    if (threadIdx.x < 32) {
        float v = tile[threadIdx.x];
        for (int off = 16; off > 0; off /= 2)
            v += __shfl_down_sync(0xffffffff, v, off);
        if (threadIdx.x == 0) out[blockIdx.x] = v;
    }
}

int main(int argc, char** argv) {

    int N = argc > 1 ? atoi(argv[1]) : 1024;
    run_benchmark(reduction_v3, dim3(256), dim3(N / 1024), N);
    return 0;
}