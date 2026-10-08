# 环境搭建手册

> 本机实测于 2026-10-08。已验证可用的部分标 ✅，需要你动手的部分标 🔧。

---

## 1. 本机现状（我已经验证过的）

| 组件 | 状态 | 路径 |
|---|---|---|
| Visual Studio Community 2026 | ✅ 已装 | `C:\Program Files\Microsoft Visual Studio\18\Community` |
| MSVC 编译器 | ✅ 14.44 + 14.51 双版本 | `...\VC\Tools\MSVC\14.51.36231\bin\Hostx64\x64\cl.exe` |
| CMake | ✅ 已随 VS 安装 | `...\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe` |
| Ninja | ✅ 已随 VS 安装 | `...\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe` |
| Windows SDK | ✅ 10.0.26100 / 10.0.28000 | `C:\Program Files (x86)\Windows Kits\10` |
| Python | ✅ 3.14.6 | `AppData\Local\Python` |
| Git | ✅ | `/ucrt64/bin/git` |
| VS Code | ✅ | `AppData\Local\Programs\Microsoft VS Code` |

**结论：C++ 工具链已经完整，不需要额外安装任何东西。**

我用一个 4 线程并发的自检程序（`code/00-hello-cpp`）验证过，
MSVC + CMake + Ninja 全链路可编译、可链接、可运行，结果正确。

---

## 2. 五分钟验证工具链

打开 **PowerShell** 或 **cmd**，执行：

```bat
cd C:\Users\Admin\Desktop\GPU
.\build.bat 00-hello-cpp run
```

> ⚠️ **注意**：在 cmd 里必须写 `.\build.bat`，
> 直接写 `build.bat` 会因为「当前目录不在搜索路径」而报错。
> 在 PowerShell 里同理。

期望输出：

```
=== 工具链自检 ===
C++ 标准: 201703
硬件并发度: 8
并行累加结果: 199999980000000  (耗时 ~5 ms)
期望值:       199999980000000
[OK] 工具链正常
```

**关键点**：如果 `C++ 标准` 显示 `199711` 而不是 `201703`，
说明 `/Zc:__cplusplus` 没生效（这是 MSVC 的默认行为，很坑）。
本项目的 `CMakeLists.txt` 已经加了这个选项，正常应该显示 201703。

### `build.bat` 的用法

```bat
.\build.bat 00-hello-cpp          :: 只构建
.\build.bat 00-hello-cpp run      :: 构建并运行
.\build.bat 00-hello-cpp clean    :: 清理 build 目录
```

