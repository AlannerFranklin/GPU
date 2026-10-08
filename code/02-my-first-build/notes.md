1.为了方便管理
2.不加会出现同名的情况？但是我不太懂怎么加这个include guard
3.因为要调用申明的函数，故意写成int add(int a)`会报错找不到函数。
4.会爆栈
注意 `#include <cstdio>` 用的是尖括号，`#include "math_utils.h"` 用的是引号。
> **顺手查一下这两种写法有什么区别**（这是面试高频题）。
这两种写法我也不知道区别是什么，我只知道调用库得<>，调用自定义文件是引号



第二轮答案
1.我忘记怎么让c++程序编译起来了
2.我改用printf输出了
3.应该是多个文件都有同名函数的时候必须有，我查过include guard，但是还是不明白怎么加
4.除了方便管理，应该还是申明的问题？
5.这里用无符号数吗？但我不知道咋写
6.三个实验是什么？


另外我知道用补码，但是我不太知道怎么写补码计算这个


C:\Users\Admin\Desktop\GPU\code\02-my-first-build>link /nologo main.obj math_utils.obj /OUT:app.exe
main.obj : error LNK2019: 无法解析的外部符号 "int __cdecl add(int,int)" (?add@@YAHHH@Z)，函数 main 中引用了该符号
  已定义且可能匹配的符号上的提示:
    "int __cdecl add(int)" (?add@@YAHH@Z)
app.exe : fatal error LNK1120: 1 个无法解析的外部命令

第三轮答案

C:\Users\Admin\Desktop\GPU\code\02-my-first-build>cl /P main.cpp
Microsoft (R) C/C++ Optimizing Compiler Version 19.51.36257 for x64
版权所有(C) Microsoft Corporation。保留所有权利。

main.cpp

C:\Users\Admin\Desktop\GPU\code\02-my-first-build>cl /FA /c main.cpp
Microsoft (R) C/C++ Optimizing Compiler Version 19.51.36257 for x64
版权所有(C) Microsoft Corporation。保留所有权利。

main.cpp

C:\Users\Admin\Desktop\GPU\code\02-my-first-build>cl /c main.cpp
Microsoft (R) C/C++ Optimizing Compiler Version 19.51.36257 for x64
版权所有(C) Microsoft Corporation。保留所有权利。

main.cpp

C:\Users\Admin\Desktop\GPU\code\02-my-first-build>dumpbin /symbols main.obj
Microsoft (R) COFF/PE Dumper Version 14.51.36257.0
Copyright (C) Microsoft Corporation.  All rights reserved.


Dump of file main.obj

File Type: COFF OBJECT

COFF SYMBOL TABLE
000 01058DA1 ABS    notype       Static       | @comp.id
001 80010190 ABS    notype       Static       | @feat.00
002 00000000 SECT1  notype       Static       | .drectve
    Section length   62, #relocs    0, #linenums    0, checksum        0
004 00000000 SECT2  notype       Static       | .debug$S
    Section length   8C, #relocs    0, #linenums    0, checksum        0
006 00000000 SECT3  notype       Static       | .rdata
    Section length    8, #relocs    0, #linenums    0, checksum 4D29F3D8
008 00000000 SECT3  notype       Static       | $SG6076
009 00000004 SECT3  notype       Static       | $SG6077
00A 00000000 SECT4  notype       Static       | .text$mn
    Section length   40, #relocs    6, #linenums    0, checksum 3CF3E6B5
00C 00000000 SECT5  notype       Static       | .text$mn
    Section length    8, #relocs    1, #linenums    0, checksum 411950D3, selection    2 (pick any)
00E 00000000 SECT6  notype       Static       | .text$mn
    Section length   43, #relocs    2, #linenums    0, checksum 2D481083, selection    2 (pick any)
010 00000000 SECT7  notype       Static       | .text$mn
    Section length   57, #relocs    2, #linenums    0, checksum 41BAE1CE, selection    2 (pick any)
