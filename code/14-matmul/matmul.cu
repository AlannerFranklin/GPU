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

// ============================================================
// 14-matmul · naive 版 —— 待填写
// ============================================================
//
// 【目标】 C = A x B
//   A / B / C 都是 N x N 的 float 方阵，N = 1024
//
// 【kernel】 __global__ void matmul(...)
//   一个线程算 C 的一个元素 —— 这个线程的 row / col 从生到死不改变。
//
//     col = blockIdx.x * blockDim.x + threadIdx.x   // .x 管列
//     row = blockIdx.y * blockDim.y + threadIdx.y   // .y 管行
//
//     sum 从 0 开始，k 从 0 循环到 N-1：
//         sum += A[row * N + k] * B[k * N + col]
//
//     写回 C[row * N + col]
//
//   自己决定：参数怎么传、sum 用什么类型、循环在哪结束。

__global__ void matmul(float* A, float* B, float* C, int N) {
    int row = blockIdx.y * blockDim.y + threadIdx.y;
    int col = blockIdx.x * blockDim.x + threadIdx.x;

    float sum = 0;
    for (int k = 0;k < N;k++) {
        sum += A[row * N + k] * B[k * N + col];
    }
    C[row* N + col] = sum;
}

//
// 【main】
//   1. 主机数组 a_h / b_h / c_h，各 N*N 个 float（别忘了 delete[]）
//   2. 填初值：a_h[i] = i % 7 ， b_h[i] = i % 5
//   3. cudaMalloc 出 a_d / b_d / c_d
//   4. a_h / b_h 拷到设备（想想 c_h 要不要拷 —— 「谁先被读，谁才需要初始化」）
//   5. 配 dim3 block / grid，launch
//   6. c_d 拷回 c_h
//   7. 对拍（见下）
//   8. cudaEventDestroy / cudaFree / delete[]
//
// 【对拍】在主机端用三重循环独立再算一遍，至少比 3 个元素：
//
//     double ref = 0;
//     for (int k = 0; k < N; k++)
//         ref += (double)a_h[r * N + k] * b_h[k * N + c];
//
//   挑 C[0][0]、C[0][1]、C[1023][1023] 三个位置，和 c_h 里的值对比。
//   GPU 用 float 累加 1024 项会有误差，比较时看相对误差（<1e-3 算过）。
//
//   粗估量级：每一项最大 6 * 4 = 24，累加 1024 项，结果应在几千这个量级。
//   如果打印出 0、几十、或者 1e30 这种，说明下标或循环写错了。
//
// 【计时】照 vector_add.cu / transpose.cu 的 cudaEvent 写法：
//   预热 1 次不算，再跑 3 次取最小，打印 min_ms。
//   kernel launch 之外的东西（cudaMemcpy）绝对不能进计时窗口。
//
// ============================================================
int main(int argc, char** argv) {
    int N = argc > 1 ? atoi(argv[1]) : 1024;
    float* a_h = new float[N * N];
    float* b_h = new float[N * N];
    float* c_h = new float[N * N];

    for (int i = 0;i < N * N;i++) {
        a_h[i] = i % 7;
        b_h[i] = i % 5;
    }
    float* a_d = nullptr;
    float* b_d = nullptr;
    float* c_d = nullptr;

    CUDA_CHECK(cudaMalloc((void**)&a_d, N * N * sizeof(float)));
    CUDA_CHECK(cudaMalloc((void**)&b_d, N * N * sizeof(float)));
    CUDA_CHECK(cudaMalloc((void**)&c_d, N * N * sizeof(float)));


    CUDA_CHECK(cudaMemcpy(a_d, a_h, N * N * sizeof(float), cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(b_d, b_h, N * N * sizeof(float), cudaMemcpyHostToDevice));

    dim3 block(32, 32);
    dim3 grid(N / 32, N / 32);

    cudaEvent_t start = 0;
    cudaEvent_t stop = 0;
    CUDA_CHECK(cudaEventCreate(&start));
    CUDA_CHECK(cudaEventCreate(&stop));

    float min_time = 1e12f;
    float ms = 0;

    for (int rep = 0; rep < 4;rep++) {
        if (rep == 0) matmul<<<grid, block>>>(a_d, b_d, c_d, N);
        else {
            CUDA_CHECK(cudaEventRecord(start));
            matmul<<<grid, block>>>(a_d, b_d, c_d, N);

            CUDA_CHECK(cudaEventRecord(stop));
            CUDA_CHECK(cudaGetLastError());
            CUDA_CHECK(cudaEventSynchronize(stop));
            CUDA_CHECK(cudaEventElapsedTime(&ms, start, stop));
            min_time = std::min(ms, min_time);     
        }
    }

    CUDA_CHECK(cudaMemcpy(c_h, c_d, N * N * sizeof(float), cudaMemcpyDeviceToHost));
            
    printf("min_ms=%.6f\n", min_time);

    int rr[3] = {0, 0, 1023};
    int cc[3] = {0, 1, 1023};
    for (int t = 0;t < 3;t++) {
        double ref = 0;
        for(int k = 0;k < N;k++) {
            ref += double(a_h[rr[t] * N + k]) * double(b_h[k * N + cc[t]]);
        }
        printf("c_h=%.6f, ref=%.6f\n", c_h[rr[t] * N + cc[t]], ref);
    }

    cudaEventDestroy(start);
    cudaEventDestroy(stop);

    delete[] a_h;
    delete[] b_h;
    delete[] c_h;

    cudaFree(a_d);
    cudaFree(b_d);
    cudaFree(c_d);

    return 0;
}