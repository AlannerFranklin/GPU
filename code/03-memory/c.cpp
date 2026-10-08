#include <cstdio>
#include <new>
#include <exception>

int main() {
    try {
        int* p = new int[100000000];
        p[0] = 1;
        printf("ok %d\n", p[0]);
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