# 13-transpose 笔记：shared 转置 6.8x 与 bank conflict 的账

> 记录时间：2026-11-07 12:41
> 4070 Ti Super + nvcc 12.6。对应 ROADMAP 第 11 周。实验：naive vs shared（tile[32][33] vs tile[32][32]）。

## 数据（N=1024，读写共 8MB）

| 版本 | min_ms | 相对 naive | 有用带宽 |
|---|---|---|---|
| naive（写散开） | 0.069632 | 1.0x | 115 GB/s |
| shared `tile[32][33]` | 0.010240 | **6.8x** | 781 GB/s |
| shared `tile[32][32]`（对照） | 0.015360 | 4.5x | 521 GB/s |

## 为什么是 6.8 倍而不是 8 倍

**8 倍的前提是"读写都散开"**（vector_add_strided 的场景）。naive 转置只散了一半：

- 读 `in[row*N+col]`：相邻线程 col 连续 → 合并 ✅
- 写 `out[col*N+row]`：相邻线程地址差 N → 散开 ❌

流量账：

```
完美版: 读 4MB + 写 4MB = 8MB 交易
naive:  读 4MB + 写 32MB = 36MB 交易    ← 只有写被 ×8
流量比 = 36 / 8 = 4.5 倍
```

实测的 6.8 还多一层（第三课的老熟人）：

```
naive:   36MB ÷ 0.0696ms = 517 GB/s  sector 吞吐
shared:   8MB ÷ 0.0102ms = 781 GB/s
         781 / 517 = 1.51 倍   ← 散开写压低 sector 吞吐（请求排队）
```

**6.8 ≈ 4.5（流量，惩罚折半）× 1.5（请求排队损耗）**

## 为什么 32 路冲突整体只罚 1.5 倍

冲突只发生在**一行代码**（读 tile 那一步）：

```c
out[by * N + bx] = tile[threadIdx.x][threadIdx.y];
//  warp 内 tx 变、ty 固定 → 等价于按列读二维数组
```

按列读 row-major 数组的 bank 算式：

```
tile[32][32]:  bank = (tx × 32 + ty) mod 32 = ty
               → tx 是多少都消掉，32 线程全挤 ty 号 bank → 32 路冲突、32 笔串行

tile[32][33]:  bank = (tx × 33 + ty) mod 32 = (tx + ty) mod 32   ← 33 ≡ 1 (mod 32)
               → tx 走 0..31 恰好铺满 32 个 bank，一个不重 → 1 笔
```

**33 的魔法：行宽 32 时每行回到同一起点，行宽 33 时每行错开一格。**

整体只罚 1.5 倍的原因：冲突把"读 tile 这一步"放慢了 32 倍（~30 → ~960 周期），
但这一步原本只占总时间的零头（shared 比全局内存快几十倍），主要开销是全局访存。
**bank conflict 罚的是步骤的 32 倍，不是 kernel 的 32 倍——步骤占比小，整体惩罚就小。**

⚠️ tiled matmul 里每个元素要从 shared 读两次（A tile + B tile），shared 占比高得多，
32 路冲突会罚得狠——B tile 大概率要用 `[32][33]`。

## 自检记录（2026-11-07）

| 题 | 结果 |
|---|---|
| 1. naive 流量比 4.5 的账 | ✅ |
| 2. 按列读同 bank 的算式 | ✅ |
| 3. padding 错开的算式（33 ≡ 1 mod 32） | ✅ |

三题全对，本课通过。
