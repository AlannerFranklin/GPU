#pragma once
#include <cstdio>      // printf
#include <cstddef>     // size_t
#include <utility>     // std::move
// ⚠️ 规矩：这个文件用到的每一个名字，对应的头文件都要自己 include，
//    不能指望"别人先 include 过了，轮到我时就有了"。
//    MSVC 对模板里的名字查找偷懒，会纵容你；gcc/clang 当场编不过。
//    验证方法：写一个只 #include "myvector.h" 的 cpp，然后 cl /c 它。

// ================================================================
//  MyVector<T> —— 自己能扩容的动态数组
//
//  三层结构（T = MyString 时尤其要看清楚）：
//
//    第 1 层   MyVector 对象本身    24 字节，住在【栈】上，全程序只有一个
//                │ data
//                ▼
//    第 2 层   一排 T 对象          cap_ 个，住在【堆】上，由 new T[n] 造出来
//                │ data
//                ▼
//    第 3 层   T 自己拥有的东西      比如 T=MyString 时，每个 MyString 各自的 char 数组
//
//  本文件里"出生 / 死亡 / 赋值"只发生在三处：
//     new T[n]       → 出生 n 次（这 n 个 T 全部被【默认构造】）
//     delete[] p     → 死亡（那一排有几个，就死几个）
//     data[i] = x    → 赋值，【不是】生死
//
//  ⚠️ 已知欠债：因为用了 new T[n]，本实现比 std::vector 多要求一条 ——
//     T 必须能默认构造。std::vector 用 placement new（住几个造几个），没这个要求。
// ================================================================

template <typename T>
class MyVector{
    public :
        MyVector();                                 // 出生：空数组
        ~MyVector();                                // 死亡：还掉 data 那一排

        size_t size() const;                        // 工具：住了几个人
        size_t capacity() const;                    // 工具：有几个房间
        T& operator[](size_t i);                    // 工具：拿第 i 个格子（返回引用）

        void reserve(size_t n);                     // 生死现场：保证至少有 n 个房间
        void push_back(const T& v);                 // 往末尾放一个【左值】
        void push_back(T&& v);                      // 往末尾放一个【右值】

        // —— 五法则：上周在 MyString 上学的那套。
        //    区别只是成员从 1 个（len）变成了 2 个（size_ 和 cap_），
        //    所以每一处都要多想一遍："cap_ 呢？"
        MyVector(const MyVector& o);                // 出生：拷贝构造
        MyVector(MyVector&& o) noexcept;            // 出生：移动构造
        MyVector& operator=(const MyVector& o);     // 赋值：拷贝赋值
        MyVector& operator=(MyVector&& o) noexcept; // 赋值：移动赋值

    private:
        T *data;        // 第 2 层的门牌号（nullptr = 一栋楼都没有）
        size_t size_;   // 住了几个人（有效元素个数）
        size_t cap_;    // 有几个房间（data 那一排总共多长）

        // ⚠️ size_ 和 cap_ 是两回事，而且任何时刻三个成员都必须【互相自洽】。
        //    例：data = nullptr 但 cap_ = 4，就是自相矛盾的状态 ——
        //    push_back 会信 cap_ 说"还有空位"，跳过扩容，然后去写 nullptr → 崩溃。
};

// ================================================================
//  【出生】空数组：三个成员一起归零
//  这个函数没打日志，所以程序开头你看不到任何输出 —— 但它真的跑了
// ================================================================
template <typename T>
MyVector<T>::MyVector() {
    data = nullptr;     // 一栋楼都没有
    size_ = 0;          // 住了 0 个人
    cap_ = 0;           // 有 0 个房间
}

// ================================================================
//  【工具】返回【引用】而不是值 —— 这样 v[0] 出现在等号左边时才能写进去
// ================================================================
template <typename T>
T& MyVector<T>::operator[](size_t i) {
    return data[i];     // 不做越界检查。这就是它和 std::vector::at 的区别
}

// ================================================================
//  【死亡】把楼还回去
//  注意：对象自己（24 字节）在【栈】上，栈是自动回收的，不用你管。
//        要手动还的只有 data 指着的那一排（第 2 层）。
// ================================================================
template <typename T>
MyVector<T>::~MyVector() {
    delete[] data;      // 那一排有几个对象就析构几个（= 当前的 cap_ 个，不是 size_ 个）
}

