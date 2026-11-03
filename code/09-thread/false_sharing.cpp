#include <atomic>
#include <thread>
#include <chrono>
#include <cstdio>

const int       T     = 4;
const long long ITERS = 2000000;     // 每线程写 1000 万次

// ================= 版本 1：紧挨着放 =================
// 4 个 atomic<long long>，每个 8 字节，共 32 字节
// 32 < 64，所以它们【全部落在同一条 cache line 里】
alignas(64) std::atomic<long long> g_near[4];

void work_near(int tid) {
    for (long long i = 0; i < ITERS; i++) {
        g_near[tid].fetch_add(i, std::memory_order_relaxed);
    }
}

// ================= 版本 2：对齐填充 =================
// TODO：定义 g_far[4]
//   要求：每个元素【独占一条 64 字节的 cache line】
//   提示： struct alignas(64) Padded { std::atomic<long long> v; };
//         然后 Padded g_far[4];

struct alignas(64) Padded
{
    /* data */
    std::atomic<long long> v;
};
Padded g_far[4];

void work_far(int tid) {
    for (long long i = 0; i < ITERS; i++) {
        // TODO：和 work_near 一样，只是改成写 g_far[tid].v
        g_far[tid].v.fetch_add(i, std::memory_order_relaxed);
    }
}

// ================= 下面是骨架，不用改 =================

double run(void (*fn)(int)) {
    auto t0 = std::chrono::steady_clock::now();
    std::thread th[T];
    for (int i = 0; i < T; i++) th[i] = std::thread(fn, i);
    for (int i = 0; i < T; i++) th[i].join();
    auto t1 = std::chrono::steady_clock::now();
    return std::chrono::duration<double, std::milli>(t1 - t0).count();
}

int main() {
    double msNear = run(work_near);
    double msFar  = run(work_far);

    printf("紧挨着 : %9.3f ms\n", msNear);
    printf("对齐填充: %9.3f ms\n", msFar);
    printf("伪共享代价: %.2fx\n", msNear / msFar);
    printf("check: %lld %lld\n",
           g_near[0].load(), g_far[0].v.load());
    return 0;
}
