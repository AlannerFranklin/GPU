#include "reduction_common.h"

__global__ void reduction_v4(float* a, float* out, int N) {

    __shared__ float warp_sums[8];

    int lane = threadIdx.x % 32;
    int warp = threadIdx.x / 32;

    int base = blockIdx.x * (blockDim.x * 4);
    float sum = 0.0;
    for (int i = 0;i < 4;i++) {
        sum += a[base + i * blockDim.x + threadIdx.x];
    }
    for(int off = 16;off > 0;off /= 2) {
        sum += __shfl_down_sync(0xffffffff, sum, off);
    }
    if (lane == 0) warp_sums[warp] = sum;
    __syncthreads();

    if (warp == 0) {
        sum = (lane < 8) ? warp_sums[lane] : 0.0f;
        for (int off = 4; off > 0; off /= 2)
            sum += __shfl_down_sync(0xffffffff, sum, off);
        if (lane == 0) out[blockIdx.x] = sum;
    }
}

int main(int argc, char** argv) {

    int N = argc > 1 ? atoi(argv[1]) : 1024;
    run_benchmark(reduction_v4, dim3(256), dim3(N / 1024), N);
    return 0;
}