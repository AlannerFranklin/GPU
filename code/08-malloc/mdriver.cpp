// 迷你 Malloc Lab 驱动。
//
// 它做三件事：
//   ① 造一个假堆（memlib），你的分配器只能从这里拿内存
//   ② 跑一批随机的 malloc / free，每个块填上可校验的花纹
//   ③ 每步都检查：指针非空、在堆内、16 对齐、不重叠、内容没被别的块踩坏
//
// 用法：
//   mdriver.exe            只测 malloc / free
//   mdriver.exe realloc    额外测 realloc

#include "mm.h"
#include "memlib.h"

#include <cstdio>
#include <cstdint>
#include <cstring>
#include <chrono>
#include <vector>
#include <algorithm>

namespace {

const size_t HEAP_SIZE   = 2u * 1024 * 1024;   // 假堆只有 2 MB，逼你复用内存
const size_t MAX_SLOTS   = 512;
const size_t MAX_PAYLOAD = 1024;
const size_t ALIGN       = 16;
const int    OPS         = 200000;
const int    VERIFY_EVERY = 20000;

struct Block {
    unsigned char* ptr  = nullptr;
    size_t         size = 0;
    unsigned char  seed = 0;
    bool           live = false;
};

std::uint64_t g_rng = 88172645463325252ull;

std::uint64_t next_rand() {
    g_rng ^= g_rng << 13;
    g_rng ^= g_rng >> 7;
    g_rng ^= g_rng << 17;
    return g_rng;
}

unsigned char pattern(unsigned char seed, size_t i) {
    return static_cast<unsigned char>(
        seed ^ static_cast<unsigned char>(static_cast<unsigned>(i) * 31u + 7u));
}

void fill_block(Block& b) {
    for (size_t i = 0; i < b.size; i++) {
        b.ptr[i] = pattern(b.seed, i);
    }
}

bool verify_block(const Block& b, const char* when) {
    for (size_t i = 0; i < b.size; i++) {
        unsigned char want = pattern(b.seed, i);
        if (b.ptr[i] != want) {
            std::printf("[FAIL] %s：块 %p 大小 %zu，第 %zu 字节被改坏（期望 %u，实际 %u）\n",
                        when, static_cast<void*>(b.ptr), b.size, i,
                        static_cast<unsigned>(want), static_cast<unsigned>(b.ptr[i]));
            return false;
        }
    }
    return true;
}

bool in_heap(const unsigned char* p, size_t n) {
    const unsigned char* lo = static_cast<const unsigned char*>(mem_heap_lo());
    const unsigned char* hi = static_cast<const unsigned char*>(mem_heap_hi());
    if (n == 0) {
        return p >= lo && p <= hi;
    }
    return p >= lo && p + (n - 1) <= hi;
}

bool check_overlap(const std::vector<Block>& blocks) {
    std::vector<const Block*> live;
    for (const Block& b : blocks) {
        if (b.live) {
            live.push_back(&b);
        }
    }
    std::sort(live.begin(), live.end(),
              [](const Block* a, const Block* b) { return a->ptr < b->ptr; });

    for (size_t k = 1; k < live.size(); k++) {
        const Block* prev = live[k - 1];
        const Block* cur  = live[k];
        if (prev->ptr + prev->size > cur->ptr) {
            std::printf("[FAIL] 两个块的用户区重叠：[%p, +%zu) 和 [%p, +%zu)\n",
                        static_cast<void*>(prev->ptr), prev->size,
                        static_cast<void*>(cur->ptr), cur->size);
            return false;
        }
    }
    return true;
}

bool verify_all(const std::vector<Block>& blocks, const char* when) {
    for (const Block& b : blocks) {
        if (b.live && !verify_block(b, when)) {
            return false;
        }
    }
    return true;
}

} // namespace

