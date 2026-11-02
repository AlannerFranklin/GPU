#include <atomic>
#include <thread>
#include <cstdio>

class SPSCQueue {
    public :
        bool push(int v);
        bool pop(int& v);
    private:
        static constexpr int N = 8;
        int buf[N];
        std::atomic<size_t> head{0};
        std::atomic<size_t> tail{0};
};

bool SPSCQueue::push(int v) {
    size_t h = head.load(std::memory_order_relaxed);
    size_t next = (h + 1) % N;
    if (next == tail.load(std::memory_order_acquire)) return false;
    buf[h] = v;
    head.store(next, std::memory_order_release);
    return true;
}

bool SPSCQueue::pop(int& v) {
    size_t t = tail.load(std::memory_order_relaxed);
    if (head.load(std::memory_order_acquire) == t) return false;
    v = buf[t];
    tail.store((t + 1) % N, std::memory_order_release);
    return true;
}

SPSCQueue g_q;
const int TOTAL = 100000;
bool g_ok = true;

bool try_push() {
    for (int i = 0;i < TOTAL; i++) {
        while(!g_q.push(i)) {}
    }
    return g_ok;
}

bool try_pop() {
    int v = 0;
    for (int i = 0;i < TOTAL; i++) {
        while(!g_q.pop(v)) {}
        if (!(v == i)) g_ok = false;
    }
    return g_ok;
}

int main() {
    std::thread th1(try_push);
    std::thread th2(try_pop);
    
    th1.join();
    th2.join();
    printf("ok=%d\n", g_ok);
    return 0;
}