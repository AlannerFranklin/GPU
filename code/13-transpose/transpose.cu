#include <cstdio>
#include <cstdlib>
#include <cuda_runtime.h>
#include <algorithm>

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
    const int N = 1024;
    int total = N * N;
    int* a_h = new int[total];
    for (int i = 0;i < total; i++) a_h[i] = i;
    int* b_h = new int[total];

    int* a_d = nullptr;
    int* b_d = nullptr;


    CUDA_CHECK(cudaMalloc((void**)&a_d, total * sizeof(int)));
    CUDA_CHECK(cudaMalloc((void**)&b_d, total * sizeof(int)));


    cudaEvent_t start = 0;
    cudaEvent_t stop = 0;
    CUDA_CHECK(cudaEventCreate(&start));
    CUDA_CHECK(cudaEventCreate(&stop));

    float min_time = 1e12f;
    float ms = 0;

    CUDA_CHECK(cudaMemcpy(a_d, a_h, total * sizeof(int), cudaMemcpyHostToDevice));

    dim3 block(32, 32);
    dim3 grid(N / 32, N / 32);

    for (int rep = 0; rep < 4;rep++) {
        if (rep == 0) transpose<<<grid, block>>>(a_d, b_d, N);
        else {
            CUDA_CHECK(cudaEventRecord(start));
            transpose<<<grid, block>>>(a_d, b_d, N);

            CUDA_CHECK(cudaEventRecord(stop));
            CUDA_CHECK(cudaGetLastError());
            CUDA_CHECK(cudaEventSynchronize(stop));
            CUDA_CHECK(cudaEventElapsedTime(&ms, start, stop));
            min_time = std::min(ms, min_time);     
        }
    }
    CUDA_CHECK(cudaMemcpy(b_h, b_d, total * sizeof(int), cudaMemcpyDeviceToHost));
            
    printf("min_ms=%.6f\n", min_time);
    printf("%d %d %d %d\n", b_h[0], b_h[1], b_h[1023], b_h[1024]);
    
    delete[] a_h;
    delete[] b_h;

    CUDA_CHECK(cudaEventDestroy(start));
    CUDA_CHECK(cudaEventDestroy(stop));

    cudaFree(a_d);
    cudaFree(b_d);
    
    return 0;
}