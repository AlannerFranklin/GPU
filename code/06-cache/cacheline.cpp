#include <cstdio>
#include <chrono>
#include <vector>
#include <cstdlib>

int main() {
    const int CAP = 1048576;            // 1M 个 int = 4 MB
    const long long TOTAL = 16000000;   // 1600 万次（4MB 上跑 3200 万太慢）
    std::vector<int> a(CAP);
    for (int i = 0;i < CAP;i++) {
        a[i] = rand() & 0xFF;
    }

    for (int s = 1;s <= 64;s *= 2) {
        
        volatile long long sum = 0;
        int idx = 0;
        auto t0 = std::chrono::steady_clock::now();

        for (long long k = 0;k < TOTAL;k++) {
            sum += a[idx];
            idx += s;
            if (idx > CAP) idx -= CAP;
        }

        auto t1 = std::chrono::steady_clock::now();
        double ns  = std::chrono::duration<double, std::nano>(t1 - t0).count();
        double per = ns / TOTAL;
        printf("s = %2d   %6.3f ns/access   sum = %lld\n", s, per, (long long)sum);
    }
    return 0;
}