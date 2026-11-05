#include <cstdio>
#include <cuda_runtime.h>
#include <cstdlib>
#include <algorithm>

#if 0
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
#endif

#define CUDA_CHECK(call)                                            \
    do {                                                            \
        cudaError_t err_ = (call);                                  \
        if (err_ != cudaSuccess) {                                  \
            printf("[CUDA error] %s:%d\n  %s (code=%d)\n",          \
                __FILE__, __LINE__,                                 \
                cudaGetErrorString(err_), (int)err_);               \
            exit(1);                                                \
        }                                                           \
    } while(0)

__global__ void vector_add(int* a, int *b, int* c, int n) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;

    if (i < n) {
        c[i] = a[i] + b[i];
    }
}

__global__ void vector_add_strided(int* a, int* b, int* c, int n) {
    int i = threadIdx.x * gridDim.x + blockIdx.x;

    if (i < n) {
        c[i] = a[i] + b[i];
    }
}

int main() {
    const int N = 1000000;
    int block = 256;

    cudaDeviceProp prop;
    CUDA_CHECK(cudaGetDeviceProperties(&prop, 0));
    printf("%s\n", prop.name);
    printf("L2 = %.1f MB\n", prop.l2CacheSize / 1048576.0);
    int* a_h = new int[N];
    int* b_h = new int[N];
    int* c_h = new int[N];
    for (int i = 0;i < N;i++) {
        a_h[i] = i;
        b_h[i] = i * 2;
    }
    int* a_d = nullptr;
    int* b_d = nullptr;
    int* c_d = nullptr;
    CUDA_CHECK(cudaMalloc((void**)&a_d, N * sizeof(int)));
    CUDA_CHECK(cudaMalloc((void**)&b_d, N * sizeof(int)));
    CUDA_CHECK(cudaMalloc((void**)&c_d, N * sizeof(int)));

    cudaEvent_t start = 0;
    cudaEvent_t stop = 0;
    CUDA_CHECK(cudaEventCreate(&start));
    CUDA_CHECK(cudaEventCreate(&stop));
    
    
    CUDA_CHECK(cudaMemcpy(a_d, a_h, N * sizeof(int), cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(b_d, b_h, N * sizeof(int), cudaMemcpyHostToDevice));

    float vector_add_min_time = 1e12f;
    float vector_add_strided_min_time = 1e12f;
    float ms = 0;
    const double total_bytes = 3.0 * N * sizeof(int);

    for (int rep = 0;rep < 4;rep++) {
        
        int grid = (block + N - 1) / block;
        if (rep == 0) {
            vector_add<<<grid, block>>>(a_d, b_d, c_d, N);
            CUDA_CHECK(cudaGetLastError());
        } else {
            CUDA_CHECK(cudaEventRecord(start));
    
            vector_add<<<grid, block>>>(a_d, b_d, c_d, N);

            CUDA_CHECK(cudaEventRecord(stop));

            CUDA_CHECK(cudaGetLastError());
            //CUDA_CHECK(cudaDeviceSynchronize());
            
            CUDA_CHECK(cudaEventSynchronize(stop));

            
            CUDA_CHECK(cudaEventElapsedTime(&ms, start, stop));
            vector_add_min_time = std::min(ms, vector_add_min_time);
            
            printf("vector_add : rep=%d, ms=%.6f\n", rep, ms);
            printf("vector_add : bandwidth = %.1f GB/s\n", total_bytes / ms / 1e6);
    
        }
        if (rep == 0) {
            vector_add_strided<<<grid, block>>>(a_d, b_d, c_d, N);
            CUDA_CHECK(cudaGetLastError());
        } else {
            CUDA_CHECK(cudaEventRecord(start));
    
            vector_add_strided<<<grid, block>>>(a_d, b_d, c_d, N);

            CUDA_CHECK(cudaEventRecord(stop));

            CUDA_CHECK(cudaGetLastError());
            //CUDA_CHECK(cudaDeviceSynchronize());
            
            CUDA_CHECK(cudaEventSynchronize(stop));

            
            CUDA_CHECK(cudaEventElapsedTime(&ms, start, stop));
            vector_add_strided_min_time = std::min(ms, vector_add_strided_min_time);
            printf("vector_add_strided : rep=%d, ms=%.6f\n", rep, ms);
            printf("vector_add_strided : bandwidth = %.1f GB/s\n", total_bytes / ms / 1e6);

        }
    }
    printf("vector_add_strided_min_time=%.6f, vector_add_min_time=%.6f\n", vector_add_strided_min_time, vector_add_min_time);
    float ratio = vector_add_strided_min_time / vector_add_min_time;
    printf("ratio=%.6f\n", ratio);

    CUDA_CHECK(cudaEventDestroy(start));
    CUDA_CHECK(cudaEventDestroy(stop));

    CUDA_CHECK(cudaMemcpy(c_h, c_d, N * sizeof(int), cudaMemcpyDeviceToHost));
    for (int i = 0; i < 5; i++) printf("c[%d] = %d\n", i, c_h[i]);

    printf("%d\n", c_h[N - 1] == 3 * (N - 1));

    delete[] a_h;
    delete[] b_h;
    delete[] c_h;
    cudaFree(a_d);
    cudaFree(b_d);
    cudaFree(c_d);
    return 0;
}