// Week 1 练习：验证本机 C++ 工具链
// 目标：确认 MSVC + CMake + Ninja 全部打通，并观察编译期/运行期的基本行为。

#include <cstdio>
#include <thread>
#include <vector>
#include <atomic>
#include <chrono>

int main() {
    std::printf("=== 工具链自检 ===\n");
    std::printf("C++ 标准: %ld\n", __cplusplus);
    std::printf("硬件并发度: %u\n", std::thread::hardware_concurrency());

    // 练习：用 4 个线程各自累加到 atomic，感受一下并发
    constexpr int kThreads   = 4;
    constexpr int kPerThread = 10'000'000;

    std::atomic<long long> total{0};
    std::vector<std::thread> pool;

    auto t0 = std::chrono::steady_clock::now();
    for (int t = 0; t < kThreads; ++t) {
        pool.emplace_back([&total, kPerThread] {
            long long local = 0;
            for (int i = 0; i < kPerThread; ++i) local += i;
            total.fetch_add(local, std::memory_order_relaxed);
        });
    }
    for (auto& th : pool) th.join();
    auto t1 = std::chrono::steady_clock::now();

    double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    std::printf("并行累加结果: %lld  (耗时 %.2f ms)\n", total.load(), ms);

    // 期望值：4 * sum(0..9999999)
    const long long expected = 4LL * (kPerThread - 1LL) * kPerThread / 2;
    std::printf("期望值:       %lld\n", expected);
    std::printf("%s\n", total.load() == expected ? "[OK] 工具链正常" : "[FAIL] 结果不符");

    return total.load() == expected ? 0 : 1;
}
