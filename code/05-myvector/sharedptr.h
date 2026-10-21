#pragma once

template <typename T>
class SharedPtr {
    public:
        // 【出生】接管一个裸指针
        explicit SharedPtr(T* p) {
            ptr   = p;
            count = new int(1);          // ① 新建账本。初始值应该是几？
        }

        // 【出生】拷贝构造
        SharedPtr(const SharedPtr& o) {
            ptr = o.ptr;// ② 三行：抄楼号、抄账本、记账
            count = o.count;
            (*count)++;
        }

        // 【死亡】
        ~SharedPtr() {
            (*count)--;                   // ③ 你刚答对的那四行
            if((*count) == 0) {
                delete ptr;
                delete count;
            }
        }

        SharedPtr& operator=(const SharedPtr& o) {
            if (this == &o) return *this;
            (*count)--;
            if ((*count) == 0) {
                delete ptr;
                delete count;
            }
            ptr = o.ptr;
            count = o.count;
            (*count)++;
            return *this;
        }

        T& operator*()  const { return *ptr; }
        T* operator->() const { return ptr;  }
        T* get()        const { return ptr;  }
        int ref_count() const {return *count;}

    private:
        T*   ptr;      // 楼
        int* count;    // 账本（堆上那个共用 int 的地址）
};
