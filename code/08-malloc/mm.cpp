// 你要写的分配器。现在这四个函数都是空的，驱动会立刻报错。
// 先让它编译通过、看到失败，再一个一个填。

#include "mm.h"
#include "memlib.h"
#include <cstddef>
#include <cstring>

void mm_init() {
}

void* mm_malloc(size_t size) {
    size_t block_size = (size + 16 + 15) & ~15;
    char* cur = (char*)mem_heap_lo();

    while (cur <= (char*)mem_heap_hi()) {
        size_t* ptr = (size_t*)cur;
        size_t h = *ptr;
        size_t sz = h & ~15;
        if ((h & 1) == 0 && block_size <= sz) {
            *ptr = sz | 1;
            return cur + 16;
        }
        cur += sz;
    }
    void* raw = mem_sbrk(block_size);
    if (raw == (void*)-1) return NULL;
    *(size_t*)raw = block_size | 1;
    return (char*)raw + 16;
}

void mm_free(void* ptr) {
    if (ptr == NULL) return;
    size_t* ptr1 = (size_t*)((char*)ptr - 16);
    *ptr1 = *ptr1 & ~1;
    return;
}

void* mm_realloc(void* ptr, size_t size) {  
    if (ptr == NULL) {
        return mm_malloc(size);
    }
    if (size == 0) {
        mm_free(ptr);
        return NULL;
    }
    void* newp = mm_malloc(size);
    if (newp == NULL) return NULL;

    size_t *h = (size_t *)((char*)ptr - 16);
    size_t old_cap = (*h & ~15) - 16;

    size_t n = (old_cap < size) ? old_cap : size;
    memcpy(newp, ptr, n);
    mm_free(ptr);
    return newp;
}
