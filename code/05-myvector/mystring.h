#pragma once
#include <cstdio>      // printf
#include <cstring>     // strlen

// ================================================================
//  MyString —— 一个"拥有资源"的类
//
//  它 owns 一块堆内存：
//      MyString 对象（栈上，16 字节：一个指针 + 一个 len）
//          │ data
//          ▼
//      一排 char（堆上，new char[len+1] 造出来的）   ← 注意：这是 char，不是类
//
//  ⚠️ 只有 data 那一块需要还。对象自己不用管。
//  ⚠️ char 没有析构函数，所以 [xigou] 打的永远是 MyString 本身，不会是那块 char。
//
//  ┌──────── 出生（构造函数，3 个）────────┐
//  │ MyString()              空串         │
//  │ MyString(const char*)   "hello" 这种 │
//  │ MyString(const MyString&)  拷贝      │
//  │ MyString(MyString&&)       移动      │
//  └──────────────────────────────────────┘
//  ┌──────── 赋值（operator=，2 个）───────┐
//  │ operator=(const MyString&)  拷贝赋值 │
//  │ operator=(MyString&&)       移动赋值 │
//  └──────────────────────────────────────┘
//  ┌──────── 死亡（1 个）──────────────────┐
//  │ ~MyString()                          │
//  └──────────────────────────────────────┘
//
//  ★ 最容易搞错的一件事：
//      MyString b = a;   ← 这一行【有 new 一样的动作但其实是出生】→ 走【拷贝构造】
//      b = a;            ← 这一行【b 早就活着了】              → 走【拷贝赋值】
//    分辨方法：这一行有没有声明一个新名字？有 → 出生；没有 → 赋值。
// ================================================================

class MyString {
    public:
        char* data;     // 第 2 层的门牌号（nullptr = 没有楼，空串也是这种状态）
        size_t len;     // 字符串长度，不含结尾的 '\0'
                        // ⚠️ 实际的楼是 len + 1 字节，多出来的那 1 个装 '\0'

        // 计数器：定义在 main.cpp 里（static 成员必须在 .cpp 里定义一次）。
        // 写这个类的目的就是数清楚"每种函数到底被调了几次"。
        static int move_ctor_count;
        static int move_assign_count;
        static int copy_ctor_count;
        static int copy_assign_count;

        // ——【出生】默认构造：造一个空串 ——
        // ⚠️ 这个构造函数是【为了 MyVector 才补的】。
        //    MyVector 里那句 `new T[n]` 会把每个格子都默认构造一遍，
        //    T 没有默认构造就编不过（error C2512）。
        //    std::vector 不需要 T 有默认构造 —— 它用 placement new 只造 size_ 个。
        MyString() : data(nullptr), len(0) {}
        //          ↑ 冒号后面叫【初始化列表】。
        //            效果和写在函数体里 `data = nullptr; len = 0;` 一样，
        //            区别是初始化列表在"出生"时就赋好值，函数体里是"出生后再赋值"。

        // ——【出生】用一个 C 字符串造 ——
        // 这是唯一会自己造楼的一个构造函数。
        MyString(const char* s) {
            len = strlen(s);                // 先量长度（不含 '\0'）
            data = new char[len + 1];       // ★ 造楼，多要 1 个字节给 '\0'
            for(int i = 0;i < len;i++) {
                data[i] = s[i];             // 逐字节抄
            }
            data[len] = '\0';               // 手动补结尾。
                                            // ⚠️ 不补的话，printf("%s") 会一直往后读，
                                            //    直到碰上内存里某个 0 —— 这就是"乱码/越界读"
            printf("[gouzao]%p %p %s\n", (void*)this, (void*)data, data);
            //                             ↑this=自己在哪   ↑新楼在哪   ↑楼里的内容
        }

        // ——【出生】拷贝构造：MyString b = a; 或者 MyString b(a); 走这里 ——
        // 关键：绝【不】能写成 data = o.data；
        //       那是两个人共用一个门牌号 → 出了作用域被 delete 两次 → 崩。
        //       自己也有 new，要删就删自己的。
        MyString(const MyString& o) {
            len = o.len;                    // 先抄长度
            data = new char[len + 1];       // ★ 自己另盖一栋
            for(int i = 0;i < len;i++) {
                data[i] = o.data[i];        // 抄内容，不是抄门牌号
            }
            data[len] = '\0';
            printf("[copy ctor] this=%p, data=%p, from=%p, s=\"%s\"\n",
           (void*)this, (void*)data, (void*)o.data, data);
            ++copy_ctor_count;              // ★ 计数器放这里 → 拷贝构造
        }

