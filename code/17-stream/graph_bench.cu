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

__global__ void tiny(float* p) { *p += 1.0f; }

constexpr int N = 10000;

int main() {
    float* d = nullptr;
    CUDA_CHECK(cudaMalloc(&d, sizeof(float)));

    cudaStream_t s;
    CUDA_CHECK(cudaStreamCreate(&s));

    cudaEvent_t start = 0, stop = 0;
    CUDA_CHECK(cudaEventCreate(&start));
    CUDA_CHECK(cudaEventCreate(&stop));

    float ms = 0;

    float min_base = 1e12f;
    for (int rep = 0;rep < 4;rep++) {
        if (rep == 0) {
            for (int i = 0;i < N;i++) {
                tiny<<<1, 1, 0, s>>>(d);
            }
        } else {
            CUDA_CHECK(cudaEventRecord(start));
            for (int i = 0;i < N;i++) {
                tiny<<<1, 1, 0, s>>>(d);
            }
            CUDA_CHECK(cudaEventRecord(stop));
            CUDA_CHECK(cudaEventSynchronize(stop));
            CUDA_CHECK(cudaEventElapsedTime(&ms, start, stop));
            min_base = std::min(ms, min_base);
            
        }
    }

    CUDA_CHECK(cudaStreamSynchronize(s));

    CUDA_CHECK(cudaStreamBeginCapture(s, cudaStreamCaptureModeGlobal));
    for (int i = 0;i < N;i++) {
        tiny<<<1, 1, 0, s>>>(d);
    }
    cudaGraph_t graph = nullptr;
    CUDA_CHECK(cudaStreamEndCapture(s, &graph));

    cudaGraphExec_t exec = nullptr;
    CUDA_CHECK(cudaGraphInstantiate(&exec, graph, 0));

    float min_graph = 1e12f;
    float ms_graph = 0;
    for (int rep = 0;rep < 4;rep++) {
        if (rep == 0) {
            CUDA_CHECK(cudaGraphLaunch(exec, s));
        } else {
            CUDA_CHECK(cudaEventRecord(start));
            CUDA_CHECK(cudaGraphLaunch(exec, s));
            CUDA_CHECK(cudaEventRecord(stop));
            CUDA_CHECK(cudaEventSynchronize(stop));
            CUDA_CHECK(cudaEventElapsedTime(&ms_graph, start, stop));
            min_graph = std::min(ms_graph, min_graph);
        }
    }
    printf("baseline  (10000 launches) = %8.4f ms\n", min_base);
    printf("graph     (1 launch)       = %8.4f ms\n", min_graph);
    printf("---- 加速 %.2f 倍 ----\n", min_base / min_graph);

    // 释放
    CUDA_CHECK(cudaGraphExecDestroy(exec));
    CUDA_CHECK(cudaGraphDestroy(graph));
    CUDA_CHECK(cudaEventDestroy(start));
    CUDA_CHECK(cudaEventDestroy(stop));
    CUDA_CHECK(cudaStreamDestroy(s));
    CUDA_CHECK(cudaFree(d));
    return 0;
}