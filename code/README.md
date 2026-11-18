# code/ · 课题索引

> 编号是**课题序号**（学习顺序），不是周次。
> 每个目录配一份 `CMakeLists.txt`，统一用根目录的 `build.bat` 编译：

```bat
build.bat <目录名>          只编译
build.bat <目录名> run      编译并运行
build.bat <目录名> clean    清掉 build/
```

> 两台机器的约定（编译器路径、`sm_89`、`/utf-8`）见 [handover/README.md](../handover/README.md)。

## 总表

| 目录 | 课题 | 内容 | 状态 |
|---|---|---|---|
| `00-hello-cpp` | 工具链 | 最小 C++ 程序 | ✅ |
| `01-hello-sycl` | SYCL | 核显 hello world | ⚪ 未跑通（环境，见下） |
| `02-my-first-build` | 编译与链接 | 静态库 + `dumpbin` 符号表 | ✅ |
| `03-memory` | 指针与内存 | `sizeof` / 对齐 / 指针内存图 | ✅ |
| `04-mystring` | 类与 RAII | `MyString` + 移动语义 | ✅ |
| `05-myvector` | 模板与 STL | `MyVector<T>` / `unique_ptr` / `shared_ptr` | ✅ |
| `06-cache` | 存储器层次 | CSAPP cache lab + 步长 + 分块转置 | ✅ |
| `07-vm` | 虚拟内存 | 缺页、TLB | ✅ |
| `08-malloc` | 动态内存 | CSAPP malloc lab | ✅ |
| `09-thread` | 并发 | 自旋锁 / SPSC 队列 / 线程池 | ✅ |
| `10-cuda` | 第一个 CUDA kernel | `vector_add`，合并 vs 散开访存 | ✅ |
| `11-shared` | shared memory | `reverse.cu` | ✅ |
| [`12-reduction`](12-reduction/) | 归约 | **七个版本** v0-v6，见目录内 README | 🔶 代码完成，数字待补 |
| `13-transpose` | 矩阵转置 | naive + shared 版 | ✅ |
| [`14-matmul`](14-matmul/) | 矩阵乘法 | **四个版本** + Roofline 瓶颈分析，内容最全 | ✅ |
| [`15-scan`](15-scan/) | 前缀和 | Blelloch / Hillis-Steele，见目录内 README | 🔶 代码完成，数字待补 |

图例：✅ 有性能数字或已完成 ｜ 🔶 代码写完但缺实测数字 ｜ ⚪ 受阻

## 有独立 README 的目录

只有内容足够多的三个目录单独写了 README：

- **[`14-matmul/README.md`](14-matmul/README.md)** —— 四个版本、资源用量表、跑法、ncu 口径
- **[`14-matmul/bottleneck.md`](14-matmul/bottleneck.md)** —— 四版 Roofline 瓶颈分析（第 12 周交付物）
- **[`12-reduction/README.md`](12-reduction/README.md)** —— 七个归约版本的谱系
- **[`15-scan/README.md`](15-scan/README.md)** —— 前缀和的三个版本

其余目录的知识点记在各自的 `notes.md` 里。

## `01-hello-sycl` 为什么是 ⚪

第 9 周的计划是在公司机的 UHD 730 上跑 SYCL，实际不可用。
`main.cpp` 和 `CMakeLists.txt` 都在，缺的是能跑 ISYCL 的环境。

**白天 AI 建议正式放弃这一项**（门 A 不需要 SYCL，且 shared memory / barrier / work-group
这些概念已经在 CUDA 里全部实践过），但需要本人拍板。详见
[handover/2026-11-18-weekend-debts.md](../handover/2026-11-18-weekend-debts.md) 第七节。

## 目录约定

新开一个课题时：

1. 复制最近的 `CMakeLists.txt`，只改 `project()` 名和 `add_executable` 列表
2. 源文件放同目录，头文件用 `#include "xxx.h"`（相对当前目录）
3. 编译标准统一：C++17 / `sm_89` / `/utf-8`，**不要改**
4. 不要在 `code/` 下放构建产物——`build.bat` 会生成 `build/`，已被 `.gitignore` 挡掉
