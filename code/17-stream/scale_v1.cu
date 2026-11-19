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

constexpr int N = 1 << 24;
constexpr int CHUNKS = 4;
constexpr int CHUNK_N = N / CHUNKS;

__global__ void scale_kernel(float* a, float* c, int n) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < n) c[i] = a[i] * 2.0f + 1.0f;
}

int main() {

    cudaDeviceProp prop;
    CUDA_CHECK(cudaGetDeviceProperties(&prop, 0));
    printf("asyncEngineCount = %d\n", prop.asyncEngineCount);


    float* h_a = nullptr;
    float* h_c = nullptr;
    CUDA_CHECK(cudaMallocHost(&h_a, N * sizeof(float)));
    CUDA_CHECK(cudaMallocHost(&h_c, N * sizeof(float)));

    for (int i = 0;i < N;i++) h_a[i] = (float)i;
    float* d_a[CHUNKS];
    float* d_c[CHUNKS];
    for (int k = 0;k < CHUNKS;k++) {
        CUDA_CHECK(cudaMalloc(&d_a[k], CHUNK_N * sizeof(float)));
        CUDA_CHECK(cudaMalloc(&d_c[k], CHUNK_N * sizeof(float)));
    }

    cudaStream_t s[CHUNKS];
    for (int k = 0;k < CHUNKS;k++) { CUDA_CHECK(cudaStreamCreate(&s[k])); }
    cudaEvent_t start = 0, stop = 0;
    CUDA_CHECK(cudaEventCreate(&start));
    CUDA_CHECK(cudaEventCreate(&stop));

    float min_time = 1e12f, ms = 0;
    int grid = CHUNK_N / 256;
    for (int rep = 0;rep < 4;rep++) {
        if (rep == 0) {
            for (int k = 0; k < CHUNKS; k++) {
                cudaMemcpyAsync(d_a[k], h_a + k*CHUNK_N, CHUNK_N*sizeof(float), cudaMemcpyHostToDevice, s[k]);
                scale_kernel<<<grid, 256, 0, s[k]>>>(d_a[k], d_c[k], CHUNK_N);
                cudaMemcpyAsync(h_c + k*CHUNK_N, d_c[k], CHUNK_N*sizeof(float), cudaMemcpyDeviceToHost, s[k]);
            }
        } else {
            CUDA_CHECK(cudaEventRecord(start));
            for (int k = 0;k < CHUNKS;k++) {
                cudaMemcpyAsync(d_a[k], h_a + k*CHUNK_N, CHUNK_N*sizeof(float), cudaMemcpyHostToDevice, s[k]);
                scale_kernel<<<grid, 256, 0, s[k]>>>(d_a[k], d_c[k], CHUNK_N);
                cudaMemcpyAsync(h_c + k*CHUNK_N, d_c[k], CHUNK_N*sizeof(float), cudaMemcpyDeviceToHost, s[k]);
            }
            CUDA_CHECK(cudaEventRecord(stop));
            CUDA_CHECK(cudaGetLastError());
            CUDA_CHECK(cudaEventSynchronize(stop));
            CUDA_CHECK(cudaEventElapsedTime(&ms, start, stop));
            min_time = std::min(ms, min_time);
        }
    }
    printf("min_ms=%.4f\n", min_time);

    int bad = 0;
    for (int i = 0;i < N;i += N / 1024) {
        float expert = h_a[i] * 2.0f + 1.0f;
        if (fabsf(h_c[i] - expert) > 1e-3f) bad++;
    }

    printf("check: %s (bad=%d)\n", bad == 0 ? "OK" : "FAILED", bad);
    
    CUDA_CHECK(cudaFreeHost(h_a));
    CUDA_CHECK(cudaFreeHost(h_c));

    for (int k = 0; k < CHUNKS; k++) {
        CUDA_CHECK(cudaFree(d_a[k]));
        CUDA_CHECK(cudaFree(d_c[k]));
    }


    CUDA_CHECK(cudaEventDestroy(start));
    CUDA_CHECK(cudaEventDestroy(stop));


    for (int k = 0;k < CHUNKS;k++) { CUDA_CHECK(cudaStreamDestroy(s[k])); }
    return 0;
}