#include <cstdio>

int global = 0;
int count = 0;
void printaddress(int zone, int count) {
    count++;
    if (count > 4) return;
    printf("%p\n", (void*)&zone);
    printaddress(zone, count);
}

int main() {
    int zone = 0;
    int *p = new int[3]{1, 2, 3};
    const char* lit = "hello";
    printf("%p\n", (void*)&global);
    printf("%p\n", (void*)&zone);
    printf("%p\n", (void*)p);
    printf("%p\n", (void*)lit);

    printaddress(zone, 0);
    printf("******************\n");

    int x1 = 0, x2 = 0, x3 = 0, x4 = 0 , x5 = 0;
    printf("%p\n", (void*)&x1);
    printf("%p\n", (void*)&x2);
    printf("%p\n", (void*)&x3);
    printf("%p\n", (void*)&x4);
    printf("%p\n", (void*)&x5);
    return 0;
}