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

const int NTASK  = 1000;    // 先别用 10000，B 组会爆
const int ITERS  = 1000;
const int ROUNDS = 3;

class ThreadPool{
    public:
        ThreadPool(int n);
        ~ThreadPool();
        //void enqueue(std::function<void()> task);
        template <typename F>
        auto enqueue(F&& f) {
            using R = decltype(f());
            auto task = std::make_shared<std::packaged_task<R()>>(std::forward<F>(f));
            std::future<R> fut = task->get_future();
            {
                std::lock_guard<std::mutex> lk(mtx);
                tasks.push([task] {(*task)();});
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
    /*
    for (auto it = threads.begin(); it != threads.end(); it++) {
        it->join();
    }
    */
}

void ThreadPool::worker() {
    while(true) {
        std::unique_lock<std::mutex> lk(mtx);
        cv.wait(lk, [this] { return !tasks.empty() || flag; });
        if (flag && tasks.empty()) break;
        std::function<void()> task = std::move(tasks.front());
        tasks.pop();
        lk.unlock();
        task();
    }
}

/*void ThreadPool::enqueue(std::function<void()> task) {
    std::unique_lock<std::mutex> lk(mtx);
    tasks.push(std::move(task));
    cv.notify_one();
}*/

static long long do_work(int seed, int iters) {
    long long s = 0;
    for (int i = 0; i < iters; i++) {
        s += (long long)(seed + i) * 3;
    }
    return s;
}

int main() {
    ThreadPool pool(4);
    /*for (int i = 0;i < 8;i++) {
        pool.enqueue([i] {printf("task %d\n", i); });
    }*/
    //std::this_thread::sleep_for(std::chrono::milliseconds(100));   // 睡 100 毫秒
    /*auto f1 = pool.enqueue([] {return 42;});
    auto f2 = pool.enqueue([]{ return 3.14; });
    auto f3 = pool.enqueue([]{ printf("hello from worker\n"); });

    printf("%d\n", f1.get());
    printf("%f\n", f2.get());
    f3.get();
    std::vector<std::future<long long>> futs;
    futs.reserve(10000);
    for (int i = 0; i < 10000; i++) {
        futs.push_back(pool.enqueue([i] { return (long long)i; }));
    }
        long long sum = 0;
    for (auto& f : futs) {
        sum += f.get();
    }

    const long long EXPECT = 10000LL * 9999 / 2;
    printf("sum=%lld expect=%lld %s\n", sum, EXPECT, sum == EXPECT ? "OK" : "FAIL");
    */
    double bestPool   = 1e18;
    double bestThread = 1e18;
    long long checkPool = 0, checkThread = 0;

    for (int r = 0; r < ROUNDS; r++) {

        // ==================== A：线程池 ====================
        double msA;
        {
            auto t0 = std::chrono::steady_clock::now();

            std::vector<std::future<long long>> futs;
            futs.reserve(NTASK);
            for (int i = 0; i < NTASK; i++) {
                futs.push_back(pool.enqueue([i] { return do_work(i, ITERS); }));
            }
            long long s = 0;
            for (auto& f : futs) s += f.get();
            checkPool = s;

            auto t1 = std::chrono::steady_clock::now();
            msA = std::chrono::duration<double, std::milli>(t1 - t0).count();
        }

        // ==================== B：每任务一线程 ====================
        double msB;
        {
            auto t0 = std::chrono::steady_clock::now();

            std::vector<std::thread> ths;
            std::vector<long long> results(NTASK);
            for (int i = 0; i < NTASK; i++) {
                ths.emplace_back([i, &results] {
                    results[i] = do_work(i, ITERS);
                });
            }
            for (auto& t : ths) t.join();

            long long s = 0;
            for (long long v : results) s += v;
            checkThread = s;

            auto t1 = std::chrono::steady_clock::now();
            msB = std::chrono::duration<double, std::milli>(t1 - t0).count();
        }

        printf("round %d: pool=%8.3f ms  thread=%8.3f ms\n", r, msA, msB);
        bestPool   = std::min(bestPool, msA);
        bestThread = std::min(bestThread, msB);
    }

    printf("\nmin: pool=%.3f ms  thread=%.3f ms  speedup=%.2fx\n",
           bestPool, bestThread, bestThread / bestPool);
    printf("check: pool=%lld thread=%lld %s\n",
           checkPool, checkThread, checkPool == checkThread ? "一致" : "不一致");

    return 0;
    return 0;
}