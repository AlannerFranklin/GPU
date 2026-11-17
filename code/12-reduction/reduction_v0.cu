#include "reduction_common.h"

__global__ void reduction_v0(float* a, float* out, int N) {
    __shared__ float tile[256];
    int base = blockIdx.x * blockDim.x;
    tile[threadIdx.x] = a[base + threadIdx.x];
    __syncthreads();

    for (int s = blockDim.x / 2;s > 0; s /= 2) {
        if (threadIdx.x < s) {
            tile[threadIdx.x] += tile[threadIdx.x + s];
        }
        __syncthreads();
    }
    if (threadIdx.x == 0) out[blockIdx.x] = tile[0];
}

int main(int argc, char** argv) {

    int N = argc > 1 ? atoi(argv[1]) : 1024;
    run_benchmark(reduction_v0, dim3(256), dim3(N / 256), N);
    return 0;
}