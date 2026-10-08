# 编译速查卡

> 忘了怎么编译的时候，就看这一页。**别翻其它文件。**

---

## 我要把程序跑起来

**第 1 步** —— 打开一个配好编译器的命令行：

```bat
cd C:\Users\Admin\Desktop\GPU
.\devshell.bat code\02-my-first-build
```

> （把 `02-my-first-build` 换成你要编译的文件夹名）

**第 2 步** —— 在弹出的新窗口里，敲这两行：

```bat
cl /nologo /EHsc /utf-8 main.cpp math_utils.cpp /Fe:app.exe
.\app.exe
```

第一行编译，第二行运行。

**看到 `5` 和 `120` 就是成功了。**

---

## 就这么简单

编译一个由多个 `.cpp` 组成的程序，**一行命令**：

```
cl /nologo /EHsc /utf-8 <所有.cpp文件，空格隔开> /Fe:<输出名>.exe
```

然后运行：

```
.\<输出名>.exe
```

---

## 参数是什么意思

| 参数 | 作用 |
|---|---|
| `cl` | 调用编译器 |
| `/nologo` | 别打印版权信息，屏幕干净点 |
| `/EHsc` | 启用标准 C++ 异常处理（写 C++ 基本都要加） |
| `/utf-8` | 源码按 UTF-8 解析，**不加的话中文注释会乱码** |
| `/Fe:app.exe` | 输出的可执行文件名 |
| `.\app.exe` | 运行当前目录下的程序 |

> ⚠️ `.\` 不能省。Windows 命令行默认不搜索当前目录。

---

## 分成两步做（第 2 步的实验要用）

```bat
cl /nologo /EHsc /utf-8 /c main.cpp          :: 只编译，产出 main.obj
cl /nologo /EHsc /utf-8 /c math_utils.cpp    :: 只编译，产出 math_utils.obj
link /nologo main.obj math_utils.obj /OUT:app.exe   :: 链接
.\app.exe
```

**加 `/c` 就只编译不链接。** 这是理解"编译错误 vs 链接错误"的关键。

---

## 看中间产物（第 2 步的实验要用）

| 命令 | 产出 | 是什么 |
|---|---|---|
| `cl /P main.cpp` | `main.i` | 预处理结果（`#include` 被展开） |
| `cl /FA /c main.cpp` | `main.asm` | 汇编代码 |
| `cl /c main.cpp` | `main.obj` | 目标文件（机器码 + 符号表） |
| `dumpbin /symbols main.obj` | 屏幕 | 看目标文件里的符号 |

---

## 报错时怎么办

| 错误代码 | 出现在 | 含义 |
|---|---|---|
| `C` 开头（如 `C2065`） | **编译阶段** | 单个文件语法/语义错误 |
| `LNK` 开头（如 `LNK2019`） | **链接阶段** | 多个文件对不上 |

**看到 `C` → 那个文件自己有问题。**
**看到 `LNK` → 文件之间对不上，尤其是"声明了但没实现"。**

---

## 忘光了怎么办

```bat
cl /?           :: 编译器帮助
link /?         :: 链接器帮助
dumpbin /?      :: 工具帮助
```

或者直接在 `devshell.bat` 里敲，它会提示你可以试哪些命令。

---

# 附：GCC 和 MSVC 对照表

> 你以前用过 `gcc -o app main.c`，现在换 MSVC 觉得流程变了。
> **其实流程一模一样**，变的是你这次**故意把一条命令拆成了四步**。

## 四步对照

| 步骤 | GCC | MSVC | 产出 |
|---|---|---|---|
| 预处理 | `gcc -E main.c -o main.i` | `cl /P main.cpp` | `.i` |
| 编译成汇编 | `gcc -S main.c` | `cl /FA /c main.cpp` | `.s` / `.asm` |
| 汇编成目标文件 | `gcc -c main.c` | `cl /c main.cpp` | `.o` / `.obj` |
| 链接 | `gcc main.o x.o -o app` | `link main.obj x.obj /OUT:app.exe` | 可执行文件 |
| **一条命令全做完** | `gcc -o app main.c x.c` | `cl main.cpp x.cpp /Fe:app.exe` | 可执行文件 |

**两边都是"能拆能合"。** 你以前用的 `gcc -o` 就是最后那行 —— 一条命令内部自动调了四次工具。
现在手动拆开，是为了**看见**中间发生了什么。

## 真正的差异（这些才是工具链造成的）

| 项目 | GCC/Linux | MSVC/Windows |
|---|---|---|
| 选项前缀 | `-`（`-o`） | `/`（`/Fe:`） |
| 目标文件 | `.o` | `.obj` |
| 汇编语法 | AT&T（`movl %eax, %ebx`） | Intel（`mov ebx, eax`） |
| 静态库 | `.a` | `.lib` |
| 动态库 | `.so` | `.dll` + 导入库 `.lib` |
| 链接器 | `ld`（gcc 会自动调） | `link` |
| 异常处理 | 默认开启 | 必须加 `/EHsc` |
| 名字修饰 | `_Z3addii` | `?add@@YAHHH@Z` |

## ⚠️ 一个关键的坑

**两个编译器的"名字修饰"（name mangling）规则完全不同。**

同一个 `int add(int, int)`：
- GCC 编出来是 `_Z3addii`
- MSVC 编出来是 `?add@@YAHHH@Z`

所以 **GCC 编的 `.o` 和 MSVC 编的 `.obj` 绝对不能混着链接** ——
链接器根本认不出是同一个函数。这就是为什么一个项目要么全用 GCC，要么全用 MSVC。

## `.lib` 有两种，长得一模一样

MSVC 的 `.lib` 后缀是共用的，**静态库**和**导入库**都叫 `.lib`。分辨方法：

```bat
lib /list 某个.lib
```

| 输出 | 它是 |
|---|---|
| `.obj` 文件名列表 | **静态库** —— 一包目标文件 |
| `.dll` 文件名 | **导入库** —— 一张写给某个 dll 的欠条清单 |

> 看大小**分不出来**。实测：静态库 1354 字节，导入库 1916 字节，同一量级。
