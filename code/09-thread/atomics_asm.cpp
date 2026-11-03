#include <atomic>

std::atomic<int> g_relaxed{0};
std::atomic<int> g_release{0};
std::atomic<int> g_seqcst{0};

void store_relaxed(int v) {
    g_relaxed.store(v, std::memory_order_relaxed);
}

void store_release(int v) {
    g_release.store(v, std::memory_order_release);
}

void store_seqcst(int v) {
    g_seqcst.store(v, std::memory_order_seq_cst);
}

int load_relaxed() {
    return g_relaxed.load(std::memory_order_relaxed);
}

int load_acquire() {
    return g_release.load(std::memory_order_acquire);
}

int load_seqcst() {
    return g_seqcst.load(std::memory_order_seq_cst);
}