template <typename T>
size_t MyVector<T>::size() const{
    return size_;
}

template <typename T>
size_t MyVector<T>::capacity() const {
    return cap_;
}

// ================================================================
//  【生死现场】reserve(n)：保证至少有 n 个房间
//
//  三步走，顺序不能换：
//    ① 造一排新的        → 出生 n 次
//    ② 把住户搬过去      → 赋值 size_ 次（【不是】n 次）
//    ③ 拆掉老的那一排    → 死亡：老的 cap_ 次
//
//  "旧数据不能先 delete[]"：先拆了，第 ② 步就是在读一块已经还给堆管理器的内存。
//   这叫 use-after-free。
// ================================================================
template <typename T>
void MyVector<T>::reserve(size_t n) {
    if (n <= cap_) return;              // ★ 只增不减：够住就原地不动，size_ 一个都不碰

    T* data1 = new T[n];                // ★ 出生 n 次！这 n 个 T 全部被【默认构造】——
                                        //   这就是 MyString 必须补一个默认构造的原因。

    for(size_t i = 0;i < size_;i++) {   // ★ 上界是 size_（住了几个人）——
                                        //   不是 n（那是房间数），也不是 cap_（那也是房间数）
        data1[i] = std::move(data[i]);  // ★ 赋值，【不是】出生：这个格子上一行刚出生过
        //data1[i] = data[i];
                                        //   std::move 让这里走【移动赋值】而不是【拷贝赋值】。
                                        //   去掉它，1000 次 push_back 里的搬家会变成深拷贝
    }

    delete[] data;                      // ★ 死亡：老那一排【整排】拆掉（= 老的 cap_ 个）
                                        //   data 是 nullptr 时这是合法空操作
                                        //   （第一次 push_back 走的就是这条路）
    data = data1;                       // 换门牌号：现在 data 和 data1 装的是同一个号

    // delete[] data1;                  // ✘✘✘ 千万别加回来！
                                        //   data1 和 data 装的是同一个门牌号。
                                        //   "删掉 data1"不是"把这个变量作废"，是"拆掉那个号指向的楼"。
                                        //   拆了 → 下次 delete[] data 就是二次释放 → 0xC0000374

    cap_ = n;                           // 记新容量。注意 size_ 自始至终没变
}

// ================================================================
//  【放一个左值进去】 v 是【有名字】的变量 → 走这里
// ================================================================
template <typename T>
void MyVector<T>::push_back(const T& v) {
    if (size_ + 1 > cap_) {             // 要放第 size_+1 个，还有空位吗？
        if (cap_ != 0)
            reserve(cap_ * 2);          // 没空位 → 翻倍
        else
            reserve(size_ + 1);         // ★ cap_ == 0 必须单独处理：
                                        //   0 * 2 永远是 0，会无限 reserve
    }
    data[size_] = v;                    // ★ 赋值，【不是】出生！
                                        //   这个格子在 reserve 的 new T[n] 那一刻就出生了。
                                        //   v 是左值 → 编译器挑 operator=(const T&) → 深拷贝
    size_++;                            // 住户 +1

    printf("[push_back] const T&\n");
    // ↑ 日志放末尾：这次没问题，但只要函数中途崩了，这行就永远打不出来。
    //   排查崩溃的日志要放【开头】。
}

// ================================================================
//  【放一个右值进去】 临时对象 / std::move(x) → 走这里
//  和左值版唯一的区别：最后那一行。
// ================================================================
template <typename T>
void MyVector<T>::push_back(T&& v) {
    if (size_ + 1 > cap_) {
        if (cap_ != 0)
            reserve(cap_ * 2);
        else
            reserve(size_ + 1);
    }
    data[size_] = std::move(v);         // ★ 唯一不同的一行。
                                        //   v 的【类型】是 T&&，但它【有名字】，
                                        //   有名字 = 左值 → 不写 move 就会走拷贝赋值。
                                        //   std::move(v) 一个字节内存都没动，
                                        //   它只是把编译器眼里的"左值"改写成"右值"。
    size_++;
    printf("[push_back] T&&\n");

}

