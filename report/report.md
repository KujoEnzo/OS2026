# Lab2 实验进度：模块一

## 已完成
- 实现 page_init()，读取设备树中的物理内存范围。
- 建立 Page 描述符数组，保留固件、内核及描述符占用区域。
- 将剩余完整页面交给 First-Fit 管理器。
- 将 Makefile 中 qemu、debug、test 的加载参数改为 -kernel。

## 验证结果
- 探测到物理内存范围：[0x80000000, 0x88000000)，共 128 MiB。
- 当前管理器：default_pmm_manager。
- 输出 check_alloc_page() succeeded!。
- 运行日志：logs/lab2-module1-run2.log。

## 未完成
- Best-Fit、Buddy System 和 SLUB。
- 尚未运行自动评分。

## 当前构建状态
Best-Fit 初始文件暂存为 best_fit_pmm.c.pending，
避免尚未实现的模块阻止模块一编译。
开始模块二时需恢复为 best_fit_pmm.c。
