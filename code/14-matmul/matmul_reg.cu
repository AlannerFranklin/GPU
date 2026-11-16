#include "matmul_common.h"

__global__ void matmul_reg(float* A, float* B, float* C, int N) {
    __shared__ float As[32][32];
    __shared__ float Bs[32][128];

    int block_row = blockIdx.y * 32;
    int block_col = blockIdx.x * 128;
    int thread_row = threadIdx.y * 4;

    float acc[4][4] = {0};
    float a[4];
    float b[4];

    for (int t = 0; t < N / 32; t++) {

        for (int i = 0; i < 4; i++) {
            As[thread_row + i][threadIdx.x] = A[(block_row + thread_row + i) * N + t * 32 + threadIdx.x];
        }

        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                Bs[thread_row + i][threadIdx.x + j * 32] = B[(t * 32 + thread_row + i) * N + block_col + threadIdx.x + j * 32];
            }
        }

        __syncthreads();

        for (int k = 0;k < 32;k++) {
            for (int i = 0;i < 4;i++) {
                a[i] = As[thread_row + i][k];
            }
            for (int j = 0; j < 4; j++) {
                b[j] = Bs[k][threadIdx.x + j * 32];
            }
            for (int i = 0; i < 4; i++) {
                for (int j = 0; j < 4; j++) {
                    acc[i][j] += a[i] * b[j];
                }
            }
        }
        __syncthreads();
    }
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            C[(block_row + thread_row + i) * N + block_col + threadIdx.x + j * 32] = acc[i][j];
        }
    }    
}

int main(int argc, char** argv) {
    int N = argc > 1 ? atoi(argv[1]) : 1024;
    run_benchmark(matmul_reg, dim3(32, 8), dim3(N / 128, N / 32), N);
    return 0;
}
