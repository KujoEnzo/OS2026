# Lab4 A 验证记录

## 环境与命令

在 Linux 或 WSL 中进入 `code/`，安装好 `riscv64-unknown-elf-gcc`、`make`、`qemu-system-riscv64`、Python3 后执行：

```sh
python3 tools/a_tests/run.py --suite all --logs ../report/logs/reproduce
```

可单独使用 `--suite pmm` 或 `--suite schedule`。实际版本见 [environment.log](logs/environment.log)。运行器复制源码到唯一临时目录，替换副本的启动入口并加入测试，退出时清理副本。正式代码不含启动测试钩子。

## 页映射验证

- 实现前缺少 `page_insert/page_remove`，测试链接失败：[red-pmm/pmm-build.log](logs/red-pmm/pmm-build.log)。
- 实现后课程 `check_alloc_page/check_pgdir/check_boot_pgdir`、SLOB/VMA 自检及原版 `test_lab4_pmm_basic` 通过。
- 补充用例通过：重复映射同一物理页、权限变更、不同物理页替换、引用计数、实际低地址访存验证 TLB 刷新、删除幂等、缺失中间表时的删除、首个/第二个页表分配失败。
- 分配失败测试临时限制真实物理分配器的分配次数，不修改页表算法；检查目标数据页没有新增引用、无目标映射，释放保留的中间表后页数恢复。

## 调度切换验证

- 实现前测试缺少 `schedule/proc_run/cpu_idle` 而链接失败：[red-schedule/schedule-build.log](logs/red-schedule/schedule-build.log)。
- 使用人工 PCB、独立栈和真实 A 函数，检查两个线程按顺序轮转、链表回绕、跳过睡眠线程、空链表/无可运行线程回退、唯一当前线程选择和 `runs/need_resched`。
- 用另一张物理根页表（复制内核映射）执行线程，读回 Sv39 `satp`，返回后验证原页表恢复。
- 汇编哨兵检查 `s0` 至 `s11` 保存恢复；线程通过不同栈多次往返，验证 `ra/sp` 的实际执行效果。
- 检查原中断关闭和开启两种情况下的恢复；通过直接 `proc_run` 再经嵌套 `schedule` 返回，验证跨线程的中断状态恢复。
- 最后由真实 `cpu_idle` 派发测试线程，打印最终 PASS 并通过 SBI 关机。未获得最终标记、发生 panic、超时或进程非零退出均判失败。

## 独立审查后的补强

审查发现最初的低 32 位、线程间相同寄存器哨兵会漏检高位截断。修订后使用每个线程不同的完整 64 位值，并加入只在临时副本生效的反例：

```sh
python3 tools/a_tests/run.py --suite schedule --mutation s11-lw --logs ../report/logs/red-s11-truncation
```

这条反例命令应退出 1：`s11` 的恢复从 `ld` 故意改为 `lw` 后，在寄存器比对处断言失败；即使 panic 导致 QEMU 返回 0，运行器也会判失败。正常不带 `--mutation` 的完整测试在修订后通过。最终日志位于 `logs/final/`。

## 验证边界

测试中两张根页表内容相同，能动态核实 `satp` 模式、根物理地址切换及返回恢复，但不能单独证实 `proc_run` 的全局 TLB 刷新效果；该指令通过源码及反汇编检查。页映射用例则通过不同内容物理页的实际低地址访存验证了 `tlb_invalidate`。

中断状态测试能验证保存和返回恢复，但未开启真实时钟中断；它不能独立证明 `schedule` 在查找链表期间已屏蔽中断，该临界区覆盖通过源码检查。B 接入后应再进行真实时钟和首次 trapframe 恢复路径的联合验证。

## 正常构建与完整评分

- `make -j4`：退出 2，唯一正式链接缺失为 B 的 `proc_init`。见 [normal-build.log](logs/normal-build.log)。
- `make grade GRADE_TMP=<唯一临时目录>/grade`：退出 2，课程注入器因 B 的 `proc_init` 尚不存在而停止。见 [official-grade.log](logs/official-grade.log)。因此没有完整 Lab4 分数，也未执行课程完整线程创建/调度评分。
- B 的六函数和正式 `initproc` 首次运行链仍需接入验证；人工 PCB 测试不代替这一步。

## 已知编译警告

原框架 `kmalloc.c:92` 存在传入整数给 `kva2page(void *)` 的警告；原版评分测试的两个线程回调返回 `void`，与 `kernel_thread` 的 `int` 回调签名不符。日志保留这些警告，未为消除警告修改课程测试。A 用例实际通过不意味着构建输出零警告。
