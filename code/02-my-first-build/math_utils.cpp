#include "math_utils.h"

/*int add(int a, int b) {
    return a + b;
}*/
int add(int a) {
    return a;
}

int factorial(int n) {
    int num = 1;
    for(int i = 2; i <= n;i++) {
        num *= i;
    }
    return num;
}