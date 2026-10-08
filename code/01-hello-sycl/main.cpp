// ============================================================================
// Week 9 练习：在 UHD 730 核显上跑第一个 GPU kernel
//
// 这个文件的唯一目的是：把 SYCL 的写法和 CUDA 的写法并排放在一起，
// 让你在白天的工位上也能练 GPU 编程，而不是等到晚上回家。
//
// 【CUDA 对照】左边是 CUDA，右边是这个文件里对应的 SYCL 写法：
//
//   __global__ void k(float* a)          queue.parallel_for<kernel_name>(
//   {                                      sycl::nd_range<1>{global, local},
//       int i = blockIdx.x*blockDim.x      [=](sycl::nd_item<1> item) {
//             + threadIdx.x;                   size_t i = item.get_global_id(0);
//       ...                                  ...
//   }                                    });
//
//   k<<<grid, block>>>(d_a);             queue.submit(...) 或 queue.parallel_for(...)
//   cudaDeviceSynchronize();             queue.wait();
//
// 心智模型完全一致：都是「你写一个处理单个元素的函数，
// 运行时把它实例化到成千上万个线程上」。
// ============================================================================

#include <sycl/sycl.hpp>
#include <cstdio>
#include <vector>
#include <chrono>
#include <cmath>

constexpr size_t kNumElements = 1 << 22;   // 约 420 万个元素

// ---- 极简计时工具 --------------------------------------------------------
template <typename Fn>
double time_ms(Fn&& fn) {
    auto t0 = std::chrono::steady_clock::now();
    fn();
    auto t1 = std::chrono::steady_clock::now();
    return std::chrono::duration<double, std::milli>(t1 - t0).count();
}

