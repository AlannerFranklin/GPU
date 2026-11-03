# Benchmark 骨架学习笔记

> 线程池本体是自己写的；这份笔记补的是「怎么测量」这一块 —— 产品代码和测试代码是两种独立技能。

---

## 一、任何 benchmark 只回答三个问题

### ① 怎么量时间？

```cpp
auto t0 = std::chrono::steady_clock::now();     // 记下「开始」这个时刻
//        ... 被测的那段代码 ...
auto t1 = std::chrono::steady_clock::now();     // 记下「结束」这个时刻
double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
```

这三行是固定套路，每次测速都一模一样。拆开看：

- **`std::chrono::steady_clock`** —— 单调时钟。只往前走，永远不会往回跳。测耗时必须用它。
  **不能用 `system_clock`**：那是「墙上时钟」，系统对时、时区调整都会让它跳变，测出来的时间可能变成负数。
- **`::now()`** —— 返回「当前时刻」这个对象。注意是**时刻**（时间轴上的一个点），不是**时长**。
- **`t1 - t0`** —— 两个时刻相减，得到 `duration`（时长）对象。
- **`duration<double, std::milli>`** —— 把时长换算成「以毫秒为单位、用 double 存储」。`milli` 是 `std::milli`，表示 1/1000 秒。
- **`.count()`** —— 取出里面的纯数字。`duration` 对象本身不能直接 `printf`，必须 `.count()` 拿出数值。

### ② 怎么保证公平？

- **同样的工作量**：A 和 B 必须处理相同数量的任务、每个任务做相同的计算量。差一点，比较就不成立。
- **交替跑**：每一轮里先跑 A 再跑 B。不能「A 跑三轮、B 跑三轮」—— 那样 A 全在冷状态下跑，B 全在热状态下跑（CPU 频率爬升、缓存预热），对比就偏了。
- **多轮取最小值**：干扰只会让时间**变长**，不会变短。所以最小值最接近真实性能。这就是 `bestPool = std::min(bestPool, ms);` 的来历。

### ③ 怎么证明测的是对的？

```cpp
printf("check: pool=%lld thread=%lld %s\n",
       checkPool, checkThread, checkPool == checkThread ? "一致" : "不一致");
```

两种跑法算出来的总和必须相同。**如果不同，说明它们干的工作量不一样，比快慢毫无意义。**
先验证结果一致，再看时间 —— 顺序不能反。

---

## 二、逐行注释的完整骨架

```cpp
#include <algorithm>        // std::min

const int NTASK  = 1000;    // 提交多少个任务
const int ITERS  = 1000;    // 每个任务内部算多少次
const int ROUNDS = 3;       // 重复几轮

static long long do_work(int seed, int iters) {   // 被测的「活」
    long long s = 0;
    for (int i = 0; i < iters; i++) {
        s += (long long)(seed + i) * 3;
    }
    return s;
}

int main() {
    ThreadPool pool(4);           // 4 个 worker，建一次，三轮复用

    double bestPool   = 1e18;     // 存最小值，所以初值给一个很大的数
    double bestThread = 1e18;
    long long checkPool = 0, checkThread = 0;   // 存两边的结果，用来验证工作量相同

    for (int r = 0; r < ROUNDS; r++) {          // 一共跑三轮

        // ============ A：线程池 ============
        double msA;                             // 这一轮的耗时，先声明
        {                                       // 这对花括号只是为了限定作用域
            auto t0 = std::chrono::steady_clock::now();   // ← 开始计时

            std::vector<std::future<long long>> futs;     // 装 1000 个 future
            futs.reserve(NTASK);                          // 一次性分配够，避免边 push 边扩容
            for (int i = 0; i < NTASK; i++) {
                futs.push_back(pool.enqueue([i] {         // [i] 按值捕获，见下方「坑」
                    return do_work(i, ITERS);
                }));
            }
            long long s = 0;
            for (auto& f : futs) s += f.get();            // 逐个取结果并累加；get() 会阻塞到算完
            checkPool = s;

            auto t1 = std::chrono::steady_clock::now();   // ← 停止计时
            msA = std::chrono::duration<double, std::milli>(t1 - t0).count();
        }                                                   // ← futs 在这里销毁，内存释放

        // ============ B：每任务一线程 ============
        double msB;
        {
            auto t0 = std::chrono::steady_clock::now();   // ← 开始计时

            std::vector<std::thread> ths;                 // 装 1000 个线程
            std::vector<long long> results(NTASK);        // 每个线程把结果写进自己那格
            for (int i = 0; i < NTASK; i++) {
                ths.emplace_back([i, &results] {          // 两个变量，两种捕获方式
                    results[i] = do_work(i, ITERS);
                });
            }
            for (auto& t : ths) t.join();                 // 等全部线程结束
                                                          // 注意 auto& ：thread 不能拷贝

            long long s = 0;
            for (long long v : results) s += v;           // 累加结果
            checkThread = s;

            auto t1 = std::chrono::steady_clock::now();   // ← 停止计时
            msB = std::chrono::duration<double, std::milli>(t1 - t0).count();
        }

        printf("round %d: pool=%8.3f ms  thread=%8.3f ms\n", r, msA, msB);
        //              ↑ %8.3f = 宽度 8、保留 3 位小数，对齐好看

        bestPool   = std::min(bestPool, msA);             // 只保留三轮里最小的
        bestThread = std::min(bestThread, msB);
    }

    printf("\nmin: pool=%.3f ms  thread=%.3f ms  speedup=%.2fx\n",
           bestPool, bestThread, bestThread / bestPool);  // 加速比 = 慢的 / 快的

    return 0;
}
```