012 00000000 SECT5  notype ()    External     | __local_stdio_printf_options
013 00000000 UNDEF  notype ()    External     | __acrt_iob_func
014 00000000 UNDEF  notype ()    External     | __stdio_common_vfprintf
015 00000000 SECT6  notype ()    External     | _vfprintf_l
016 00000000 SECT7  notype ()    External     | printf
017 00000000 UNDEF  notype ()    External     | ?add@@YAHHH@Z (int __cdecl add(int,int))
018 00000000 UNDEF  notype ()    External     | ?factorial@@YAHH@Z (int __cdecl factorial(int))
019 00000000 SECT4  notype ()    External     | main
01A 00000000 SECT6  notype       Label        | $LN3
01B 00000000 SECT7  notype       Label        | $LN3
01C 00000000 SECT4  notype       Label        | $LN3
01D 00000000 SECT8  notype       Static       | .xdata
    Section length    8, #relocs    0, #linenums    0, checksum 8D3961AC, selection    5 (pick associative Section 0x6)
01F 00000000 SECT8  notype       Static       | $unwind$_vfprintf_l
020 00000000 SECT9  notype       Static       | .pdata
    Section length    C, #relocs    3, #linenums    0, checksum A712C50E, selection    5 (pick associative Section 0x6)
022 00000000 SECT9  notype       Static       | $pdata$_vfprintf_l
023 00000000 SECTA  notype       Static       | .xdata
    Section length    8, #relocs    0, #linenums    0, checksum 8D3961AC, selection    5 (pick associative Section 0x7)
025 00000000 SECTA  notype       Static       | $unwind$printf
026 00000000 SECTB  notype       Static       | .pdata
    Section length    C, #relocs    3, #linenums    0, checksum 5FE3FADF, selection    5 (pick associative Section 0x7)
028 00000000 SECTB  notype       Static       | $pdata$printf
029 00000000 SECTC  notype       Static       | .xdata
    Section length    8, #relocs    0, #linenums    0, checksum  FC539D1
02B 00000000 SECTC  notype       Static       | $unwind$main
02C 00000000 SECTD  notype       Static       | .pdata
    Section length    C, #relocs    3, #linenums    0, checksum 299DC2ED
02E 00000000 SECTD  notype       Static       | $pdata$main
02F 00000000 SECTE  notype       Static       | .bss
    Section length    8, #relocs    0, #linenums    0, checksum        0, selection    2 (pick any)