int main() {
    // ------------------------------------------------------------------
    // 1. 选设备。这等价于 CUDA 里 cudaSetDevice / 选 GPU。
    //    本机只有 UHD 730 一块 GPU，所以直接找 GPU；
    //    如果 oneAPI 没配好，会退回 CPU 设备，程序依然能跑（但这就不算 GPU 编程了）。
    // ------------------------------------------------------------------
    sycl::queue q{sycl::gpu_selector_v, sycl::property::queue::in_order{}};

    auto dev = q.get_device();
    std::printf("=== SYCL 设备自检 ===\n");
    std::printf("设备名   : %s\n", dev.get_info<sycl::info::device::name>().c_str());
    std::printf("厂商     : %s\n", dev.get_info<sycl::info::device::vendor>().c_str());
    std::printf("驱动版本 : %s\n", dev.get_info<sycl::info::device::driver_version>().c_str());
    std::printf("计算单元 : %u\n", dev.get_info<sycl::info::device::max_compute_units>());
    std::printf("最大组大小: %zu\n", dev.get_info<sycl::info::device::max_work_group_size>());
    std::printf("全局内存 : %.1f GB\n",
                dev.get_info<sycl::info::device::global_mem_size>() / 1024.0 / 1024.0 / 1024.0);

    if (!dev.is_gpu()) {
        std::printf("\n[警告] 没有选中 GPU！正在用 CPU 运行，失去练习意义。\n");
        std::printf("       请检查 Intel 显卡驱动，以及是否用 icpx 编译并带了 -fsycl。\n\n");
    } else {
        std::printf("\n[OK] 已选中 GPU，下面的 kernel 真的跑在显卡上。\n\n");
    }

    // ------------------------------------------------------------------
    // 2. 准备数据。
    //    CUDA 里你需要显式 cudaMalloc + cudaMemcpy；
    //    SYCL 的 USM（统一共享内存）让主机和设备用同一个指针，省掉拷贝。
    //    这里用共享分配，让你能直接对比 CPU 结果和 GPU 结果。
    // ------------------------------------------------------------------
    const size_t bytes = kNumElements * sizeof(float);
    float* a = sycl::malloc_shared<float>(kNumElements, q);
    float* b = sycl::malloc_shared<float>(kNumElements, q);
    float* c = sycl::malloc_shared<float>(kNumElements, q);

    for (size_t i = 0; i < kNumElements; ++i) {
        a[i] = std::sin(static_cast<float>(i) * 0.001f);
        b[i] = std::cos(static_cast<float>(i) * 0.001f);
    }

    // ------------------------------------------------------------------
    // 3. 启动 kernel —— 整个文件的核心。
    //
    //    这里刻意用 nd_range 而不是简单的 range，
    //    因为 nd_range 才暴露「工作组」概念，才能和 CUDA 的 block/grid 一一对应。
    // ------------------------------------------------------------------
    constexpr size_t kLocalSize = 256;                          // == blockDim.x
    const size_t globalSize = ((kNumElements + kLocalSize - 1) / kLocalSize) * kLocalSize;
    const size_t numGroups = globalSize / kLocalSize;           // == gridDim.x

    std::printf("<<< grid=%zu, block=%zu >>>  (CUDA 术语)\n", numGroups, kLocalSize);

    double gpu_ms = time_ms([&] {
        q.parallel_for<struct vector_add_kernel>(
            sycl::nd_range<1>{sycl::range<1>{globalSize}, sycl::range<1>{kLocalSize}},
            [=](sycl::nd_item<1> item) {
                // ---- 这两行 == CUDA 的 blockIdx.x * blockDim.x + threadIdx.x ----
                const size_t i = item.get_global_id(0);
                if (i < kNumElements) {                 // 边界检查，和 CUDA 里一模一样
                    c[i] = a[i] + b[i];
                }
            });
        q.wait();                                        // == cudaDeviceSynchronize()
    });

    // ------------------------------------------------------------------
    // 4. 正确性验证
    // ------------------------------------------------------------------
    size_t errors = 0;
    for (size_t i = 0; i < kNumElements; ++i) {
        float expect = a[i] + b[i];
        if (std::fabs(c[i] - expect) > 1e-5f && ++errors > 5) break;
    }

    // ------------------------------------------------------------------
    // 5. 性能数字 —— 【铁律二】每个 kernel 都必须有数字
    //    有效带宽 = 读写总字节数 / 时间
    //    注意：核显和 CPU 共享内存带宽，这个数字不能拿去推断独显的性能。
    // ------------------------------------------------------------------
    const double gb = (3.0 * bytes) / 1e9;   // 读 a、读 b、写 c
    std::printf("\n=== 结果 ===\n");
    std::printf("元素数量 : %zu\n", kNumElements);
    std::printf("错误数量 : %zu\n", errors);
    std::printf("GPU 耗时 : %.3f ms\n", gpu_ms);
    std::printf("有效带宽 : %.1f GB/s\n", gb / (gpu_ms / 1000.0));
    std::printf("%s\n", errors == 0 ? "[OK] 结果正确" : "[FAIL] 结果错误");

    // ------------------------------------------------------------------
    // 【留给你自己的作业 —— 别用 AI，自己写】
    //
    //  1. 把 kernel 改成 grid-stride loop 写法（CUDA 里最常用的模式）：
    //         for (size_t i = item.get_global_id(0); i < N;
    //              i += item.get_global_range(0)) { ... }
    //     并解释：为什么这种写法在 N 不是 block 整数倍时更干净？
    //
    //  2. 把 kLocalSize 改成 64 / 128 / 512 / 1024，记录每种情况的耗时，
    //     画出「组大小 vs 耗时」曲线。思考：为什么不是越大越好？
    //
    //  3. 加一个第二 kernel 做 c[i] = a[i] * c[i]，两个 kernel 串行提交，
    //     计时。然后看看能不能用两个 queue 让它并行。
    //
    //  4. 把上面的 nd_item 换成 item（无工作组版本），比较代码差异，
    //     然后回答：NDRange 到底多给了你什么？
    //
    //  5. 【最重要】晚上回家，把这个文件改写成 CUDA。
    //     你会发现除了语法，几乎不用动脑子。那就是你真正学懂了。
    // ------------------------------------------------------------------

    sycl::free(a, q);
    sycl::free(b, q);
    sycl::free(c, q);
    return errors == 0 ? 0 : 1;
}
