#pragma once
#include <cstddef>

// 假堆：mem_init 一次性向操作系统要一块 max_size 字节的区域，
// 之后 mm_malloc 只能通过 mem_sbrk 从这块区域里往上切。
//
// 这样做是为了把「向操作系统要内存」和「管理内存」分开：
// 前者是操作系统的事，后者才是你要写的分配器的事。

// 创建假堆。成功返回 true。
bool mem_init(size_t max_size);

// 把堆的边界向上推 incr 字节，返回推进前的边界（也就是新切出来的那块的起点）。
// 空间不够时返回 (void*)-1。
void* mem_sbrk(long long incr);

// 假堆的起始地址（最低有效地址）。
void* mem_heap_lo();

// 假堆当前已切到的最高有效地址。
void* mem_heap_hi();

// 假堆当前已经切出去多少字节。
size_t mem_heap_size();
