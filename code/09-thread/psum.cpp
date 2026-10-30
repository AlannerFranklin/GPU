#include <thread>
#include <vector>
#include <chrono>
#include <cstdio>


const int MAX_T = 8;
long long partial[MAX_T];
long long g_total;

void worker(int t, const int* a, size_t n, int T) {
    size_t chunk = n / T;
    size_t start = t * chunk;
    size_t end = (t == T - 1) ? n : (t + 1) * chunk;
    long long s = 0;
    for (size_t i = start;i < end;i++) {
        s += a[i];
    }
    partial[t] = s;
}

double run_parallel(const int* a, size_t n, int T) {
    auto t0 = std::chrono::steady_clock::now();
    std::vector<std::thread> threads;
    for (int t = 0;t < T;t++) {
        threads.push_back(std::thread(worker, t, a, n, T));
    }
    for (auto& th : threads) {
        th.join();
    }
    long long total = 0;
    for (long long i = 0;i < T;i++) {
        total += partial[i];
    }
    g_total = total;
    auto t1 = std::chrono::steady_clock::now();
    return std::chrono::duration<double, std::milli>(t1 - t0).count();
}

int main() {
    static int N = 100000000;
    int* a = new int[N];
    for (int i = 0;i < N;i++) {
        a[i] = i % 100;
    }
    for (int rep = 0; rep < 3; rep++) {
        std::printf("==== rep %d ====\n", rep);

        long long sum = 0;
        auto t0 = std::chrono::steady_clock::now();
        for (int i = 0;i < N;i++) {
            sum += a[i];
        }
        auto t1 = std::chrono::steady_clock::now();
        auto t2 = std::chrono::duration<double, std::milli>(t1 - t0).count();
        std::printf("sum=%lld, time=%.3f ms\n", sum, t2);

        auto t3 = std::chrono::steady_clock::now();
        worker(0, a, N, 1);
        auto t4 = std::chrono::steady_clock::now();
        auto t5 = std::chrono::duration<double, std::milli>(t4 - t3).count();
        std::printf("T=%d  time=%.3f ms  speedup=%.2fx  sum=%lld\n",
                    1, t5, t2  / t5, partial[0]);

        for (int T = 1; T <= 8; T *= 2) {
            double ms = run_parallel(a, N, T);
            std::printf("T=%d  time=%.3f ms  speedup=%.2fx  sum=%lld\n",
                        T, ms, t2  / ms, g_total);
        }
    }
    delete[] a;
    return 0;
}