---

## 三、四个必须理解的细节

### 1. 那对单独出现的 `{ }` 是干什么的

```cpp
double msA;
{
    std::vector<std::future<long long>> futs;   // 只活在这对花括号里
    ...
}                                               // ← 到这里 futs 析构，内存释放
```

如果不用花括号，`futs` 会活到 `main` 结束，三轮下来一直占着内存。
加上花括号，每一轮结束自动清理，下一轮从**完全相同的状态**开始。

### 2. `[i]` 和 `[i, &results]` —— 捕获列表可以混用

| 写法 | 含义 |
|---|---|
| `[i]` | 按值捕获 `i`：把**当时**的值拷一份进 lambda |
| `[&results]` | 按引用捕获 `results`：所有线程操作**同一个** vector |

为什么 `i` 必须按值？如果写 `[&i]`，1000 个 lambda 捕获的是**同一个变量 `i` 的引用**。
等线程真正执行时 `i` 早就变成 1000 了，而且循环结束后 `i` 已销毁 —— 悬垂引用，未定义行为。

为什么 `results` 必须按引用？每个线程要写**同一个** vector 的不同格子。
按值捕获的话每个线程拷一份自己的，写完都白写，主线程看不到。

### 3. `std::min` 为什么需要 `#include <algorithm>`

`std::min` 不是语言关键字，是标准库函数，定义在 `<algorithm>` 里。
用到标准库的哪个组件，就 include 对应的头文件 —— 不能靠别的头文件顺带拉进来。

### 4. `auto&` 里的 `&`

```cpp
for (auto& t : ths) t.join();
for (auto& f : futs) s += f.get();
```

`std::thread` 和 `std::future` 都是**只能移动、不能拷贝**的类型。
范围 for 不加 `&` 会尝试拷贝元素，直接编译失败。
而且 `f.get()` 是非 const 成员函数（它把结果移走，之后 future 失效），必须通过非 const 引用调用。

---

## 四、常见错误

| 错误 | 后果 |
|---|---|
| 用 `system_clock` 计时 | 时间可能被系统对时改掉，出现负数或荒谬值 |
| 先把 A 跑三轮再跑 B 三轮 | 冷热状态不同，比较不公平 |
| 取平均值而不是最小值 | 被偶发的系统干扰拉高 |
| 忘了检查两边结果一致 | 可能比的是两段做不同事情的代码 |
| `[&i]` 按引用捕获循环变量 | 悬垂引用，未定义行为 |
| `NTASK` 开到 10000 去做 thread-per-task | 每线程 1MB 栈，10GB 直接失败 |

---

## 五、Benchmark 1 的实测结果（2026-11-02）

```
round 0: pool=   0.754 ms  thread=  27.431 ms
round 1: pool=   0.827 ms  thread=  26.599 ms
round 2: pool=   0.853 ms  thread=  26.510 ms

min: pool=0.754 ms  thread=26.510 ms  speedup=35.15x
check: pool=2997000000 thread=2997000000 一致
```

**怎么读：**

| | 总时间 | 每任务 |
|---|---|---|
| 线程池 | 0.754 ms | 0.754 μs |
| 每任务一线程 | 26.510 ms | 26.5 μs |

**这个 35x 不是「并行让计算快了 35 倍」。**
每个任务真正的计算只有零点几微秒，而创建一个线程要 26 微秒左右 —— 开销是计算的 50 倍以上。

