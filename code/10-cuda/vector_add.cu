#include <cstdio>

__global__ void vector_add(int* a, int* b, int* c, int n) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < n) {
        c[i] = a[i] + b[i];
    }
}

int main() {
    const int N = 1000;
    int* a_h = new int[N];
    int* b_h = new int[N];
    int* c_h = new int[N];

    for (int i = 0; i < N;i++) {
        a_h[i] = i;
        b_h[i] = i * 2;
    }

    int *a_d = nullptr, *b_d = nullptr, *c_d = nullptr;

    cudaError_t err1 = cudaMalloc((void**)&a_d, N * sizeof(int));
    if (err1 != cudaSuccess) {
        printf("cudaMalloc 失败: code=%d, %s\n", (int)err1, cudaGetErrorString(err1));
        return 1;
    }
    cudaError_t err2 =  cudaMalloc((void**)&b_d, N * sizeof(int));
    if (err2 != cudaSuccess) {
        printf("cudaMalloc 失败: code=%d, %s\n", (int)err2, cudaGetErrorString(err2));
        return 1;
    }
    cudaError_t err3 = cudaMalloc((void**)&c_d, N * sizeof(int));
    if (err3 != cudaSuccess) {
        printf("cudaMalloc 失败: code=%d, %s\n", (int)err3, cudaGetErrorString(err3));
        return 1;
    }
    cudaError_t err4 = cudaMemcpy(a_d, a_h, N * sizeof(int), cudaMemcpyHostToDevice);
    if (err4 != cudaSuccess) {
        printf("cudaMemcpy 失败: code=%d, %s\n", (int)err4, cudaGetErrorString(err4));
        return 1;
    }
    cudaError_t err5 = cudaMemcpy(b_d, b_h, N * sizeof(int), cudaMemcpyHostToDevice);
    if (err5 != cudaSuccess) {
        printf("cudaMemcpy 失败: code=%d, %s\n", (int)err5, cudaGetErrorString(err5));
        return 1;
    }

    int block = 256;
    int grid = (N + block - 1) / block;
    vector_add<<<grid, block>>>(a_d, b_d, c_d, N);

    cudaError_t err6 = cudaMemcpy(c_h, c_d, N * sizeof(int), cudaMemcpyDeviceToHost);
    if (err6 != cudaSuccess) {
        printf("cudaMemcpy 失败: %s\n", cudaGetErrorString(err6));
        return 1;
    }
    for (int i = 0; i < 5; i++) printf("c[%d] = %d\n", i, c_h[i]);

    delete[] a_h; delete[] b_h; delete[] c_h;
    cudaFree(a_d);
    cudaFree(b_d);
    cudaFree(c_d);
    return 0;
}