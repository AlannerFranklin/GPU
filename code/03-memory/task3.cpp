#include <cstdio>
#include <cstddef>
#include <cstring>   // 用于对比 memcmp

struct Node {
    int a;
    double b;
    char c[8];
};

void* my_memcpy(void* dst, const void* src, size_t n) {
    const char* ptr1 = (const char*)src;
    char* ptr2 = (char*)dst;
    for (size_t i = 0; i < n; i++) {
        *(ptr2 + i) = *(ptr1 + i);
    }
    return dst;
}

int main() {
    // ---------- 测试 1：基本 int 数组 ----------
    {
        int src[5] = {1, 2, 3, 4, 5};
        int dst[5] = {0, 0, 0, 0, 0};

        void* ret = my_memcpy(dst, src, sizeof(src));

        printf("测试1 int 数组:\n");
        printf("  返回值 == dst ? %s\n", ret == dst ? "是" : "否");
        bool ok = (memcmp(dst, src, sizeof(src)) == 0);
        printf("  内容一致 ? %s\n", ok ? "是" : "否");
        printf("  dst = ");
        for (int i = 0; i < 5; i++) printf("%d ", dst[i]);
        printf("\n\n");
    }

    // ---------- 测试 2：结构体 ----------
    {
        Node src;
        src.a = 42;
        src.b = 3.14;
        const char* text = "hello";
        for (int i = 0; i < 8; i++) {
            src.c[i] = (i < 5) ? text[i] : '\0';
        }

        Node dst;
        my_memcpy(&dst, &src, sizeof(Node));

        printf("测试2 结构体:\n");
        printf("  dst.a = %d\n", dst.a);
        printf("  dst.b = %f\n", dst.b);
        printf("  dst.c = %s\n", dst.c);
        printf("  按字节一致 ? %s\n\n",
               memcmp(&dst, &src, sizeof(Node)) == 0 ? "是" : "否");
    }

    // ---------- 测试 3：字符串（含 '\0'）----------
    {
        const char* src = "Hello, CUDA!";
        char dst[32] = {0};

        my_memcpy(dst, src, strlen(src) + 1);  // +1 把 '\0' 也拷过去

        printf("测试3 字符串:\n");
        printf("  dst = \"%s\"\n", dst);
        printf("  长度一致 ? %s\n\n",
               strlen(dst) == strlen(src) ? "是" : "否");
    }

    // ---------- 测试 4：部分拷贝（只拷前 n 字节）----------
    {
        int src[4] = {10, 20, 30, 40};
        int dst[4] = {0, 0, 0, 0};

        // 只拷前两个 int
        my_memcpy(dst, src, 2 * sizeof(int));

        printf("测试4 部分拷贝:\n");
        printf("  dst = %d %d %d %d\n", dst[0], dst[1], dst[2], dst[3]);
        printf("  期望 10 20 0 0\n\n");
    }

    // ---------- 测试 5：重叠内存（危险区，看行为）----------
    {
        char buf[16] = "abcdefghij";

        // 源和目标重叠，用 memcpy 是未定义行为，这里只观察
        my_memcpy(buf + 2, buf, 8);

        printf("测试5 重叠拷贝（前向覆盖）:\n");
        printf("  buf = ");
        for (int i = 0; i < 10; i++) {
            printf("%c", buf[i] ? buf[i] : '.');
        }
        printf("\n");
        printf("  提示：memcpy 不保证重叠正确，需要 memmove 才安全\n\n");
    }

    // ---------- 测试 6：n = 0 ----------
    {
        int src = 123;
        int dst = 456;

        my_memcpy(&dst, &src, 0);  // 不拷贝

        printf("测试6 n=0:\n");
        printf("  dst 应保持 456，实际 = %d\n\n", dst);
    }

    return 0;
}