所以 35x 衡量的是「**线程创建开销 / 任务本身开销**」的比值。

真正的并行加速比有物理上限：本机 4 物理核，纯 CPU 活最多 4 倍；吃内存带宽的活按之前实测只有 2.1 倍（单通道 DDR5-4800）。
任务越重，这个 35x 会越往 4 倍靠。

**结论：线程池的价值是摊薄线程创建开销，不是让计算变快。**

---

## 六、Benchmark 2：线程数扫描（2026-11-03）

任务加重到每个约 0.35 ms，创建开销被淹没，测的才是真正的并行扩展性。

```
NTASK=64  ITERS=1000000
  T=1    23.115 ms  speedup=1.00x  check=96005952000000
  T=2    11.848 ms  speedup=1.95x  check=96005952000000
  T=4     6.735 ms  speedup=3.43x  check=96005952000000
  T=8     4.629 ms  speedup=4.99x  check=96005952000000
```

| T | 加速比 | 效率（÷T） | 说明 |
|---|---|---|---|
| 1 | 1.00x | — | 基准 |
| 2 | 1.95x | 98% | 几乎无额外开销 |
| 4 | 3.43x | 86% | 4 个物理核跑满 |
| 8 | **4.99x** | 62% | **超过 4 倍** |

**T=8 超过 4 倍是 SMT（超线程）的功劳。** 4.99 / 4.00 = 1.245，即超线程额外带来约 24%。
同一个物理规律在 W07 的并行归约里也复现过（8 线程比 4 线程快约 20%）。

**与 Benchmark 1 对照：**

| | 最好的一档 | 说明 |
|---|---|---|
| Benchmark 1（轻任务） | 30.6x | 省掉线程创建开销 |
| Benchmark 2（重任务） | 4.99x | 真正的并行加速比 |

同一个线程池，两个数字差 6 倍 —— 因为它们量的不是一回事。

---

## 七、Benchmark 3：任务粒度扫描（2026-11-03）

总迭代次数固定为 1 亿，只改「任务被切碎的程度」。线程数固定 T=4。

```
  NTASK=     1  ITERS=100000000     34.359 ms  0.344 ns/iter
  NTASK=    10  ITERS= 10000000     11.828 ms  0.118 ns/iter
  NTASK=   100  ITERS=  1000000     10.072 ms  0.101 ns/iter   ← 最快
  NTASK=  1000  ITERS=   100000     10.633 ms  0.106 ns/iter
  NTASK= 10000  ITERS=    10000     10.801 ms  0.108 ns/iter
  NTASK=100000  ITERS=     1000     86.118 ms  0.861 ns/iter   ← 崩了
```

**为什么各行的 `check` 不同但计算量相同：**

`do_work` 算的是 `Σ (seed + i) * 3`。总迭代次数固定为 1 亿，但任务编号的分配方式变了 —— NTASK=1 时 seed 恒为 0，NTASK=100000 时 seed 从 0 到 99999。加的数不同，总和自然不同。

公式验证：`总和 = 1.5 × NTASK × ITERS × (NTASK + ITERS − 2)`

- NTASK=1：`1.5 × 1 × 1e8 × (1 + 1e8 − 2)` = 14,999,999,850,000,000 ✓
- NTASK=100000：`1.5 × 1e5 × 1000 × (1e5 + 1000 − 2)` = 15,149,700,000,000 ✓

**三个特征：**

1. **NTASK=1 → 100：3.4x** —— 单任务只有一个 worker 在干活，切到 100 个任务后 4 个核全开。
2. **NTASK=100 是最佳点** —— 100 个任务 / 4 个 worker = 每个 25 个。任务够大（每个约 0.36 ms），调度开销可忽略；又够多，负载均衡。
3. **NTASK=100000 每迭代成本涨 8.5 倍** —— 每个任务只算 1000 次迭代 ≈ 0.36 μs 计算，但每个 worker 处理一个任务要花 3.65 μs。开销（`make_shared` 堆分配、`packaged_task` 构造、队列加锁、`notify_one`、`future` 共享状态）是计算的 9 倍。

**拐点：** 从 NTASK=10000（每个任务约 3.6 μs 计算）开始，成本明显回升。
所以这个线程池的**经济粒度下限约在 3~4 μs 计算量**。任务比这更小，调度开销开始吃掉收益。

**Benchmark 3 的结论：线程池不是「任务越碎越好」。**
