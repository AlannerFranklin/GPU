#include <cstdio>
#include <cstring>
#include <cstddef>
#include <vector>

class MyString {
    public:
        char* data;
        size_t len;

        static int move_ctor_count;
        static int move_assign_count;
        static int copy_ctor_count;

        MyString(const char* s) {
            len = strlen(s);
            data = new char[len + 1];
            for(int i = 0;i < len;i++) {
                data[i] = s[i];
            }
            data[len] = '\0';
            printf("[gouzao]%p %p %s\n", (void*)this, (void*)data, data);
        }

        MyString(const MyString& o) {
            len = o.len;
            data = new char[len + 1];
            for(int i = 0;i < len;i++) {
                data[i] = o.data[i];
            }
            data[len] = '\0';
            printf("[copy ctor] this=%p, data=%p, from=%p, s=\"%s\"\n",
           (void*)this, (void*)data, (void*)o.data, data);
            ++copy_ctor_count;
        }

        MyString(MyString&& o) noexcept {
            data = o.data;
            len = o.len;
            o.data = nullptr;
            o.len = 0;
            ++move_ctor_count;
            printf("[move ctor] this=%p, data=%p, from(o.data after null)=%p, s=\"%s\"\n",
           (void*)this, (void*)data, (void*)o.data, data);
        }

        MyString& operator=(MyString&& o) noexcept {
            printf("[move assign] BEFORE: this=%p, this->data=%p, o.data=%p, o.len=%zu\n",
           (void*)this, (void*)data, (void*)o.data, o.len);
            if (this == &o) return *this;
            delete[] data;
            data = o.data;
            len = o.len;
            o.data = nullptr;
            o.len = 0;
            ++move_assign_count;
            printf("[move assign] AFTER:  this=%p, this->data=%p, o.data=%p, o.len=%zu, s=\"%s\"\n",
           (void*)this, (void*)data, (void*)o.data, o.len, data);
            return *this;
        }

        MyString& operator=(const MyString& o) {
            printf("[copy assign] this=%p, old_data=%p, from=%p\n",
           (void*)this, (void*)data, (void*)o.data);
           if (this == &o) {
                printf("[copy assign] self assignment, skip\n");
                return *this;
           }
           delete[] data;
           len = o.len;
           data = new char[len + 1];
           for(int i = 0;i < len;i++) {
                data[i] = o.data[i];
            }
            data[len] = '\0';
            printf("[copy assign] this=%p, data=%p, from=%p, s=\"%s\"\n",
           (void*)this, (void*)data, (void*)o.data, data);
            return *this;
        }

        ~MyString() {
            printf("[xigou]%p %p %s\n", (void*)this, (void*)data, data);
            delete[] data;
        }
};

int MyString::move_ctor_count = 0;
int MyString::move_assign_count = 0;
int MyString::copy_ctor_count = 0;

int main() {

    MyString x("hello");
    printf("before: x.data=%p\n", (void*)x.data);
    MyString&& r = std::move(x);          // ← 只有这一句，没调用任何函数
    printf("after : x.data=%p\n", (void*)x.data);


    printf("=== 1. 普通构造 ===\n");
    MyString a("hello");

    printf("\n=== 2. 拷贝构造 ===\n");
    MyString b = a;

    printf("\n=== 3. 拷贝赋值 ===\n");
    MyString c("world");
    c = a;

    printf("\n=== 4. 自赋值 ===\n");
    c = c;

    printf("\n=== 5. 地址对比 ===\n");
    printf("%p %p %p \n", (void*)a.data, (void*)b.data, (void*)c.data);

    printf("\n=== 6. 移动构造：push_back 临时对象 ===\n");
    {
        std::vector<MyString> v;
        v.push_back("hello");

        printf("\n=== 7. 移动赋值：v[0] = 临时对象 ===\n");
        v[0] = "world";

        printf("\nmove_ctor_count=%d\n", MyString::move_ctor_count);
        printf("move_assign_count=%d\n", MyString::move_assign_count);
        printf("copy_ctor_count=%d\n", MyString::copy_ctor_count);
    }

    std::vector<MyString> arr;
    //arr.reserve(1000);
    for (int i = 0;i < 1000;i++) {
        arr.push_back("s");
    }
    printf("\nmove_ctor_count=%d\n", MyString::move_ctor_count);
    printf("move_assign_count=%d\n", MyString::move_assign_count);
    printf("copy_ctor_count=%d\n", MyString::copy_ctor_count);

    printf("\n=== main 结束，开始析构 ===\n");

    return 0;
}