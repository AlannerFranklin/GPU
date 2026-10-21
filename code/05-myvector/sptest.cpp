#include "sharedptr.h"
#include "mystring.h"

int main() {
    SharedPtr<MyString> a(new MyString("hello"));
    SharedPtr<MyString> b = a;
    SharedPtr<MyString> c = b;
    SharedPtr<MyString> d(new MyString("D"));
    b = d;  
    printf("%p %p %p\n", (void*)a.get(), (void*)b.get(), (void*)c.get());
    printf("%d %d %d\n", a.ref_count(), b.ref_count(), c.ref_count());
    printf("a=%d b=%d c=%d d=%d\n", a.ref_count(), b.ref_count(), c.ref_count(), d.ref_count());

    return 0;
}