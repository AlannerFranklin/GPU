#include "mystring.h"
#include "myvector.h"
#include <utility>

int MyString::move_ctor_count   = 0;
int MyString::move_assign_count = 0;
int MyString::copy_ctor_count   = 0;
int MyString::copy_assign_count = 0;

int main() {
    MyVector<MyString> v;
    for (int i = 0; i < 1000; i++) {
        v.push_back("s");
    }

    printf("\n=== 最终 ===\n");
    printf("copy_ctor=%d move_ctor=%d copy_assign=%d move_assign=%d\n",
           MyString::copy_ctor_count, MyString::move_ctor_count,
           MyString::copy_assign_count, MyString::move_assign_count);
    printf("size=%zu cap=%zu\n", v.size(), v.capacity());
    return 0;
}
