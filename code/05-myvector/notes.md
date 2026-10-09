C:\Users\Admin\Desktop\GPU\code\05-myvector>cl /nologo /utf-8 /W4 t1.cpp
t1.cpp
t1.cpp(8): warning C4477: “printf”: 格式字符串“%d”需要类型“int”的参数，但可变参数 1 拥有了类型“T”
        with
        [
            T=double
        ]

C:\Users\Admin\Desktop\GPU\code\05-myvector>cl /nologo /utf-8 /W4 t1.cpp
t1.cpp
t1.cpp(7): error C2672: “mymax”: 未找到匹配的重载函数
t1.cpp(4): note: 可能是“T mymax(T,T)”
t1.cpp(7): note: “T mymax(T,T)”: 模板 参数“T”不明确
t1.cpp(7): note: 可能是“double”
t1.cpp(7): note: 或    “int”
t1.cpp(7): note: “T mymax(T,T)”: 无法从“double”推导出“T”的 模板 参数
t1.cpp(7): warning C4473: “printf”: 没有为格式字符串传递足够的参数
t1.cpp(7): note: 占位符和其参数预计 1 可变参数，但提供的却是 0 参数
t1.cpp(7): note: 缺失的可变参数 1 为格式字符串“%d”所需
t1.cpp(8): warning C4477: “printf”: 格式字符串“%d”需要类型“int”的参数，但可变参数 1 拥有了类型“T”
        with
        [
            T=double
        ]

模板的声明和定义为什么不能拆成两个文件
因为，拆成两个文件，定义和声明可能会出现类型不匹配的问题，导致报错



C:\Users\Admin\Desktop\GPU\code\05-myvector>cl /nologo /utf-8 /EHsc /W4 main.cpp /Fe:myvector.exe
main.cpp
main.obj : error LNK2019: 无法解析的外部符号 "public: __cdecl MyVector<int>::MyVector<int>(void)" (??0?$MyVector@H@@QEAA@XZ)，函数 main 中引用了该符号
main.obj : error LNK2019: 无法解析的外部符号 "public: __cdecl MyVector<int>::~MyVector<int>(void)" (??1?$MyVector@H@@QEAA@XZ)，函数 main 中引用了该符号
main.obj : error LNK2019: 无法解析的外部符号 "public: unsigned __int64 __cdecl MyVector<int>::size(void)const " (?size@?$MyVector@H@@QEBA_KXZ)，函数 main 中引用了该符号
main.obj : error LNK2019: 无法解析的外部符号 "public: unsigned __int64 __cdecl MyVector<int>::capacity(void)const " (?capacity@?$MyVector@H@@QEBA_KXZ)，函数 main 中引用了该符号
myvector.exe : fatal error LNK1120: 4 个无法解析的外部命令

C:\Users\Admin\Desktop\GPU\code\05-myvector>cl /nologo -utf-8 /EHsc /W4 main.cpp /Fe:myvector.exe
main.cpp
C:\Users\Admin\Desktop\GPU\code\05-myvector\myvector.h(38): error C2244: “MyVector<T>::size”: 无法将函数定义与现有的声明匹配
C:\Users\Admin\Desktop\GPU\code\05-myvector\myvector.h(38): note: 参见“MyVector<T>::size”的声明
C:\Users\Admin\Desktop\GPU\code\05-myvector\myvector.h(38): note: 定义
C:\Users\Admin\Desktop\GPU\code\05-myvector\myvector.h(38): note: 'int MyVector<T>::size(void) const'
C:\Users\Admin\Desktop\GPU\code\05-myvector\myvector.h(38): note: 现有声明
C:\Users\Admin\Desktop\GPU\code\05-myvector\myvector.h(8): note: 'size_t MyVector<T>::size(void) const'
C:\Users\Admin\Desktop\GPU\code\05-myvector\myvector.h(43): error C2244: “MyVector<T>::capacity”: 无法将函数定义与现有的声明匹配
C:\Users\Admin\Desktop\GPU\code\05-myvector\myvector.h(43): note: 参见“MyVector<T>::capacity”的声明
C:\Users\Admin\Desktop\GPU\code\05-myvector\myvector.h(43): note: 定义
C:\Users\Admin\Desktop\GPU\code\05-myvector\myvector.h(43): note: 'int MyVector<T>::capacity(void) const'
C:\Users\Admin\Desktop\GPU\code\05-myvector\myvector.h(43): note: 现有声明
C:\Users\Admin\Desktop\GPU\code\05-myvector\myvector.h(9): note: 'size_t MyVector<T>::capacity(void) const'


C:\Users\Admin\Desktop\GPU\code\05-myvector>cl /nologo -utf-8 /EHsc /W4 main.cpp /Fe:myvector.exe
main.cpp

C:\Users\Admin\Desktop\GPU\code\05-myvector>.\myvector.exe
--- 传左值 ---
[push_back] const T&
--- 传临时对象 ---
[push_back] T&&
--- 传 std::move(a) ---
[push_back] T&&

C:\Users\Admin\Desktop\GPU\code\05-myvector>echo 退出码=%errorlevel%
退出码=0

不用std::move会变成拷贝赋值，我们需要是移动赋值，让编译器自己找类型

Q1
不能，必须要知道当前的大小和整体的容量，reserve100后，cap是100，size还是原来大小

Q2
扩容先判断，如果小，就直接返回，如果更大，就进行扩容，申请新的内存空间，移动赋值size_过去，然后把旧的删掉，先delete会丢失数据，这就叫use-after-free，然后把临时的data1赋给data，data的cap标为最新的大小n。

