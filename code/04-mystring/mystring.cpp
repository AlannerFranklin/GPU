#include <cstdio>
#include <cstring>

class MyString {
    public:
        char* data;
        size_t len;

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
            printf("[copy ctor] this=%p, data=%p, from=%p, s=\"%s\"\n",
           (void*)this, (void*)data, (void*)o.data, data);
            return *this;
        }
        ~MyString() {
            printf("[xigou]%p %p %s\n", (void*)this, (void*)data, data);
            delete[] data;
        }
};

int main() {
    printf("=== 1. 普通构造 ===\n");
    MyString a("hello");

    printf("\n=== 2. 拷贝构造 ===\n");
    MyString b = a;

    printf("\n=== 3. 拷贝赋值 ===\n");
    MyString c("world");
    c = a;

    printf("\n=== 4. 自赋值 ===\n");
    c = c;

    printf("\n=== 5. operator<< ===\n");
    printf("%p %p %p \n", &a, &b, &c);

    printf("\n=== main 结束，开始析构 ===\n");
    return 0;
}