// ================================================================
//  【出生】拷贝构造
//  跟 MyString 的拷贝构造同一个套路：有 new，自己另盖一栋楼。
//
//  和 MyString 唯一的差别：那里只有 len 一份状态，这里有 size_ 和 cap_ 两份，
//  两份都得填。
// ================================================================
template <typename T>
MyVector<T>::MyVector(const MyVector& o) {
    size_ = o.size_;                    // 抄住户数
    cap_ = o.size_;                     // ★ 新房子按【内容】造，不按 o.cap_ 造。
                                        //   若抄 o.cap_：o 是 size_=3, cap_=1024 时，
                                        //   会白造 1021 个对象（T=MyString 就是 1021 次 new char[]）
                                        //   而且 cap_ 是别人家的实现细节，不该被传染过来
    data = new T[cap_];                 // ★ 出生 cap_ 次（= size_ 次）

    for (size_t i = 0;i < size_;i++) {
        data[i] = o.data[i];            // ★ 赋值，不是出生
    }
    printf("[copy ctor] this=%p, data=%p, from=%p\n",
           (void*)this, (void*)data, (void*)o.data);
}

// ================================================================
//  【出生】移动构造 —— 整栋楼端走
//  和拷贝构造的关键差别：这里【没有 new】。
// ================================================================
template <typename T>
MyVector<T>::MyVector(MyVector&& o) noexcept {
    data = o.data;                      // 直接拿 o 的门牌号
    size_ = o.size_;
    cap_ = o.cap_;                      // ★ 这里抄 cap_ 是对的！
                                        //   移动 = 整栋楼端走，房间数当然一起端走。
                                        //   跟拷贝构造正好相反（那里是 cap_ = o.size_）
    o.data = nullptr;                   // ★ 把 o 掏空 —— 三个成员一个都不能漏
    o.size_ = 0;                        //   漏了 size_ 或 data → 下次 delete[]/push_back 立刻炸
    o.cap_ = 0;                         //   漏了 cap_ → 可能很久都不出事，直到有人
                                        //   对那个被搬空的壳做一次 push_back：
                                        //   size_=0, cap_=4 会让它跳过扩容，直接写 nullptr
    printf("[move ctor] this=%p, data=%p, from=%p\n",
           (void*)this, (void*)data, (void*)o.data);
}

// ================================================================
//  【赋值】拷贝赋值
//  和拷贝构造的差别就一条：我【原来已经有楼了】，所以要先拆掉自己的。
// ================================================================
template <typename T>
MyVector<T>& MyVector<T>::operator=(const MyVector& o) {
    printf("[copy assign] this=%p, old_data=%p, from=%p\n",
           (void*)this, (void*)data, (void*)o.data);
    if (this == &o) return *this;       // 自赋值守卫：不守的话下一行会先把自己的楼拆了，
                                        // 再去读 o.data —— 而 o 就是自己
    delete[] data;                      // ★ 先还掉自己原来那栋楼

    size_ = o.size_;
    cap_ = o.size_;                     // 同上：按内容造
    data = new T[cap_];                 // 再盖一栋新的

    for (size_t i = 0;i < size_;i++) {
        data[i] = o.data[i];
    }
    return *this;                       // 返回引用，支持 a = b = c 连写
}

// ================================================================
//  【赋值】移动赋值
//  和移动构造的差别同样只有一条：先拆掉自己的旧楼。
// ================================================================
template <typename T>
MyVector<T>& MyVector<T>::operator=(MyVector&& o) noexcept {
    printf("[move assign] BEFORE: this=%p, data=%p, o.data=%p\n",
           (void*)this, (void*)data, (void*)o.data);
    if (this == &o) return *this;       // 自赋值守卫。
                                        // 这里守的是 a = std::move(a);
                                        // 不守的话：下一行先 delete[] 了自己的楼，
                                        // 然后 data = o.data 拿到的就是刚被自己拆掉的门牌号
    delete[] data;                      // ★ 先拆自己的

    data = o.data;                      // 再偷 o 的
    size_ = o.size_;
    cap_ = o.cap_;
    o.data = nullptr;                   // 掏空 o，三个成员一起
    o.size_ = 0;
    o.cap_ = 0;
    printf("[move assign] AFTER:  this=%p, data=%p, o.data=%p\n",
           (void*)this, (void*)data, (void*)o.data);
    return *this;
}