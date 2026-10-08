#include <cstdio>
#include <exception>

void f(int n) {
    printf("%d ", n);
    f(n + 1);
}

int main() {
    try {
        f(0);
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