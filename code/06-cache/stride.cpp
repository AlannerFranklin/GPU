#include <cstdio>
#include <chrono>
#include <vector>
#include <cstdlib>

int main() {
    const int N = 4096;
    std::vector<int> a(N * N);

    for (int i = 0; i < N * N; ++i)
        a[i] = rand() & 0xFF;

    volatile long long sum1 = 0;

    auto t0 = std::chrono::steady_clock::now();
    for (int i = 0; i < N; ++i)
        for (int j = 0; j < N; ++j)
            sum1 += a[i * N + j];
    auto t1 = std::chrono::steady_clock::now();

    volatile long long sum2 = 0;

    auto t2 = std::chrono::steady_clock::now();
    for (int j = 0; j < N; ++j)
        for (int i = 0; i < N; ++i)
            sum2 += a[i * N + j];
    auto t3 = std::chrono::steady_clock::now();

    auto ms1 = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0);
    auto ms2 = std::chrono::duration_cast<std::chrono::milliseconds>(t3 - t2);

    printf("sum1 = %lld\n", (long long)sum1);
    printf("sum2 = %lld\n", (long long)sum2);
    printf("row-major : %lld ms\n", (long long)ms1.count());
    printf("col-major : %lld ms\n", (long long)ms2.count());
    return 0;
}