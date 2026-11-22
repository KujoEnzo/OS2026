# Lab4 A 基线与来源

- 日期：2026-11-22。
- 小组仓库：https://github.com/KujoEnzo/OS2026 。本地开发分支 `lab4-a-mm-sched`，从 `origin/main` 的 `3e972c231d7fbcb074c56a3b057eb892fd912dfb` 开始。
- 框架：http://oslab.mobisys.top/lab2026/lab4.zip ，解压目录为 `lab4_new/`，映射为本仓库 `code/`。
- ZIP SHA256：`7a92f243f88ca9a4ec78b52e554a14d78a41bbf6a79a1275f6cc929aaebceba2`。
- 前置来源：`lab3` 分支 `8a7aced7d97d82f8ae64bbc7bc90e6614fa31a7b`，其中已合入 Lab3 模块二；README 的“模块二待完成”说明滞后。上游 Lab2 实现来源记录为 `7a79a86b81ef33301a28eef87db85b2ae45cb21f`。

## 迁移边界

1. `pmm.c`：迁移 `page_init`，按 Lab4 宏定义把物理上界换成 `KERNTOP - PHYSICAL_MEMORY_OFFSET`，内核起点换成 `KERNBASE - PHYSICAL_MEMORY_OFFSET`。保留 DTB 范围、描述符容量和溢出检查。
2. `default_pmm.c`：迁移 Best-Fit 的分配/释放与合并逻辑，接到框架要求的 `default_pmm_manager` 符号。框架已有 `basic_check`、`best_fit_check` 原文不变。修正框架对不存在的 `best_fit_pmm.h` 的引用，使用现有 `default_pmm.h`。
3. `trapentry.S`：只填 `RESTORE_ALL` 和调用 `trap` 的 Lab3 占位，保留 Lab4 的 `forkrets`。
4. `trap.c`：迁移中断与异常处理及其 `trap_in_kernel` 辅助函数；不携带 Lab3 十组时钟后自动关机的实验专用行为。保留 Lab4 的 `idt_init`（包含 SUM 设置）及原有 trap 分发。
5. 新实现 A 的六函数；`tlb_invalidate` 增加编译器 `memory` 屏障以约束页表写入顺序。

正式启动入口、`proc.h`、Makefile、评分脚本及评分测试均未改动。完整性记录见 [framework-integrity.json](logs/framework-integrity.json)。

## 实际发现的问题

- 首次迁移编译因 Lab2/3 中两个常量在 Lab4 中不存在而失败；根据 Lab4 `memlayout.h` 推导等价物理边界后解决。原日志：[baseline-build.log](logs/baseline-build.log)。
- 前置补齐后基线进入链接，报告尚缺 A/B 符号，符合未实现状态：[baseline-link.log](logs/baseline-link.log)。
- 本机 QEMU/OpenSBI 使用课程 `-device loader,...` 方式时只输出固件，`Domain0 Next Address` 为零，15 秒超时；使用 `-kernel bin/ucore.img` 后指向 `0x80200000` 并通过自检。对照日志见 `logs/green-pmm/pmm-qemu.log` 和 `logs/green-pmm-kernel/pmm-qemu.log`。保留课程 Makefile；A 测试运行器使用后者。

基线检查与 A 测试证明前置迁移能支撑当前 A 用例，不等同于重新认证整个 Lab2/3。
