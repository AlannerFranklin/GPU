1.全局变量离得远
2.堆的地址更大，大了很多
3.后声明的大，栈往小的方向长

C:\Users\Admin\Desktop\GPU\code\03-memory>.\mem.exe
00007FF7153A2B70
00000048E52FFCD0
0000023EEC4D7CF0
00007FF715397314
00000048E52FFCB0
00000048E52FFC80
00000048E52FFC50
00000048E52FFC20
******************
00000048E52FFCE0
00000048E52FFCE4
00000048E52FFCE8
00000048E52FFCEC
00000048E52FFCF0

4.离得近，他们应该是共用一块内存空间。


C:\Users\Admin\Desktop\GPU\code\03-memory>.\ptr.exe
000000053190FDA8 000000053190FDAC
00007FF64CDC7310 00007FF64CDC7311
3 3
000001FC8CE4D5C0  000001FC8CE4D5C4
000001FC8CE4D5E0  000001FC8CE4D5E1
分别是差一个int，4字节和一个char 一字节
这块我学过，我科班出生的，我明白内存就是地址，我是学计算机的，new出来在同一个地址空间
根据不同的类型长度，算下一个量就加上多少
arr[2]和2[arr]是等价的。


C:\Users\Admin\Desktop\GPU\code\03-memory>.\memcpy.exe                                                                                                                                                                    
测试1 int 数组:          
  返回值 == dst ? 是      
  内容一致 ? 是              
  dst = 1 2 3 4 5                  
测试2 结构体:      
  dst.a = 42
  dst.b = 3.140000
  dst.c = hello
  按字节一致 ? 是
测试3 字符串:
  dst = "Hello, CUDA!"
  长度一致 ? 是
测试4 部分拷贝:
  dst = 10 20 0 0
  期望 10 20 0 0

测试5 重叠拷贝（前向覆盖）:
  buf = ababababab
  提示：memcpy 不保证重叠正确，需要 memmove 才安全

测试6 n=0:
  dst 应保持 456，实际 = 456

我使用了const char来搬运，因为只有他是1字节的，最小单位，一次加一个，才能全搬到。
dst和src重叠的时候会逐地址不断覆盖掉


memcpy 的文档上写着"两块内存不许重叠"。这不是限制，这是一份承诺。

拿到这份承诺，实现者可以做两件 memmove 不敢做的事：

① 不用判断方向
memmove 必须先比较 dst 和 src 谁前谁后（也就是我刚讲的 Q10 那套逻辑），才决定从前往后还是从后往前。多一个分支、多一次比较。

② 可以用更大的搬运单位
memcpy 可以一次搬 16 字节（SSE 指令）或 32 字节（AVX 指令），而不是逐字节搬。理论上 16~32 倍差距。


21290 21291 21292 21293 21294 21295 21296 21297 21298 21299 21300 21301 21302 21303 21304 21305 21306 21307 
C:\Users\Admin\Desktop\GPU\code\03-memory>b.exe

C:\Users\Admin\Desktop\GPU\code\03-memory>c.exe
ok 1

测试完毕，跑到21307停止，函数的局部大数组直接失败，堆new出来成功。
实验A运行时候报错，编译的时候有警告
new出来的忘了delete会泄露。

① c.cpp 里怎么让编译器"不敢"删掉 big？
有可观测的行为

② 你的 main 第 17 行 int n = 0; 声明了就没用过。 为什么编译器一句 warning 都没给你？（这次编译只有 C4717 一条）
C4189 局部变量已初始化但不引用，只在 /W4 才报，默认的 /W1 不报。

C:\Users\Admin\Desktop\GPU\code\03-memory>certutil -error -1073741571
0xc00000fd (NT: 0xc00000fd STATUS_STACK_OVERFLOW) -- 3221225725 (-1073741571)
Error message text: 无法创建新的堆栈防护页面。
CertUtil: -error command completed successfully.


栈大小	实测死在	相对 1 MB
1 MB	≥ 20480 层	1.00×
4 MB	≥ 86016 层	4.20×
16 MB	≥ 348160 层	17.0×

栈的大小是链接时写死在 exe 里的一个数字（MSVC 的链接器默认 1 MB）。
堆的大小是程序跑起来以后，运行时向操作系统一点一点要的。

区别不在"大小"，在什么时候定：

栈	堆
大小谁定	链接器，写进 exe 文件头	操作系统，运行时按需给
什么时候定	编译/链接时	程序运行时
改它要干什么	重新链接	不用改程序，它自己会长

MSVC 的链接器默认上规定了栈的大小是1MB
这个值能改，规定这么小的原因是多线程，每个线程都可以申请栈，如果栈太大，32位进程地址空间只有2GB，会崩溃。

int** p是一个指针指向一个int* p，指针的指针，sizeof=8
int (*p)[5];这是指向5个int的数组的指针，sizeof=8
int *p[5];这是定义了5个int*的数组，sizeof=40

① int   a[5]      = 5*4=20
② int*  b[5]      = 8*5=40
③ int (*c)[5]     = 8 
④ int** d[5]      = 5*8=40 
  = 8*5=40 
⑥ int  *f         = 8

int (*c)[5]，一个指针，长度为5，指针大小为8，int** d[5]，5个指针的指针，5*8=40，