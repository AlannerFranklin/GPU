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

__global__ void reduction(int* a, int* out) {
    __shared__ int tile[256];
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

int main() {
    const int N = 1024;
    const int block = 256;
    const int grid = 4;

    int* a_h = new int[N];
    for (int i = 0;i < N;i++) {
        a_h[i] = i;
    }
    int* out_h = new int[grid];
    int* a_d = nullptr;
    int* out_d = nullptr;
    CUDA_CHECK(cudaMalloc((void**)&a_d, N * sizeof(int)));
    CUDA_CHECK(cudaMalloc((void**)&out_d, grid * sizeof(int)));


    CUDA_CHECK(cudaMemcpy(a_d, a_h, N * sizeof(int), cudaMemcpyHostToDevice));

    reduction<<<grid, block>>>(a_d, out_d);
    CUDA_CHECK(cudaMemcpy(out_h, out_d, grid * sizeof(int), cudaMemcpyDeviceToHost));

    int sum = 0;
    for (int i = 0;i < grid;i++) {
        sum += out_h[i];
    }
    printf("sum=%d\n", sum);

    delete[] a_h;
    delete[] out_h;

    cudaFree(a_d);
    cudaFree(out_d);

    return 0;
}