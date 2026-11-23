# Lab4 A → B 交接
> 历史阶段记录（2026-11-22 A 交付时）。最终版本请阅读 [实验报告](report.md) 和 [最终验收](final-validation.md)。A 已署名郝煦享（2412695），B 已合入并联合通过兼容评分；下文保留当时的开发状态和失败证据，不作为当前待办。

> 2026-11-22接手更新：B模块已实现并通过QEMU边界测试、A回归、GDB入口检查，
> 兼容评分30/30。见[b-report.md](b-report.md)。以下“B尚未完成”等为A交接时的历史状态。

## 交接状态

A 的六函数已经实现；A 独立 QEMU 测试通过。正式启动与完整评分仍依赖 B 的六函数，不能提交为 Lab4 全部完成。分支为 `lab4-a-mm-sched`，提交标识用 `git log -1 --oneline` 查看。未推送 GitHub。

## B 接手顺序

1. 先阅读 [基线来源](a-baseline.md)、[接口规格](a-spec.md)、[验证记录](a-validation.md)。不要从旧 Lab3 整目录覆盖本次 Lab4 框架。
2. 在 `code/kern/process/proc.c` 的 `//LAB4 function2 YOUR CODE` 处实现 `alloc_proc/proc_init/kernel_thread/do_fork/copy_thread/copy_mm`。
3. 保留 A 在同一文件后面的 `cpu_idle/proc_run`。`sched.c`、`switch.S` 和页表操作已完成。
4. 执行 A 测试回归，再进行正常构建和课程完整评分，补录实际结果。

## 接口约定

- `proc->pgdir` 是物理地址；内核线程使用 `boot_pgdir_pa`。传入虚拟地址会使 `satp` 错误。
- `idleproc` 复用当前启动现场，`current = idleproc`；idle 不放进 `proc_list`。其他可运行线程应完成链表挂接和状态设置。
- `context` 布局为 14 个 RV64 字段：`ra/sp/s0…s11`；不要私自重排。A 的编译期断言核对主要偏移。
- `copy_thread` 的首次 `context.ra` 应指向 `forkret`，`context.sp` 指向新线程自己的 trapframe；初始 trapframe 经 `forkrets/__trapret` 恢复到 `kernel_thread_entry`。
- `kernel_thread_entry` 从 `s0` 取得函数入口、`s1` 取得参数，再调用线程函数。请按课程定义正确构造 `sstatus`，保持栈的 ABI 对齐。
- 线程可运行前应完成 PCB/栈/页表/现场初始化；失败时正确释放资源，避免半初始化对象进入运行链表。
- 本次 `copy_mm` 不要求实现用户地址空间复制，按课程内核线程阶段约定处理。

## 复现

在 Linux/WSL 的 `code/` 下：

```sh
python3 tools/a_tests/run.py --suite all --logs ../report/logs/reproduce
make -j4
```

第一条验证 A；第二条在当前交接状态会因缺少 `proc_init` 链接失败，待 B 实现后重试。

若本机使用课程 `make qemu` 只停在固件且 Next Address 为零，可使用：

```sh
qemu-system-riscv64 -machine virt -nographic -bios default -kernel bin/ucore.img
```

评分脚本内部仍保留课程原始 loader 命令。在本环境中 B 完成后可通过 `QEMU` 参数加入 `-kernel bin/ucore.img` 复查兼容性；当前完整评分在注入阶段就因 B 未完成而停止，尚不能宣称该评分兼容流程已验证。

## 联调断点

建议：`proc_init` → `do_fork` → `copy_thread` → `schedule` → `proc_run` → `switch_to` → `forkret` → `forkrets` → `__trapret` → `kernel_thread_entry` → `init_main`。

重点观察：`current/idleproc/initproc`、进程状态、链表、`need_resched/runs`、`pgdir`、`satp`、`context.ra/sp`、`tf->epc/status/gpr.sp/gpr.s0/gpr.s1`。通过 `make debug` 调试时也需留意上述固件启动参数。

## 提交前待办

- B 完成模块二并与 A 联调正式 `initproc`；运行课程完整评分。
- 两位成员分别将自己负责的课程 `YOUR CODE` 标记替换为实际学号。本次未获得 A 学号，因此保留标记并注明，不填写虚构学号。
- 汇总完整报告、真实提示词和正式运行截图。现有交付包含原始日志，没有伪造终端截图。
- 确认课程 `labcodes/lab4` 与仓库 `code/` 的映射；`code/report.md` 是报告入口，完整材料位于 `report/`。
- 整理到最终 `lab4` 分支后再提交课程；不要直接把本次 A 分支视作最终验收版本。