        // ——【出生】移动构造：MyString b = std::move(a); 走这里 ——
        // 和拷贝构造最大的区别：这里【没有 new】。
        // 因为"出生不用清理" —— 我刚生出来，身上没有旧资源，所以不需要 delete。
        // 代价：o 被掏空，不能再用了（它已经变成 data=nullptr, len=0 的空壳）。
        MyString(MyString&& o) noexcept {
            data = o.data;                  // 直接把 o 的门牌号拿过来（楼没动，只是换了主人）
            len = o.len;
            o.data = nullptr;               // ★ 必须把 o 掏空 —— 否则 o 一死，
                                            //   ~MyString 会把这个号 delete 掉，
                                            //   而楼现在是我的了 → 我手里的号变成野指针
            o.len = 0;
            ++move_ctor_count;
            printf("[move ctor] this=%p, data=%p, from(o.data after null)=%p, s=\"%s\"\n",
           (void*)this, (void*)data, (void*)o.data, data);
        }

        // ——【赋值】移动赋值：b = std::move(a); 走这里 ——
        // 和移动构造比，只多了一件事：我【原来已经有楼了】，
        // 所以第一件事是把自己那栋拆掉。
        // 和拷贝赋值比：那边是 new 一栋新楼，这边是直接抢 o 的楼。
        MyString& operator=(MyString&& o) noexcept {
            printf("[move assign] BEFORE: this=%p, this->data=%p, o.data=%p, o.len=%zu\n",
           (void*)this, (void*)data, (void*)o.data, o.len);
            if (this == &o) return *this;   // 自赋值守卫：a = std::move(a);
                                            // 不守的话下一行先拆了自己的楼，
                                            // 再 data = o.data 拿到的就是刚被拆掉的门牌号
            delete[] data;                  // ★ 先拆自己的旧楼（出生时不用这行，赋值时才要）
            data = o.data;                  // 再抢 o 的
            len = o.len;
            o.data = nullptr;               // 掏空 o，两个成员都要掏
            o.len = 0;
            ++move_assign_count;
            printf("[move assign] AFTER:  this=%p, this->data=%p, o.data=%p, o.len=%zu, s=\"%s\"\n",
           (void*)this, (void*)data, (void*)o.data, o.len, data);
            return *this;                   // 返回自己的引用，支持 a = b = c 连写
        }

        // ——【赋值】拷贝赋值：b = a; 走这里 ——
        // 三件事，顺序不能乱：① 守卫 ② 拆自己的 ③ 造新的、抄内容
        MyString& operator=(const MyString& o) {
            printf("[copy assign] this=%p, old_data=%p, from=%p\n",
           (void*)this, (void*)data, (void*)o.data);
            if (this == &o) {               // ★ 自己给自己赋值：b = b;
                printf("[copy assign] self assignment, skip\n");
                return *this;               //   不守的话：下面 delete[] data 先把楼拆了，
                                            //   接着 o.data 读的就是已经还给堆管理器的内存
                                            //   → use-after-free
            }
            delete[] data;                  // ★ 先还掉自己原来那栋楼
            len = o.len;
            data = new char[len + 1];       // 再盖一栋新的
            for(int i = 0;i < len;i++) {
                data[i] = o.data[i];
            }
            data[len] = '\0';
            ++copy_assign_count;
            printf("[copy assign] this=%p, data=%p, from=%p, s=\"%s\"\n",
           (void*)this, (void*)data, (void*)o.data, data);
            return *this;
        }

        // ——【死亡】析构：对象被销毁时自动调用 ——
        // 什么时候会死？
        //   · 局部变量活到作用域结束（比如函数返回）
        //   · delete p / delete[] p（数组里每个元素都要死一次）
        //   · 它作为成员/元素被别人带着一起死
        //   · 临时对象活到"整条完整语句"结束
        ~MyString() {
            printf("[xigou]%p %p %s\n", (void*)this, (void*)data, data);
            //      ↑ 先打印再删！反过来的话，%s 读的就是刚被你还掉的内存 → 乱码
            delete[] data;                  // ★ delete[] nullptr 是合法的空操作，不用担心
        }
};

// ⚠️ 注意：这里【没有】计数器变量的定义。
//    static 成员必须在某个 .cpp 里定义一次，现在是在 main.cpp 里写的：
//        int MyString::move_ctor_count = 0;
//    ...
//    如果哪天报 LNK2001 找不到这个符号，就是定义漏了或者两个 TU 各定义了一次（LNK2005）。
