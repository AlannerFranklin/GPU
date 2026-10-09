#include "myvector.h"
#include <cstdio>
#include <utility>

int main() {
    MyVector<int> v;
    int a = 1;

    printf("--- 传左值 ---\n");
    v.push_back(a);

    printf("--- 传临时对象 ---\n");
    v.push_back(2);

    printf("--- 传 std::move(a) ---\n");
    v.push_back(std::move(a));

    return 0;
}
