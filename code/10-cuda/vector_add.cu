#include <cstdio>
#include <cuda_runtime.h>
#include <cstdlib>

#define CUDA_CHECK(call)                                        \
    do {                                                        \
        cudaError_t err_ = (call);                              \
        if (err_ != cudaSuccess) {                              \
            printf("[CUDA error] %s:%d\n  %s (code=%d)\n",      \
                   __FILE__, __LINE__,                          \
                   cudaGetErrorString(err_), (int)err_);        \
            exit(1);                                            \
        }                                                       \
    } while (0)


__global__ void vector_add(int* a, int* b, int* c, int n) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < n) {
        c[i] = a[i] + b[i];
    }
}

int main() {
    const int N = 1000;
    int block = 256;

    int* a_h = new int[N];
    int* b_h = new int[N];
    int* c_h = new int[N];

    for (int i = 0; i < N;i++) {
        a_h[i] = i;
        b_h[i] = i * 2;
    }

    int *a_d = nullptr, *b_d = nullptr, *c_d = nullptr;

    CUDA_CHECK(cudaMalloc((void**)&a_d, N * sizeof(int)));
    CUDA_CHECK(cudaMalloc((void**)&b_d, N * sizeof(int)));
    CUDA_CHECK(cudaMalloc((void**)&c_d, N * sizeof(int)));
    
    
    CUDA_CHECK(cudaMemcpy(a_d, a_h, N * sizeof(int), cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(b_d, b_h, N * sizeof(int), cudaMemcpyHostToDevice));
    
    int grid = (N + block - 1) / block;

    vector_add<<<grid, block>>>(a_d, b_d, c_d, N);

    CUDA_CHECK(cudaGetLastError());          // 启动配置错误
    CUDA_CHECK(cudaDeviceSynchronize());     // 执行期错误   ← 注意：返回值被宏接住了

    CUDA_CHECK(cudaMemcpy(c_h, c_d, N * sizeof(int), cudaMemcpyDeviceToHost));

    
    for (int i = 0; i < 5; i++) printf("c[%d] = %d\n", i, c_h[i]);

    delete[] a_h; delete[] b_h; delete[] c_h;
    cudaFree(a_d);
    cudaFree(b_d);
    cudaFree(c_d);
    return 0;
}