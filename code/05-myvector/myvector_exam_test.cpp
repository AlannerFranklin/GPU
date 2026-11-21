// MyVector 摸底测试：扩容 / 拷贝构造 / 拷贝赋值 / 析构
#include "myvector_exam.h"
#include <cstdio>
#include <cstdlib>

#ifdef _MSC_VER
#include <crtdbg.h>
#endif

static int g_fail = 0;
#define CHECK(cond, msg)                                  \
    do {                                                  \
        if (cond) printf("[OK]   %s\n", msg);             \
        else { printf("[FAIL] %s\n", msg); g_fail++; }    \
    } while (0)

int main() {
#ifdef _MSC_VER
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    // ---- 1. 基本功能 + 扩容 ----
    {
        MyVector<int> v;
        CHECK(v.size() == 0 && v.capacity() == 0, "空容器 size=0 capacity=0");

        for (int i = 0; i < 100; i++) v.push_back(i);

        CHECK(v.size() == 100, "push_back 100 个后 size=100");
        CHECK(v.capacity() >= 100, "capacity >= 100（已扩容）");

        int ok = 1;
        for (int i = 0; i < 100; i++) if (v[i] != i) ok = 0;
        CHECK(ok, "所有元素内容正确");

        v[50] = 9999;
        CHECK(v[50] == 9999, "operator[] 可写（返回引用）");
    }

    // ---- 2. 拷贝构造（深拷贝）----
    {
        MyVector<int> a;
        for (int i = 0; i < 10; i++) a.push_back(i);

        MyVector<int> b = a;            // 拷贝构造

        CHECK(b.size() == 10 && b.capacity() == a.capacity(), "拷贝构造：size/capacity 一致");

        b[0] = 777;                     // 改 b 不应影响 a
        CHECK(a[0] == 0 && b[0] == 777, "拷贝构造：深拷贝（改 b 不影响 a）");
    }

    // ---- 3. 拷贝赋值 ----
    {
        MyVector<int> a;
        for (int i = 0; i < 10; i++) a.push_back(i * 10);

        MyVector<int> c;
        c.push_back(-1);                // c 已有一块内存
        c = a;                          // 拷贝赋值

        CHECK(c.size() == 10, "拷贝赋值：size 正确");
        CHECK(c[0] == 0 && c[9] == 90, "拷贝赋值：内容正确");

        c[0] = 555;
        CHECK(a[0] == 0 && c[0] == 555, "拷贝赋值：深拷贝（改 c 不影响 a）");

        // 自赋值
        c = c;
        CHECK(c.size() == 10 && c[0] == 555, "自赋值不破坏对象");
    }

    // ---- 4. 析构 / 泄漏 ----
    // 上面所有对象的析构由作用域结束触发；MSVC 的 _CrtDumpMemoryLeaks 会在退出时报告
    printf("\n=== 结果: %d 个失败 ===\n", g_fail);
    return g_fail == 0 ? 0 : 1;
}
