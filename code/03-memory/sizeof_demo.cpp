#include <cstdio>

int main() {
    int** a = nullptr;        // 二级指针
    int (*b)[5] = nullptr;    // 指向「5 个 int 的数组」的指针
    int *c[5];                // 数组，5 个 int*

    // ---- 第一组：三个变量各占多少字节 ----
    printf("sizeof(a)=%zu\n", sizeof(a));
    printf("sizeof(b)=%zu\n", sizeof(b));
    printf("sizeof(c)=%zu\n", sizeof(c));
    
    // ---- 第二组：a 和 b 指向的东西各占多少字节 ----
    // 提示：*b 的类型是 int[5]
    printf("sizeof(*a)=%zu\n", sizeof(*a));
    printf("sizeof(*b)=%zu\n", sizeof(*b));

    // ---- 第三组：c 的一个元素占多少字节 ----
    printf("sizeof(c[0])=%zu\n", sizeof(c[0]));
    return 0;
}
