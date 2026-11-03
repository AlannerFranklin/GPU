#include <thread>
#include <cstdio>
#include <functional>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <chrono>
#include <future>
#include <memory>
#include <algorithm>

const int ROUNDS = 3;          // 每个 benchmark 重复几轮，取最小值

class ThreadPool {
    public:
        ThreadPool(int n);
        ~ThreadPool();

        // 接受任意可调用对象，返回 std::future<它算出来的类型>
        template <typename F>
        auto enqueue(F&& f) {
            using R = decltype(f());
            auto task = std::make_shared<std::packaged_task<R()>>(std::forward<F>(f));
            std::future<R> fut = task->get_future();
            {
                std::lock_guard<std::mutex> lk(mtx);
                tasks.push([task] { (*task)(); });
            }
            cv.notify_one();
            return fut;
        }

    private:
        void worker();
        std::queue<std::function<void()>> tasks;
        std::mutex mtx;
        std::condition_variable cv;
        std::vector<std::thread> threads;
        bool flag = false;
};

ThreadPool::ThreadPool(int n) {
    threads.reserve(n);
    for (int i = 0; i < n; i++) {
        std::thread th(&ThreadPool::worker, this);
        threads.push_back(std::move(th));
    }
}

ThreadPool::~ThreadPool() {
    mtx.lock();
    flag = true;
    mtx.unlock();
    cv.notify_all();
    for (auto& th : threads) {
        th.join();
    }
}

void ThreadPool::worker() {
    while (true) {
        std::unique_lock<std::mutex> lk(mtx);
        cv.wait(lk, [this] { return !tasks.empty() || flag; });
        if (flag && tasks.empty()) break;               // 关了且没活了才走
        std::function<void()> task = std::move(tasks.front());
        tasks.pop();
        lk.unlock();                                    // 执行期间不占锁
        task();
    }
}

// ---------------------------------------------------------------- 被测的活

static long long do_work(int seed, int iters) {
    long long s = 0;
    for (int i = 0; i < iters; i++) {
        s += (long long)(seed + i) * 3;
    }
    return s;
}

// ---------------------------------------------------------------- Benchmark 1

// 线程池 vs 每任务一线程：衡量「省掉线程创建开销」的价值
static void bench1_pool_vs_thread() {
    const int NTASK = 1000;
    const int ITERS = 1000;

    printf("===== Benchmark 1: 线程池 vs 每任务一线程 =====\n");
    printf("NTASK=%d  ITERS=%d\n", NTASK, ITERS);

    ThreadPool pool(4);

    double bestPool   = 1e18;
    double bestThread = 1e18;
    long long checkPool = 0, checkThread = 0;

    for (int r = 0; r < ROUNDS; r++) {
        // ---- A：线程池 ----
        double msA;
        {
            auto t0 = std::chrono::steady_clock::now();

            std::vector<std::future<long long>> futs;
            futs.reserve(NTASK);
            for (int i = 0; i < NTASK; i++) {
                futs.push_back(pool.enqueue([i, ITERS] { return do_work(i, ITERS); }));
            }
            long long s = 0;
            for (auto& f : futs) s += f.get();
            checkPool = s;

            auto t1 = std::chrono::steady_clock::now();
            msA = std::chrono::duration<double, std::milli>(t1 - t0).count();
        }

        // ---- B：每任务一线程 ----
        double msB;
        {
            auto t0 = std::chrono::steady_clock::now();

            std::vector<std::thread> ths;
            std::vector<long long> results(NTASK);
            for (int i = 0; i < NTASK; i++) {
                ths.emplace_back([i, &results, ITERS] { results[i] = do_work(i, ITERS); });
            }
            for (auto& t : ths) t.join();

            long long s = 0;
            for (long long v : results) s += v;
            checkThread = s;

            auto t1 = std::chrono::steady_clock::now();
            msB = std::chrono::duration<double, std::milli>(t1 - t0).count();
        }

        printf("  round %d: pool=%8.3f ms  thread=%8.3f ms\n", r, msA, msB);
        bestPool   = std::min(bestPool, msA);
        bestThread = std::min(bestThread, msB);
    }

    printf("  min: pool=%.3f ms  thread=%.3f ms  speedup=%.2fx\n",
           bestPool, bestThread, bestThread / bestPool);
    printf("  check: pool=%lld thread=%lld %s\n\n",
           checkPool, checkThread, checkPool == checkThread ? "一致" : "不一致");
}

// ---------------------------------------------------------------- Benchmark 2

// 线程数扫描：看并行的真实扩展性上限
static void bench2_scaling() {
    const int NTASK = 64;
    const int ITERS = 1000000;

    printf("===== Benchmark 2: 线程数扫描 =====\n");
    printf("NTASK=%d  ITERS=%d\n", NTASK, ITERS);

    double base = 0;
    for (int T = 1; T <= 8; T *= 2) {
        double best = 1e18;
        long long check = 0;

        for (int round = 0; round < ROUNDS; round++) {
            ThreadPool pool(T);                     // 每轮一个全新的池

            auto t0 = std::chrono::steady_clock::now();

            std::vector<std::future<long long>> futs;
            futs.reserve(NTASK);
            for (int i = 0; i < NTASK; i++) {
                futs.push_back(pool.enqueue([i, ITERS] { return do_work(i, ITERS); }));
            }
            long long s = 0;
            for (auto& f : futs) s += f.get();
            check = s;

            auto t1 = std::chrono::steady_clock::now();
            double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
            best = std::min(best, ms);
        }

        if (T == 1) base = best;
        printf("  T=%d  %8.3f ms  speedup=%.2fx  check=%lld\n",
               T, best, base / best, check);
    }
    printf("\n");
}

// ---------------------------------------------------------------- Benchmark 3

// 任务粒度扫描：总计算量固定，只改任务被切碎的程度
static void bench3_granularity() {
    const int T = 4;
    const long long TOTAL = 100000000LL;

    printf("===== Benchmark 3: 任务粒度扫描 =====\n");
    printf("T=%d  总迭代=%lld\n", T, TOTAL);

    for (int NTASK = 1; NTASK <= 100000; NTASK *= 10) {
        int ITERS = (int)(TOTAL / NTASK);
        double best = 1e18;
        long long check = 0;

        for (int round = 0; round < ROUNDS; round++) {
            ThreadPool pool(T);

            auto t0 = std::chrono::steady_clock::now();

            std::vector<std::future<long long>> futs;
            futs.reserve(NTASK);
            for (int i = 0; i < NTASK; i++) {
                futs.push_back(pool.enqueue([i, ITERS] { return do_work(i, ITERS); }));
            }
            long long s = 0;
            for (auto& f : futs) s += f.get();
            check = s;

            auto t1 = std::chrono::steady_clock::now();
            double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
            best = std::min(best, ms);
        }

        printf("  NTASK=%6d  ITERS=%9d  %9.3f ms  %.3f ns/iter  check=%lld\n",
               NTASK, ITERS, best, best * 1e6 / (double)TOTAL, check);
    }
    printf("\n");
}

// ---------------------------------------------------------------- main

int main() {
    bench1_pool_vs_thread();
    bench2_scaling();
    bench3_granularity();
    return 0;
}
