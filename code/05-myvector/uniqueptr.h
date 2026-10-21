#pragma once

// UniquePtr<T> —— 独占所有权的智能指针
// 规则：同一时刻有且只有一个 UniquePtr 拿着这个指针。谁拿着，谁负责 delete。

template <typename T>
class UniquePtr {
    public:
        // 空指针版：为了能放进 MyVector<UniquePtr<T>> 里
        // （还记得吗：new T[n] 会把每个格子都默认构造一遍，T 必须有默认构造）
        UniquePtr() : ptr(nullptr) {}

        // 【出生】接管一个裸指针
        explicit UniquePtr(T* p) : ptr(p) {}
        //  ↑ explicit = 禁止 `UniquePtr<T> x = new T;` 这种隐式转换，
        //    必须写 `UniquePtr<T> x(new T);`。防止你不小心就把裸指针的所有权送出去了。

        // 【死亡】拆掉自己拿的那栋楼
        
        ~UniquePtr() { delete ptr; }

        // ① 这两行是【禁止拷贝】。C++11 的语法叫 "= delete"：
        //    函数照样声明出来，但任何人调用它都【编译期报错】。
        //    你要自己写出来 —— 拷贝构造和拷贝赋值，两个都要禁。
        UniquePtr(const UniquePtr& o) = delete;// ① 拷贝构造  = 【出生】+【左值源】→ 造楼
        UniquePtr& operator=(const UniquePtr& o) = delete;// ③ 拷贝赋值  = 【活着】+【左值源】→ 拆楼 + 造楼

        // 【出生】移动构造：把 o 的号偷过来，再把 o 置空
        UniquePtr(UniquePtr&& o) noexcept {// ② 移动构造  = 【出生】+【右值源】→ 端楼
            ptr = o.ptr;
            o.ptr = nullptr;
        }

        // 【赋值】移动赋值：注意和移动构造差在哪（提示：我可能本来就有楼）
        UniquePtr& operator=(UniquePtr&& o) noexcept {// ④ 移动赋值  = 【活着】+【右值源】→ 拆楼 + 端楼
            if (this == &o) return *this;
            delete ptr;
            ptr = o.ptr;
            o.ptr = nullptr;
            return *this;
        }

        // —— 下面这几个是"用起来像指针"的部分 ——
        T& operator*()  const { return *ptr; }   // *p  拿到楼本身
        T* operator->() const { return ptr;  }   // p->f() 直接访问成员
        T* get()        const { return ptr;  }   // 只读地看一眼门牌号，不转移所有权

        // release：放弃所有权 —— 把号交出去，但【不】删楼。之后这个 UniquePtr 变成空的
        T* release() { T* t = ptr; ptr = nullptr; return t; }

    private:
        T* ptr;
};
