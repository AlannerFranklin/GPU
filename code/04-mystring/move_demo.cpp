#include <cstdio>
#include <string>
#include <utility>      // std::move 在这里

int main() {
    std::string a = "hello, this is a long string";   // 28 个字符
    const char* p1 = a.data();                        // 记下 a 的缓冲区地址

    std::string b = std::move(a);
    //std::string b = a;
    const char* p2 = b.data();                        // 记下 b 的缓冲区地址

    // 你来写三行 printf：
    // 1. 打印 p1、p2 的地址，以及 p1 == p2 是否成立
    // 2. 打印 a 的内容（a.c_str()）和长度（a.size()）
    // 3. 打印 b 的内容和长度
    printf("p1=%p p2=%p same=%d\n", p1, p2, p1 == p2);
    printf("a=[%s] size=%zu\n", a.c_str(), a.size());
    printf("b=[%s] size=%zu\n", b.c_str(), b.size());
    return 0;
}
