析构打印了两遍，构造打印了一遍
地址是一样的
因为后续我们又引用了一次，而这一块内存
MyString a("hello");
MyString b = a
又会触发一次析构，
等于a声明，b声明，然后b先死，a再死，构造只构造了一次，析构了两次。所以删了两次内存。

拷贝构造第一件事是申请空间，拷贝赋值第一件事是判断是否一致，一致就不赋值，不一致就删除原本内容，重新申请空间。

如果忘了，再遇到重叠的情况则会不断迭代复制
自己给自己赋值会出现野指针
三法则是什么？

C:\Users\Admin\Desktop\GPU\code\04-mystring>.\mystring.exe
=== 1. 普通构造 ===
[gouzao]00000047D817F718 000001BDF3C58FB0 hello

=== 2. 拷贝构造 ===
[copy ctor] this=00000047D817F728, data=000001BDF3C4D720, from=000001BDF3C58FB0, s="hello"

=== 3. 拷贝赋值 ===
[gouzao]00000047D817F708 000001BDF3C4D740 world
[copy assign] this=00000047D817F708, old_data=000001BDF3C4D740, from=000001BDF3C58FB0
[copy ctor] this=00000047D817F708, data=000001BDF3C4D740, from=000001BDF3C58FB0, s="hello"

=== 4. 自赋值 ===
[copy assign] this=00000047D817F708, old_data=000001BDF3C4D740, from=000001BDF3C4D740
[copy assign] self assignment, skip

=== 5. operator<< ===
00000047D817F718 00000047D817F728 00000047D817F708 

=== main 结束，开始析构 ===
[xigou]00000047D817F708 000001BDF3C4D740 hello
[xigou]00000047D817F728 000001BDF3C4D720 hello
[xigou]00000047D817F718 000001BDF3C58FB0 hello