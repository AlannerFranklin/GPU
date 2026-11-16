#include "matmul_common.h"

__global__ void matmul(float* A, float* B, float* C, int N) {
    int row = blockIdx.y * blockDim.y + threadIdx.y;
    int col = blockIdx.x * blockDim.x + threadIdx.x;

    float sum = 0;
    for (int k = 0;k < N;k++) {
        sum += A[row * N + k] * B[k * N + col];
    }
    C[row* N + col] = sum;
}

int main(int argc, char** argv) {
    int N = argc > 1 ? atoi(argv[1]) : 1024;
    run_benchmark(matmul, dim3(32, 32), dim3(N / 32, N / 32), N);
    return 0;
}