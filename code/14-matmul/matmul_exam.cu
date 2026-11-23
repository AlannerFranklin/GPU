#include "matmul_common.h"

__global__ void matmul_exam(float* A, float* B, float* C,int N) {
    __shared__ float As[32][32];
    __shared__ float Bs[32][32];

    int row = blockIdx.y * blockDim.y + threadIdx.y;
    int col = blockIdx.x * blockDim.x + threadIdx.x;

    float sum = 0.0f;

    for (int t = 0;t < N / 32;t++) {
        As[threadIdx.y][threadIdx.x] = A[(blockIdx.y * 32 + threadIdx.y) * N + t * 32 + threadIdx.x];
        Bs[threadIdx.y][threadIdx.x] = B[(t * 32 + threadIdx.y) * N + blockIdx.x * 32 + threadIdx.x];
        __syncthreads();
        for (int k = 0;k < 32;k++) {
            sum +=As[threadIdx.y][k]  * Bs[k][threadIdx.x];
        }
        __syncthreads();

    }
    C[row * N + col] = sum;
}

int main(int argc, char**argv) {
    int N = argc > 1 ? atoi(argv[1]) : 4096;
    run_benchmark(matmul_exam, dim3(32, 32), dim3(N / 32, N / 32), N);
    return 0;
}