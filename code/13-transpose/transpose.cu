#include <cstdio>
#include <cstdlib>
#include <cuda_runtime.h>

#define CUDA_CHECK(call)                                        \
    do {                                                        \
        cudaError_t err_ = (call);                              \
        if (err_ != cudaSuccess) {                              \
            printf("[CUDA error] %s:%d\n  %s (code=%d)\n",      \
                __FILE__, __LINE__,                             \
                cudaGetErrorString(err_), (int)err_);           \
            exit(1);                                            \
        }                                                       \
    } while(0)

__global__ void transpose(int* in, int* out, int N) {
    int col = blockIdx.x * blockDim.x + threadIdx.x;
    int row = blockIdx.y * blockDim.y + threadIdx.y;

    int in_idx = row * N + col;
    out[col * N + row] = in[in_idx];
}

int main() {
    const int N = 64;
    int total = N * N;
    int* a_h = new int[total];
    for (int i = 0;i < total; i++) a_h[i] = i;
    int* b_h = new int[total];

    int* a_d = nullptr;
    int* b_d = nullptr;

    CUDA_CHECK(cudaMalloc((void**)&a_d, total * sizeof(int)));
    CUDA_CHECK(cudaMalloc((void**)&b_d, total * sizeof(int)));

    CUDA_CHECK(cudaMemcpy(a_d, a_h, total * sizeof(int), cudaMemcpyHostToDevice));

    dim3 block(32, 32);
    dim3 grid(2, 2);

    transpose<<<grid, block>>>(a_d, b_d, N);

    CUDA_CHECK(cudaMemcpy(b_h, b_d, total * sizeof(int), cudaMemcpyDeviceToHost));

    printf("%d %d %d %d %d\n", b_h[0], b_h[1], b_h[63], b_h[64], b_h[4032]);

    delete[] a_h;
    delete[] b_h;

    cudaFree(a_d);
    cudaFree(b_d);
    
    return 0;
}