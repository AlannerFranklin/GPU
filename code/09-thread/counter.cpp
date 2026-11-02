#include <thread>
#include <cstdio>
#include <chrono>
#include <atomic>

class SpinLock {
    public:
        void lock();
        void unlock();
    private:
        std::atomic<bool> flag{false};
};

SpinLock g_lock;
void SpinLock::lock() {
    while (flag.exchange(true, std::memory_order_acquire)) {}
}

void SpinLock::unlock() {
    flag.store(false, std::memory_order_release);
}

long long counter = 0;

void sum() {
    for (int i = 0;i < 1000000; i++) {
        g_lock.lock();
        counter++;
        g_lock.unlock();
    }
    return;
}

int main() {
    for (int rep = 0; rep < 3; rep++) {
        std::printf("==== rep %d ====\n", rep);
        counter = 0;
        std::thread th1(sum);
        std::thread th2(sum);
        th1.join();
        th2.join();
        printf("counter=%lld\n", counter);
    }
}