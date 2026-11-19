#include "hist_common.h"

__global__ void histogram_shared(int* a, int* out, int N) {
    __shared__ int hist[NUM_BINS];

    for (int i = threadIdx.x; i < NUM_BINS; i += blockDim.x) hist[i] = 0;
    __syncthreads();
    for (int i = blockIdx.x * blockDim.x + threadIdx.x;i < N;i += blockDim.x * gridDim.x) {
        atomicAdd(&hist[a[i]], 1);
    }
    __syncthreads();
    for (int i = threadIdx.x;i < NUM_BINS;i += blockDim.x) {
        if (hist[i] > 0)
            atomicAdd(&out[i], hist[i]);
    }
}

int main(int argc, char** argv) {
    int N = argc > 1 ? atoi(argv[1]) : 16777216;
    run_benchmark(histogram_shared, dim3(256), dim3(1024), N);
    return 0;
}