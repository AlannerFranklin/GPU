#include "hist_common.h"

__global__ void histogram_naive(int* a, int* out, int N) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < N) atomicAdd(&out[a[i]], 1);
}


int main(int argc, char** argv) {
    int N = argc > 1 ? atoi(argv[1]) : 16777216;
    run_benchmark(histogram_naive, dim3(256), dim3(N / 256), N);
    return 0;
}