#include "mystring.h"
#include "uniqueptr.h"
#include <cstdio>
#include <utility>
#include <vector>

int main() {
    UniquePtr<MyString> p(new MyString("hello"));

    printf("--- 用起来像指针 ---\n");
    printf("*p   = %s\n", (*p).data);
    printf("p->  = %s\n", p->data);
    printf("get()= %p\n", (void*)p.get());

    printf("--- 下面这行应该编译失败 ---\n");
    //UniquePtr<MyString> q = p;      // ← ① 拷贝。编译期就该报错
    UniquePtr<MyString> q = std::move(p);    // 点：移动构造
    UniquePtr<MyString> r;
    r = std::move(q);                        // 点：移动赋值
    printf("--- 搬完之后 ---\n");
    printf("p=%p  q=%p  r=%p\n", (void*)p.get(), (void*)q.get(), (void*)r.get());

    printf("--- 塞进 std::vector ---\n");
    std::vector<UniquePtr<MyString>> vec;
    vec.push_back(std::move(r));
    vec.push_back(UniquePtr<MyString>(new MyString("world")));
    vec.push_back(UniquePtr<MyString>(new MyString("again")));

    printf("vec[0]=%s  vec[1]=%s  vec[2]=%s\n",
           vec[0]->data, vec[1]->data, vec[2]->data);

    return 0;
}
