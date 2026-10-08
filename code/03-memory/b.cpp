#include <cstdio>
#include <exception>

void f1() {
    int big[10000000];
    big[0] = 1;
    printf("%d\n", big[0]);
}

int main() {
    try {
        f1();
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