#include "scan_common.h"

constexpr int TILE = 256;

__global__ void scan_v0(float* a, float* out, int N) {
    const int n = TILE;
    __shared__ float tile[TILE];
    tile[threadIdx.x] = a[blockIdx.x * n + threadIdx.x];
    __syncthreads();

    for (int d = 1; d < n; d *= 2) {
        int i = threadIdx.x * 2 * d;
        if (i + 2 * d - 1 < n) {
            tile[i + 2 * d - 1] += tile[i + d - 1];
        }
        __syncthreads();
    }

    tile[n - 1] = 0;
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

    out[blockIdx.x * n + threadIdx.x] = tile[threadIdx.x];
}

int main(int argc, char** argv) {

    int N = argc > 1 ? atoi(argv[1]) : 1024;
    run_benchmark(scan_v0, dim3(TILE), dim3(N / TILE), N);
    return 0;
}