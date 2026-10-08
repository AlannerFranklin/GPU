#include <cstdio>
#include <new>
#include <exception>

void f(int n) {
    printf("%d ", n);
    f(n + 1);
}

void f1() {
    int big[10000000];
}

int main() {
    try {
        int* p = new int[100000000];
        int n = 0;
        f1();
        f(0);
        delete[] p;
    }
    catch (const std::bad_alloc& e) {
        printf("bad_alloc: %s\n", e.what());
        return 1;
    }
    catch (const std::exception& e) {
        printf("exception: %s\n", e.what());
        return 2;
    }
    catch (...) {
        printf("unknown exception\n");
        return 3;
    }
    return 0;
}