int main(int argc, char** argv) {
    bool test_realloc = false;
    for (int a = 1; a < argc; a++) {
        if (std::strcmp(argv[a], "realloc") == 0) {
            test_realloc = true;
        }
    }

    if (!mem_init(HEAP_SIZE)) {
        std::printf("[FAIL] 假堆创建失败\n");
        return 1;
    }

    mm_init();

    std::vector<Block> blocks(MAX_SLOTS);
    size_t peak_heap = 0;
    long long n_malloc = 0;
    long long n_free   = 0;

    auto t0 = std::chrono::steady_clock::now();

    for (int op = 0; op < OPS; op++) {
        size_t slot = static_cast<size_t>(next_rand() % MAX_SLOTS);
        Block& b = blocks[slot];

        bool do_malloc = !b.live;
        if (test_realloc && b.live && (next_rand() % 4) == 0) {
            // ---- realloc 路径 ----
            size_t new_size = 1 + static_cast<size_t>(next_rand() % MAX_PAYLOAD);
            unsigned char keep = b.seed;

            unsigned char* np = static_cast<unsigned char*>(mm_realloc(b.ptr, new_size));
            if (np == nullptr) {
                std::printf("[FAIL] 第 %d 步：mm_realloc(%p, %zu) 返回了 NULL\n",
                            op, static_cast<void*>(b.ptr), new_size);
                return 1;
            }
            if ((reinterpret_cast<std::uintptr_t>(np) % ALIGN) != 0) {
                std::printf("[FAIL] 第 %d 步：mm_realloc 返回的 %p 不是 %zu 字节对齐\n",
                            op, static_cast<void*>(np), ALIGN);
                return 1;
            }
            if (!in_heap(np, new_size)) {
                std::printf("[FAIL] 第 %d 步：mm_realloc 返回的 %p 不在假堆范围内\n",
                            op, static_cast<void*>(np));
                return 1;
            }

            // 保留部分必须原样保留
            size_t keep_n = (new_size < b.size) ? new_size : b.size;
            for (size_t k = 0; k < keep_n; k++) {
                if (np[k] != pattern(keep, k)) {
                    std::printf("[FAIL] 第 %d 步：mm_realloc 弄丢了前 %zu 字节里第 %zu 个字节\n",
                                op, keep_n, k);
                    return 1;
                }
            }

            b.ptr  = np;
            b.size = new_size;
            b.seed = static_cast<unsigned char>(keep ^ 0x5Au);
            fill_block(b);
            continue;
        }

        if (do_malloc) {
            size_t size = 1 + static_cast<size_t>(next_rand() % MAX_PAYLOAD);
            unsigned char* p = static_cast<unsigned char*>(mm_malloc(size));

            if (p == nullptr) {
                std::printf("[FAIL] 第 %d 步：mm_malloc(%zu) 返回了 NULL\n", op, size);
                std::printf("       假堆一共 %zu KB，已用 %zu KB。\n",
                            HEAP_SIZE / 1024, mem_heap_size() / 1024);
                std::printf("       堆还没用光就分配失败，或者你的分配器只往前走、不回头复用。\n");
                return 1;
            }
            if ((reinterpret_cast<std::uintptr_t>(p) % ALIGN) != 0) {
                std::printf("[FAIL] 第 %d 步：mm_malloc(%zu) 返回的 %p 不是 %zu 字节对齐\n",
                            op, size, static_cast<void*>(p), ALIGN);
                return 1;
            }
            if (!in_heap(p, size)) {
                std::printf("[FAIL] 第 %d 步：mm_malloc(%zu) 返回的 %p 不在假堆范围内 [%p, %p]\n",
                            op, size, static_cast<void*>(p),
                            mem_heap_lo(), mem_heap_hi());
                return 1;
            }

            b.ptr  = p;
            b.size = size;
            b.seed = static_cast<unsigned char>(slot * 131 + op);
            b.live = true;
            fill_block(b);
            n_malloc++;

            size_t used = mem_heap_size();
            if (used > peak_heap) {
                peak_heap = used;
            }
        } else {
            if (!verify_block(b, "free 之前")) {
                return 1;
            }
            mm_free(b.ptr);
            b.live = false;
            b.ptr  = nullptr;
            b.size = 0;
            n_free++;
        }

        if (op % VERIFY_EVERY == 0) {
            if (!verify_all(blocks, "批量检查")) {
                return 1;
            }
            if (!check_overlap(blocks)) {
                return 1;
            }
        }
    }

    auto t1 = std::chrono::steady_clock::now();

    if (!verify_all(blocks, "收尾检查") || !check_overlap(blocks)) {
        return 1;
    }

    for (Block& b : blocks) {
        if (b.live) {
            mm_free(b.ptr);
            b.live = false;
        }
    }

    // 全 free 之后再分配最大的块，看空间有没有被还回来
    for (size_t k = 0; k < MAX_SLOTS; k++) {
        void* p = mm_malloc(MAX_PAYLOAD);
        if (p == nullptr) {
            std::printf("[FAIL] 全部 free 之后，第 %zu 次再分配 MAX_PAYLOAD 就失败了 —— 空间没被回收\n", k + 1);
            return 1;
        }
        blocks[k].ptr  = static_cast<unsigned char*>(p);
        blocks[k].size = MAX_PAYLOAD;
        blocks[k].seed = static_cast<unsigned char>(k);
        blocks[k].live = true;
        fill_block(blocks[k]);
    }
    if (!verify_all(blocks, "回收后检查")) {
        return 1;
    }

    double ns = std::chrono::duration<double, std::nano>(t1 - t0).count();
    double per_op = ns / static_cast<double>(OPS);

    std::printf("[OK] %d 步全部通过\n", OPS);
    std::printf("     malloc %lld 次，free %lld 次\n", n_malloc, n_free);
    std::printf("     假堆 %zu KB，峰值用了 %zu KB\n", HEAP_SIZE / 1024, peak_heap / 1024);
    std::printf("     总耗时 %.1f ms，平均 %.1f ns/次\n", ns / 1e6, per_op);
    return 0;
}
