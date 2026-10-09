#include <cstdio>

template <typename T>
T mymax(T a, T b) { return a > b ? a : b; }

int main() {
    printf("%d\n", mymax(3, 5));
    //printf("%d\n", mymax(3, 5.0));
    printf("%f\n", mymax(1.5, 0.5));
    return 0;
}