031 00000000 SECTE  notype       External     | ?_OptionsStorage@?1??__local_stdio_printf_options@@9@4_KA (unsigned __int64 `__local_stdio_printf_options'::`2'::_OptionsStorage)
032 00000000 SECTF  notype       Static       | .chks64
    Section length   78, #relocs    0, #linenums    0, checksum        0

String Table Size = 0x10D bytes

  Summary

           8 .bss
          78 .chks64
          8C .debug$S
          62 .drectve
          24 .pdata
           8 .rdata
          E2 .text$mn
          18 .xdata

C:\Users\Admin\Desktop\GPU\code\02-my-first-build>

C:\Users\Admin\Desktop\GPU\code\02-my-first-build>dumpbin /symbols main.obj
Microsoft (R) COFF/PE Dumper Version 14.51.36257.0
Copyright (C) Microsoft Corporation.  All rights reserved.


Dump of file main.obj

File Type: COFF OBJECT

COFF SYMBOL TABLE
000 01058DA1 ABS    notype       Static       | @comp.id
001 80010190 ABS    notype       Static       | @feat.00
002 00000000 SECT1  notype       Static       | .drectve
    Section length   62, #relocs    0, #linenums    0, checksum        0
004 00000000 SECT2  notype       Static       | .debug$S
    Section length   8C, #relocs    0, #linenums    0, checksum        0
006 00000000 SECT3  notype       Static       | .rdata
    Section length    8, #relocs    0, #linenums    0, checksum 4D29F3D8
008 00000000 SECT3  notype       Static       | $SG6076
009 00000004 SECT3  notype       Static       | $SG6077
00A 00000000 SECT4  notype       Static       | .text$mn
    Section length   40, #relocs    6, #linenums    0, checksum 3CF3E6B5
00C 00000000 SECT5  notype       Static       | .text$mn
    Section length    8, #relocs    1, #linenums    0, checksum 411950D3, selection    2 (pick any)
00E 00000000 SECT6  notype       Static       | .text$mn
    Section length   43, #relocs    2, #linenums    0, checksum 2D481083, selection    2 (pick any)
010 00000000 SECT7  notype       Static       | .text$mn
    Section length   57, #relocs    2, #linenums    0, checksum 41BAE1CE, selection    2 (pick any)
012 00000000 SECT5  notype ()    External     | __local_stdio_printf_options
013 00000000 UNDEF  notype ()    External     | __acrt_iob_func
014 00000000 UNDEF  notype ()    External     | __stdio_common_vfprintf
015 00000000 SECT6  notype ()    External     | _vfprintf_l
016 00000000 SECT7  notype ()    External     | printf
017 00000000 UNDEF  notype ()    External     | ?add@@YAHHH@Z (int __cdecl add(int,int))
018 00000000 UNDEF  notype ()    External     | ?factorial@@YAHH@Z (int __cdecl factorial(int))
019 00000000 SECT4  notype ()    External     | main
01A 00000000 SECT6  notype       Label        | $LN3
01B 00000000 SECT7  notype       Label        | $LN3
01C 00000000 SECT4  notype       Label        | $LN3
01D 00000000 SECT8  notype       Static       | .xdata
    Section length    8, #relocs    0, #linenums    0, checksum 8D3961AC, selection    5 (pick associative Section 0x6)
01F 00000000 SECT8  notype       Static       | $unwind$_vfprintf_l
020 00000000 SECT9  notype       Static       | .pdata
    Section length    C, #relocs    3, #linenums    0, checksum A712C50E, selection    5 (pick associative Section 0x6)
022 00000000 SECT9  notype       Static       | $pdata$_vfprintf_l
023 00000000 SECTA  notype       Static       | .xdata
    Section length    8, #relocs    0, #linenums    0, checksum 8D3961AC, selection    5 (pick associative Section 0x7)
025 00000000 SECTA  notype       Static       | $unwind$printf
026 00000000 SECTB  notype       Static       | .pdata
    Section length    C, #relocs    3, #linenums    0, checksum 5FE3FADF, selection    5 (pick associative Section 0x7)
028 00000000 SECTB  notype       Static       | $pdata$printf
029 00000000 SECTC  notype       Static       | .xdata
    Section length    8, #relocs    0, #linenums    0, checksum  FC539D1
02B 00000000 SECTC  notype       Static       | $unwind$main
02C 00000000 SECTD  notype       Static       | .pdata
    Section length    C, #relocs    3, #linenums    0, checksum 299DC2ED
02E 00000000 SECTD  notype       Static       | $pdata$main
02F 00000000 SECTE  notype       Static       | .bss
    Section length    8, #relocs    0, #linenums    0, checksum        0, selection    2 (pick any)
031 00000000 SECTE  notype       External     | ?_OptionsStorage@?1??__local_stdio_printf_options@@9@4_KA (unsigned __int64 `__local_stdio_printf_options'::`2'::_OptionsStorage)
032 00000000 SECTF  notype       Static       | .chks64
    Section length   78, #relocs    0, #linenums    0, checksum        0

String Table Size = 0x10D bytes

  Summary

           8 .bss
          78 .chks64
          8C .debug$S
          62 .drectve
          24 .pdata
           8 .rdata
          E2 .text$mn
          18 .xdata

C:\Users\Admin\Desktop\GPU\code\02-my-first-build>dumpbin /symbols math_utils.obj
Microsoft (R) COFF/PE Dumper Version 14.51.36257.0
Copyright (C) Microsoft Corporation.  All rights reserved.


Dump of file math_utils.obj

File Type: COFF OBJECT

COFF SYMBOL TABLE
000 01058DA1 ABS    notype       Static       | @comp.id
001 80010190 ABS    notype       Static       | @feat.00
002 00000000 SECT1  notype       Static       | .drectve
    Section length   2F, #relocs    0, #linenums    0, checksum        0
004 00000000 SECT2  notype       Static       | .debug$S
    Section length   94, #relocs    0, #linenums    0, checksum        0
006 00000000 SECT3  notype       Static       | .text$mn
    Section length   59, #relocs    0, #linenums    0, checksum E6B8D357
008 00000000 SECT3  notype ()    External     | ?factorial@@YAHH@Z (int __cdecl factorial(int))
009 00000050 SECT3  notype ()    External     | ?add@@YAHH@Z (int __cdecl add(int))
00A 00000000 SECT3  notype       Label        | $LN6
00B 00000000 SECT4  notype       Static       | .xdata
    Section length    8, #relocs    0, #linenums    0, checksum CA64273D
00D 00000000 SECT4  notype       Static       | $unwind$?factorial@@YAHH@Z
00E 00000000 SECT5  notype       Static       | .pdata
    Section length    C, #relocs    3, #linenums    0, checksum E537C273
010 00000000 SECT5  notype       Static       | $pdata$?factorial@@YAHH@Z
011 00000000 SECT6  notype       Static       | .chks64
    Section length   30, #relocs    0, #linenums    0, checksum        0

String Table Size = 0x59 bytes

  Summary

          30 .chks64
          94 .debug$S
          2F .drectve
           C .pdata
          59 .text$mn
           8 .xdata



C:\Users\Admin\Desktop\GPU\code\02-my-first-build>lib /OUT:math_utils.lib math_utils.obj
Microsoft (R) Library Manager Version 14.51.36257.0
Copyright (C) Microsoft Corporation.  All rights reserved.

main.obj : error LNK2019: 无法解析的外部符号 "int __cdecl add(int,int)" (?add@@YAHHH@Z)，函数 main 中引用了该符号
  已定义且可能匹配的符号上的提示:
    "int __cdecl add(int)" (?add@@YAHH@Z)
app_static.exe : fatal error LNK1120: 1 个无法解析的外部命令

app.exe 139kb
C:\Users\Admin\Desktop\GPU\code\02-my-first-build>dumpbin /exports math_utils.dll
Microsoft (R) COFF/PE Dumper Version 14.51.36257.0
Copyright (C) Microsoft Corporation.  All rights reserved.


Dump of file math_utils.dll

File Type: DLL

  Summary

        2000 .data
        1000 .fptable
        2000 .pdata
        A000 .rdata
        1000 .reloc
        E000 .text
少了很多东西

静态库和动态库？我不太懂这里

实测跑不了了，会闪退，dll应该算是依赖文件吧



C:\Users\Admin\Desktop\GPU\code\02-my-first-build>cl /nologo /EHsc /utf-8 /c main.cpp math_utils.cpp
main.cpp
math_utils.cpp
正在生成代码...

C:\Users\Admin\Desktop\GPU\code\02-my-first-build>lib /OUT:statics.lib math_utils.obj
Microsoft (R) Library Manager Version 14.51.36257.0
Copyright (C) Microsoft Corporation.  All rights reserved.


C:\Users\Admin\Desktop\GPU\code\02-my-first-build>link main.obj statics.lib /OUT:app_static.exe
Microsoft (R) Incremental Linker Version 14.51.36257.0
Copyright (C) Microsoft Corporation.  All rights reserved.


C:\Users\Admin\Desktop\GPU\code\02-my-first-build>dumpbin /dependents app_static.exe
Microsoft (R) COFF/PE Dumper Version 14.51.36257.0
Copyright (C) Microsoft Corporation.  All rights reserved.


Dump of file app_static.exe

File Type: EXECUTABLE IMAGE

  Image has the following dependencies:

    KERNEL32.dll

  Summary

        2000 .data
        1000 .fptable
        2000 .pdata
        B000 .rdata
        1000 .reloc
       16000 .text

C:\Users\Admin\Desktop\GPU\code\02-my-first-build>cl /LD math_utils.cpp /Fe:dyns.dll
Microsoft (R) C/C++ Optimizing Compiler Version 19.51.36257 for x64
版权所有(C) Microsoft Corporation。保留所有权利。

math_utils.cpp
Microsoft (R) Incremental Linker Version 14.51.36257.0
Copyright (C) Microsoft Corporation.  All rights reserved.

/dll 
/implib:dyns.lib 
/out:dyns.dll 
math_utils.obj 

都是139kb，然后还是打不开

C:\Users\Admin\Desktop\GPU\code\02-my-first-build>link main.obj dyns.lib /OUT:app_dynamic.exe
Microsoft (R) Incremental Linker Version 14.51.36257.0
Copyright (C) Microsoft Corporation.  All rights reserved.

LINK : fatal error LNK1181: 无法打开输入文件“dyns.lib”

C:\Users\Admin\Desktop\GPU\code\02-my-first-build>dumpbin /exports dyns.dll
Microsoft (R) COFF/PE Dumper Version 14.51.36257.0
Copyright (C) Microsoft Corporation.  All rights reserved.


Dump of file dyns.dll

File Type: DLL

  Summary

        2000 .data
        1000 .fptable
        2000 .pdata
        A000 .rdata
        1000 .reloc
        E000 .text


C:\Users\Admin\Desktop\GPU\code\02-my-first-build>cl /LD math_utils.cpp /Fe:dyns.dll
Microsoft (R) C/C++ Optimizing Compiler Version 19.51.36257 for x64
版权所有(C) Microsoft Corporation。保留所有权利。

math_utils.cpp
Microsoft (R) Incremental Linker Version 14.51.36257.0
Copyright (C) Microsoft Corporation.  All rights reserved.

/dll 
/implib:dyns.lib 
/out:dyns.dll 
math_utils.obj 

C:\Users\Admin\Desktop\GPU\code\02-my-first-build>del /q *.obj *.lib *.exe *.dll *.exp

C:\Users\Admin\Desktop\GPU\code\02-my-first-build>
C:\Users\Admin\Desktop\GPU\code\02-my-first-build>cl /nologo /EHsc /utf-8 /c main.cpp math_utils.cpp
main.cpp
math_utils.cpp
正在生成代码...

C:\Users\Admin\Desktop\GPU\code\02-my-first-build>lib /nologo /OUT:statics.lib math_utils.obj

C:\Users\Admin\Desktop\GPU\code\02-my-first-build>link /nologo main.obj statics.lib /OUT:app_static.exe

C:\Users\Admin\Desktop\GPU\code\02-my-first-build>.\app_static.exe
5
120

C:\Users\Admin\Desktop\GPU\code\02-my-first-build>
C:\Users\Admin\Desktop\GPU\code\02-my-first-build>cl /nologo /EHsc /utf-8 /D MATH_UTILS_BUILDING_DLL /LD math_utils.cpp /Fe:dyns.dll
math_utils.cpp
  正在创建库 dyns.lib 和对象 dyns.exp

C:\Users\Admin\Desktop\GPU\code\02-my-first-build>dumpbin /exports dyns.dll
Microsoft (R) COFF/PE Dumper Version 14.51.36257.0
Copyright (C) Microsoft Corporation.  All rights reserved.


Dump of file dyns.dll

File Type: DLL

  Section contains the following exports for dyns.dll

    00000000 characteristics
    FFFFFFFF time date stamp
        0.00 version
           1 ordinal base
           2 number of functions
           2 number of names

    ordinal hint RVA      name

          1    0 00001000 ?add@@YAHHH@Z
          2    1 00001020 ?factorial@@YAHH@Z

  Summary

        2000 .data
        1000 .fptable
        2000 .pdata
        A000 .rdata
        1000 .reloc
        E000 .text

C:\Users\Admin\Desktop\GPU\code\02-my-first-build>
C:\Users\Admin\Desktop\GPU\code\02-my-first-build>link /nologo main.obj dyns.lib /OUT:app_dynamic.exe

C:\Users\Admin\Desktop\GPU\code\02-my-first-build>.\app_dynamic.exe
5
120

C:\Users\Admin\Desktop\GPU\code\02-my-first-build>

Q21
a 有写正在创建
b 一共生成了4个文件，math_utils.obj .exp .lib .dll
c 有东西，没有乱码， 对的上
d 不知道
Q22 大小一致，因为内存对齐，文件较小，512字节就对齐了.exe中体积最大的是代码
Q23 刚刚测错了，现在重新测试，静态能跑，动态跑不了
C:\Users\Admin\Desktop\GPU\code\02-my-first-build>app_STATIC.exe
5
120

C:\Users\Admin\Desktop\GPU\code\02-my-first-build>app_DYNAMIC.exe

C:\Users\Admin\Desktop\GPU\code\02-my-first-build>.\app_static.exe
5
120

C:\Users\Admin\Desktop\GPU\code\02-my-first-build>echo %ERRORLEVEL%
0


C000 0135报错为STATUS_DLL_NOT_FOUND，找不到dll
不过Q24我还真想不通，我觉得只用静态库就好了

明白了，小工具，拷过去就能跑，用静态，就简单，不怎么需要依赖
动态库在于要不断的更新迭代，并且避免反汇编。

至于Q21d 
需要有人下单了，才能点菜送上来，如果全部导出，等于所有人都知道内部函数是什么，怎么弄的，调用之后无法进行升级