以后每新增一个练习，就在 `code\` 下建一个子目录，
放 `CMakeLists.txt` + `main.cpp`，然后用同样命令构建。

---

## 3. 推荐 IDE 配置（VS Code）

你已经装了 VS Code，但要装这几个扩展才好用：

| 扩展 | 作用 |
|---|---|
| **C/C++** (ms-vscode.cpptools) | 代码补全、跳转、调试 |
| **CMake Tools** (ms-vscode.cmake-tools) | CMake 集成，一键构建调试 |
| **NVIDIA Nsight Visual Studio Code Edition** | 在 VS Code 里跑 `ncu` / `nsys`（P2 阶段必需） |

装完后按 `Ctrl+Shift+P` → `C/C++: Edit Configurations (UI)`，
把 `Compiler path` 指向：
```
C:/Program Files/Microsoft Visual Studio/18/Community/VC/Tools/MSVC/14.51.36231/bin/Hostx64/x64/cl.exe
```
`IntelliSense mode` 选 `windows-msvc-x64`。

> 💡 **更省事的替代方案**：直接用 Visual Studio 2026 本身，
> 它对 MSVC 的支持是原生的。但 VS Code 更轻，8GB 内存的机器上更友好。
> **以你的内存配置，我推荐 VS Code。**

---

## 4. 🔧 安装 Intel oneAPI（为了在核显上写 SYCL）

**这是本机唯一需要你手动安装的东西。**

### 为什么装

你的 UHD 730 支持 SYCL 2020 / OpenCL 3.0 / Level Zero。
装上 oneAPI 后，**白天在公司就能真的写 GPU 代码并跑在显卡上**，
而不是等到晚上回家才有 GPU 用。SYCL 和 CUDA 的概念是 1:1 对应的
（对照表见 [README §5](README.md)）。

### 装什么

访问 <https://www.intel.com/content/www/us/en/developer/tools/oneapi/base-toolkit-download.html>

下载 **Intel® oneAPI Base Toolkit**（Windows 版），安装时只勾选：

- ✅ **Intel® oneAPI DPC++/C++ Compiler**（必需）
- ✅ **Intel® oneAPI DPC++ Library (oneDPL)**（可选，有用）
- ✅ **Intel® oneAPI Math Kernel Library (oneMKL)**（可选，后面做 GEMM 对比有用）
- ✅ Intel® VTune™ Profiler（可选，相当于核显版的 Nsight）
- ❌ 其余全部取消勾选（AI Analytics、Advisor、Inspector 等你现在用不上）

> ⚠️ **磁盘与内存提醒**：全量安装要 15GB+，你的磁盘够（331GB 可用），
> 但**内存只有 8GB，安装过程会比较慢**。装的时候关掉浏览器和 VS。

### 装完怎么用

装完后**不需要重启**，但需要让命令行环境知道 oneAPI 在哪。
在你的 PowerShell 里每次开新终端先执行一次：

```powershell
& "C:\Program Files (x86)\Intel\oneAPI\setvars.ps1"
```

或者用 cmd：
```bat
call "C:\Program Files (x86)\Intel\oneAPI\setvars.bat"
```

### 验证

```bat
cd C:\Users\Admin\Desktop\GPU
.\build.bat 01-hello-sycl run
```

期望看到 **`设备名 : Intel(R) UHD Graphics 730`** 和 **`[OK] 已选中 GPU`**。
如果看到 `[警告] 没有选中 GPU`，说明 kernel 跑到 CPU 上了，
回去检查 oneAPI 的 `setvars` 有没有执行、驱动是不是最新。

> 💡 **顺手更新显卡驱动**：去 Intel 官网下载 Intel® Arc™ & Iris® Xe Graphics 驱动
> （你的 UHD 730 归在这个驱动包里）。最新驱动对 SYCL / Level Zero 的支持更完整。
> 当前驱动版本是 `32.0.101.7026`。

---

## 5. 🔧 家里那台机器（RTX 4070 Ti Super）

### 5.1 装 CUDA Toolkit

1. 去 <https://developer.nvidia.com/cuda-downloads>
   选 Windows → x86_64 → 你的系统版本 → exe (local)
2. **选 CUDA 12.x 的较新版本**（12.6 以上都行）。
   不要装太老的版本，Ada 架构（CC 8.9）需要 CUDA 11.8+。
3. 安装时选「自定义」，只勾：
   - ✅ CUDA → Development (compiler, libraries)
   - ✅ CUDA → Runtime
   - ✅ Driver components（如果已有较新驱动，取消勾选避免降级）
   - ❌ 其余（Nsight 除外，下面单独说）
4. 装完验证：
   ```bat
   nvcc --version
   nvidia-smi
   ```
   `nvidia-smi` 应该显示 `CUDA Version: 12.x` 和你的 4070 Ti Super (16GB)。

### 5.2 装 Nsight 工具（P2 阶段的核心武器）

- **Nsight Compute (ncu)**：kernel 级性能分析，看内存带宽、占用率、stall 原因
- **Nsight Systems (nsys)**：系统级时间线，看 kernel 之间的间隙、拷贝开销

这两个通常随 CUDA Toolkit 一起装。另外在 VS Code 里装
**NVIDIA Nsight Visual Studio Code Edition** 扩展，就能在编辑器里直接跑。

> ⚠️ **安全提醒**：如果开着「NVIDIA App」或 GeForce Experience 的
> 游戏内覆盖，`ncu` 可能会因为 GPU 被占用而失败。
> 跑 profiling 前先关掉它们。

### 5.3 选 Linux 还是 Windows？

| | Windows + MSVC | WSL2 + Linux |
|---|---|---|
| 上手难度 | 低，你已熟悉 | 中，要学 Linux |
| 与门 A 岗位匹配 | ✅ **高**（NVIDIA 上海的工具/驱动岗很多是 Windows） | 中 |
| Nsight 支持 | ✅ 完整 | ✅ 完整 |
| 生态（PyTorch/CUTLASS 编译） | 一般，比较折腾 | ✅ 好很多 |
| 内存占用 | 8GB 够呛 | 需要给 WSL 分内存，你本机不太行 |

**建议**：**家里那台先用 Windows 打通 CUDA**，把 P2 阶段走完。
等到 P3 阶段要搞 CUTLASS / PyTorch 扩展（Linux 生态明显更好），
再考虑装 WSL2 或双系统。**不要一上来就折腾环境，那是拖延症的最佳借口。**

---

## 6. 🔧 云 GPU（备用，按需）

需要大显存（>16GB）或多卡实验时才用，**不要常备**，按小时计费。

| 平台 | 卡型 | 参考价 |
|---|---|---|
| AutoDL | RTX 4090 / A100 | 约 2-8 元/小时 |
| 恒源云 | 同上 | 类似 |
| Google Colab | T4 (16GB) 免费 | 免费但限时、不稳定 |
| Kaggle | T4×2 / P100 免费 | 免费，每周 30 小时 |

**什么时候需要租**：
- 复现 FlashAttention 的大 batch 实验
- 学多卡通信（NCCL / NVLink）—— 4070 Ti Super 单卡做不了
- 跑 H100 才有的特性（FP8 的完整 Transformer Engine、TMA）

---

## 7. Git 配置建议

你的 GitHub 主页以后要作为主要作品展示。规范一点：

```bash
git config --global user.name "你的名字"
git config --global user.email "你的邮箱"
git config --global init.defaultBranch main
git config --global core.autocrlf true      # Windows 上处理换行符
```

每个练习项目建议都独立成仓库（或在这个 GPU 目录下建一个伞形仓库），
README 里必须有：
- 一段话说明这个项目解决什么问题
- **性能对比表**（这是 GPU 工程师的简历，见 [README 铁律二](README.md)）
- 构建和运行说明
- 一张 profiler 截图

---

## 8. 常见坑速查

| 症状 | 原因 | 解决 |
|---|---|---|
| `'build.bat' 不是内部或外部命令` | cmd 不搜索当前目录 | 写 `.\build.bat` |
| `.bat` 文件里的中文乱码、报奇怪的错 | cmd 用 GBK 代码页解析 UTF-8 的 bat | **`.bat` 文件只写 ASCII**（本项目已遵守） |
| 中文注释乱码 | 源文件编码 | `CMakeLists.txt` 里的 `/utf-8`（本项目已加） |
| `__cplusplus` 是 199711 | MSVC 默认行为 | 加 `/Zc:__cplusplus`（本项目已加） |
| SYCL 程序跑到 CPU 上 | `setvars` 没执行 / 驱动旧 | 执行 `setvars.bat`；更新 Intel 驱动 |
| `ncu` 报 GPU 被占用 | NVIDIA App 覆盖层 | 关掉 NVIDIA App / GeForce Experience |
| 编译时内存爆掉 | **本机只有 8GB** | 减少编译并行度；或升级内存 |
| CMake 找到的是 MinGW 的 gcc 而不是 MSVC | PATH 里有 Git 自带的工具链 | 用 `build.bat`（它会显式加载 MSVC 环境） |

---

## 9. 下一步

环境通了以后，回到 **[ROADMAP.md](ROADMAP.md#第-1-周--环境与编译到底做了什么)**
从第 1 周开始。**不要跳过 P1 阶段直接学 CUDA**——
那是整条路线里最常犯、代价最大的错误。
