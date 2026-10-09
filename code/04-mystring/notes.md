
C:\Users\Admin\Desktop\GPU\code\04-mystring>.\mystring.exe
=== 1. 普通构造 ===
[gouzao]000000AE7ABFF888 0000025ECDD7AFB0 hello

=== 2. 拷贝构造 ===
[copy ctor] this=000000AE7ABFF898, data=0000025ECDD74F40, from=0000025ECDD7AFB0, s="hello"

=== 3. 拷贝赋值 ===
[gouzao]000000AE7ABFF878 0000025ECDD74F60 world
[copy assign] this=000000AE7ABFF878, old_data=0000025ECDD74F60, from=0000025ECDD7AFB0
[copy assign] this=000000AE7ABFF878, data=0000025ECDD74F60, from=0000025ECDD7AFB0, s="hello"

=== 4. 自赋值 ===
[copy assign] this=000000AE7ABFF878, old_data=0000025ECDD74F60, from=0000025ECDD74F60
[copy assign] self assignment, skip

=== 5. 地址对比 ===
0000025ECDD7AFB0 0000025ECDD74F40 0000025ECDD74F60 

=== main 结束，开始析构 ===
[xigou]000000AE7ABFF878 0000025ECDD74F60 hello
[xigou]000000AE7ABFF898 0000025ECDD74F40 hello
[xigou]000000AE7ABFF888 0000025ECDD7AFB0 hello


Q1 
编译过了，编译器自动生成了一个构造函数，
MyString(const MyString& o)

通过拷贝构造创建了b
MyString(const MyString& o) {
    data = o.data;   // 只搬指针那 8 个字节
    len  = o.len;    // 只搬长度
}
里面没有 new，没有循环，没有 '\0'。 它搬的是指针的值（地址），不是指针指向的内容。

