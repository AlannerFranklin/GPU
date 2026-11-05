#include <cstdio>
#include <cuda_runtime.h>
#include <cstdlib>

#define CUDA_CHECK(call)                                       \
    do {                                                       \
        cudaError_t err_ = (call);                             \
        if (err_ != cudaSuccess) {                             \
            printf("[CUDA error] %s:%d\n  %s (code=%d)\n",     \
                __FILE__, __LINE__,                            \
                cudaGetErrorString(err_), (int)err_);          \
            exit(1);                                           \
        }                                                      \
    } while (0)

__global__ void reverse_block(int* a) {
    int base = blockIdx.x * blockDim.x;
    __shared__ int tile[256];
    tile[threadIdx.x] = a[base + threadIdx.x];
    __syncthreads();
    a[base + threadIdx.x] = tile[blockDim.x - threadIdx.x - 1];
}

int main() {
    int N = 1024;
    int block = 256;
    int grid = N / block;

    int* a_h = new int[N];
    for (int i = 0;i < N;i++) {
        a_h[i] = i;
    }
    
    int* a_d = nullptr;
    CUDA_CHECK(cudaMalloc((void**)&a_d, N * sizeof(int)));
    CUDA_CHECK(cudaMemcpy(a_d, a_h, N * sizeof(int), cudaMemcpyHostToDevice));

    
    reverse_block<<<grid, block>>>(a_d);

    CUDA_CHECK(cudaMemcpy(a_h, a_d, N * sizeof(int), cudaMemcpyDeviceToHost));

    printf("%d %d %d %d\n", a_h[0], a_h[255], a_h[256], a_h[1023]);

    delete[] a_h;
    cudaFree(a_d);
    return 0;
}