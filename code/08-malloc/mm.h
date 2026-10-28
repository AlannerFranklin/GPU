#pragma once
#include <cstddef>

// 你要实现的东西。
//
// ★ 对齐要求：mm_malloc 返回的指针必须是 16 的倍数。
//   x86-64 上 malloc 必须返回 16 字节对齐的地址，因为 SSE 指令
//   （movaps 等）要求操作数 16 字节对齐，而 double / long double /
//   指向任何类型的指针都可能被编译器塞进 SSE 寄存器。
//   对齐到 16 之后，任何 8 字节的数据（double、指针）也就自然对齐了。

void  mm_init();

void* mm_malloc(size_t size);

void  mm_free(void* ptr);

void* mm_realloc(void* ptr, size_t size);