后果：两个对象的 data 指向同一块堆内存。谁先析构谁 delete[]，另一个接着 delete[]
Q2
析构打印了两遍，构造打印了一遍
每次打印的data地址是一样的
Q3
=== main 结束，开始析构 ===
[xigou]00000039694FF6F8 000001B45944AFB0 hello
[xigou]00000039694FF718 000001B45944AFB0 `ODY�
崩了，0xC0000374
Q4
new调用了1次，a构造一次
delete是2次，b删除1次，a删除一次
因为编译器生成的拷贝构造只搬了指针，b.data 和 a.data 指向同一块堆内存。b 析构时 delete[] 了一次，a 析构时又对同一个地址 delete[] 了一次。同一块内存释放两次，堆管理器就崩了。
Q5
拷贝构造第一件事是申请空间，拷贝赋值第一件事是判断是否一致，一致就不赋值，不一致就删除原本内容，重新申请空间。
拷贝构造this里面没东西，拷贝赋值this有东西，所以需要delete

Q6
内存泄漏
Q7
=== 4. 自赋值 ===
[copy assign] this=000000EA6A4FFB28, old_data=000002BE659F4F60, from=000002BE659F4F60
[copy assign] this=000000EA6A4FFB28, data=000002BE659F4F60, from=000002BE659F4F60, s="P�e�"
会出现内容损坏的问题
堆会复用刚释放的脏内存

if (this == &o) {
    printf("[copy assign] self assignment, skip\n");
    return *this;
}
用这个防守，如果发现是自赋值，什么都不做直接返回
Q8 
拷贝构造，析构，拷贝赋值，他们在处理内存的所有权
拷贝构造：决定新对象是"借"这块内存（浅）还是"自己造一块"（深）。

析构：决定谁来"还"这块内存。

拷贝赋值：决定旧内存怎么处理、新内存怎么来。

Q9
拷贝构造更慢，需要分配内存，然后拷贝字符，而移动构造只需要改一下指针，置空原指针，拷贝是"跟字符串一样长"，移动是"跟字符串多长没关系"

Q10
被移动后的对象处于还能安全地析构状态，data指向nullptr，无法读

Q11
o 的析构函数 delete 了 o.data 指着的那块堆内存。this->data 里装的门牌号变成野指针。它析构时又拿着同一个门牌号 delete 一次 —— 和 Q1 一样，二次释放，0xC0000374。
Q12
1次拷贝，3138次移动
临时对象1000，vector扩充2137
C:\Users\Admin\Desktop\GPU\code\04-mystring>findstr "count" out.txt
move_ctor_count=1
move_assign_count=1
copy_ctor_count=1
move_ctor_count=3138
move_assign_count=1
copy_ctor_count=1


C:\Users\Admin\Desktop\GPU\code\04-mystring>findstr "count" out.txt
move_ctor_count=1
move_assign_count=1
copy_ctor_count=1
move_ctor_count=1001
move_assign_count=1
copy_ctor_count=1

会变成移动拷贝1000次
3138 和 1001 都含第 6 步那 1 次

C:\Users\Admin\Desktop\GPU\code\04-mystring>.\mystring.exe
=== 1. 普通构造 ===
[gouzao]000000DBFA8FFDC8 000001EAD6F7AFB0 hello

=== 2. 拷贝构造 ===
[copy ctor] this=000000DBFA8FFDF8, data=000001EAD6F74F40, from=000001EAD6F7AFB0, s="hello"

=== 3. 拷贝赋值 ===
[gouzao]000000DBFA8FFDB8 000001EAD6F74F60 world
[copy assign] this=000000DBFA8FFDB8, old_data=000001EAD6F74F60, from=000001EAD6F7AFB0
[copy assign] this=000000DBFA8FFDB8, data=000001EAD6F74F60, from=000001EAD6F7AFB0, s="hello"

=== 4. 自赋值 ===
[copy assign] this=000000DBFA8FFDB8, old_data=000001EAD6F74F60, from=000001EAD6F74F60
[copy assign] self assignment, skip

=== 5. 地址对比 ===
000001EAD6F7AFB0 000001EAD6F74F40 000001EAD6F74F60 

=== 6. 移动构造：push_back 临时对象 ===
[gouzao]000000DBFA8FFDD8 000001EAD6F74F80 hello
[move ctor] this=000001EAD6F79E30, data=000001EAD6F74F80, from(o.data after null)=0000000000000000, s="hello"
[xigou]000000DBFA8FFDD8 0000000000000000 (null)

=== 7. 移动赋值：v[0] = 临时对象 ===
[gouzao]000000DBFA8FFDE8 000001EAD6F74FA0 world
[move assign] BEFORE: this=000001EAD6F79E30, this->data=000001EAD6F74F80, o.data=000001EAD6F74FA0, o.len=5
[move assign] AFTER:  this=000001EAD6F79E30, this->data=000001EAD6F74FA0, o.data=0000000000000000, o.len=0, s="world"
[xigou]000000DBFA8FFDE8 0000000000000000 (null)

move_ctor_count=1
move_assign_count=1
copy_ctor_count=1
[xigou]000001EAD6F79E30 000001EAD6F74FA0 world

=== main 结束，开始析构 ===
[xigou]000000DBFA8FFDB8 000001EAD6F74F60 hello
[xigou]000000DBFA8FFDF8 000001EAD6F74F40 hello
[xigou]000000DBFA8FFDC8 000001EAD6F7AFB0 hello

C:\Users\Admin\Desktop\GPU\code\04-mystring>


Q13
std::move的功能就是一个强制类型转换，把x伪装为右值引用，为后续移动做铺垫。
Q14
std::move没有修改任何内存，所以x还在，我不能用是因为后续的移动构造函数导致的
Q15
MyString移动前需要拷贝，MyString&普通引用无法使用临时对象，MyString&&右值引用才能支持移动
Q16
对象是const时，类没有移动构造函数时，移动构造函数不是noexcept

C:\Users\Admin\Desktop\GPU\code\04-mystring>cl /nologo /utf-8 /EHsc /W4 mystring.cpp /Fe:mystring.exe
mystring.cpp
mystring.cpp(95): warning C4189: “r”: 局部变量已初始化但不引用

C:\Users\Admin\Desktop\GPU\code\04-mystring>.\mystring.exe
[gouzao]00000017473BFAF8 00000134166BAFB0 hello
before: x.data=00000134166BAFB0
after : x.data=00000134166BAFB0

地址没变，一样的，但会有警告


拷贝不够用。有一类资源天生独占（串口、文件句柄、锁）。"两个对象同时拥有它"不是慢，是错的 —— 所以这类类的拷贝构造只能 = delete。可删掉拷贝之后它还得能从函数返回、能进 vector —— 移动就是干这个的：它不复制资源，它把所有权交出去。

MyString进行拷贝构造的时候，用的不是同一块内存，所以可以写

