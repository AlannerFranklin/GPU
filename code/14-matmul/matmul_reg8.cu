#include "matmul_common.h"

__global__ void matmul_reg8(float* A, float* B, float* C, int N) {
    __shared__ float As[64][32];
    __shared__ float Bs[32][256];

    int block_row = blockIdx.y * 64;
    int block_col = blockIdx.x * 256;

    float acc[8][8] = {0.0};
    float a[8];
    float b[8];

    for(int t = 0;t <  N / 32;t++) {
        for (int i = 0;i < 8;i++) {
            As[threadIdx.y * 8 + i][threadIdx.x] = A[(block_row + threadIdx.y * 8 + i) * N + t * 32 + threadIdx.x];
        }
        for (int i = 0;i < 4;i++) {
           for (int j = 0;j < 8;j++) {
                Bs[threadIdx.y * 4 + i][threadIdx.x + j * 32] = B[(t * 32 + threadIdx.y * 4 + i) * N + block_col + threadIdx.x + j * 32];
            }
        }
        __syncthreads();
        for (int k = 0; k < 32;k++) {
            for (int i = 0;i < 8;i++) {
                a[i] = As[threadIdx.y * 8 + i][k];
            }
            for (int j = 0;j < 8;j++) {
                b[j] = Bs[k][threadIdx.x + j * 32];
            }
            for (int i = 0;i < 8;i++) {
                for (int j = 0;j < 8;j++) {
                    acc[i][j] += a[i] * b[j];
                }
            }
        }
        __syncthreads();
    }

    for (int i = 0;i < 8;i++) {
        for (int j = 0;j < 8;j++) {
            C[(block_row + threadIdx.y * 8 + i) * N + block_col + threadIdx.x + j * 32] = acc[i][j];
        }
    }
}
int main(int argc, char** argv) {
    int N = argc > 1 ? atoi(argv[1]) : 1024;
    run_benchmark(matmul_reg8, dim3(32, 8), dim3(N / 256, N / 64), N);
    return 0;
}