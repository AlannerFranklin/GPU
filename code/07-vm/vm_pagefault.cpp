#include <cstdio>
#include <chrono>

int main() {
    const size_t N = 256ull * 1024 * 1024;
    char *p = new char[N];

    auto t0 = std::chrono::steady_clock::now();
    p[0] = 1;
    auto t1 = std::chrono::steady_clock::now();
    auto d0 = std::chrono::duration<double, std::milli>(t1 - t0).count();
    printf("d0=%.3f\n", d0);

    auto t2 = std::chrono::steady_clock::now();
    for (size_t i = 0;i < N;i += 4096) {
        p[i] = 1;
    }
    auto t3 = std::chrono::steady_clock::now();
    auto d1 = std::chrono::duration<double, std::milli>(t3 - t2).count();
    printf("d1=%0.3f\n", d1);

    auto t4 = std::chrono::steady_clock::now();
    for (size_t i = 0;i < N;i += 4096) {
        p[i] = 2;
    }
    auto t5 = std::chrono::steady_clock::now();
    auto d2 = std::chrono::duration<double, std::milli>(t5 - t4).count();
    printf("d2=%0.3f\n", d2);

    printf("first=%d last=%d\n", p[0], (int)p[N - 4096]);
    delete[] p;

    return 0;
}