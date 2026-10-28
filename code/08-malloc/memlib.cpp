#include "memlib.h"

#include <cstdlib>

namespace {

char* g_heap_lo   = nullptr;   // 假堆的起点
char* g_brk       = nullptr;   // 已经切到哪了
char* g_heap_hi   = nullptr;   // 假堆的终点（不含）

} // namespace

bool mem_init(size_t max_size) {
    g_heap_lo = static_cast<char*>(std::malloc(max_size));
    if (g_heap_lo == nullptr) {
        return false;
    }
    g_brk     = g_heap_lo;
    g_heap_hi = g_heap_lo + max_size;
    return true;
}

void* mem_sbrk(long long incr) {
    if (incr < 0) {
        return (void*)-1;
    }
    if (g_brk + incr > g_heap_hi) {
        return (void*)-1;          // 假堆用完了
    }
    char* old_brk = g_brk;
    g_brk += incr;
    return old_brk;
}

void* mem_heap_lo() {
    return g_heap_lo;
}

void* mem_heap_hi() {
    return g_brk - 1;
}

size_t mem_heap_size() {
    return static_cast<size_t>(g_brk - g_heap_lo);
}
