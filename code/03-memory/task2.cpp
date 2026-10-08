#include <cstdio>


int main() {
    int arr[5] = {1, 2, 3, 4, 5};
    const char* ch = "abcde";
    printf("%p %p\n", (void*)&arr[0], (void*)&arr[1]);
    printf("%p %p\n", (void*)&ch[0], (void*)&ch[1]);
    printf("%d %d\n", arr[2], 2[arr]);
    
    int *p = new int(10);
    char *q = new char(10);
    printf("%p  %p\n", (void*)p, (void*)(p + 1));
    printf("%p  %p\n", (void*)q, (void*)(q + 1));

}