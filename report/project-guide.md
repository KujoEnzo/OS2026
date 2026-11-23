# OS2026 Lab4：项目讲解与完成情况检查

检查日期：2026-11-23。本文适合结合代码阅读、复现实验和准备答辩。

- 仓库：[KujoEnzo/OS2026](https://github.com/KujoEnzo/OS2026)。
- 最终提交分支：`lab4`（开发历史：`lab4-a-mm-sched`）。
- 固定版本：[`a9b0d532afc57ed6342d1f5b407f585f650173bb`](https://github.com/KujoEnzo/OS2026/commit/a9b0d532afc57ed6342d1f5b407f585f650173bb)。
- 最新提交已经合并 B 的 PR #3；不是此前只有 A 实现的版本。

本文基于该提交的源码、仓库原始记录和本次从 GitHub 独立检出后的实际复测。用户随后要求补齐署名并整理最终仓库，最终状态以本分支报告为准。

## 一、最终仓库状态

最终分支为 lab4；郝煦享（2412695）负责 A，田茂炜（2412784）负责 B。完整构建、A/B 专项测试通过，兼容评分30/30，署名学号和真实终端截图已补齐。当前证据和命令见 [最终验收](final-validation.md)，正式报告见 [report.md](report.md)。

本讲解的算法分析基于已合并 B 的 a9b0d53，最终整理不修改功能算法。旧分支和历史日志保留追溯，不作为课程提交入口。

## 二、这个项目究竟做了什么

Lab4 是运行在 QEMU RISC-V 虚拟机上的教学内核。它把此前实验提供的物理页分配、启动页表和中断处理组合起来，使内核能够创建线程、记录线程、挑选线程，再切换到线程函数执行。

可以沿三个问题理解整个项目：

1. **地址对应哪块内存？** 页表操作负责建立、更新和删除映射。
2. **一段函数怎样变成线程？** 创建模块为它分配 PCB 和内核栈，准备第一次执行的寄存器现场。
3. **CPU 接下来运行谁？** 调度器挑选线程，切换模块装载它的地址空间和寄存器。

这里主要实现内核线程。虽然结构和函数叫 `proc_struct`、`do_fork`，仍不能据此认为完整的用户进程、Linux fork 或用户地址空间复制已经实现。

## 三、先认识项目结构

| 位置 | 阅读重点 |
| --- | --- |
| `code/kern/init/entry.S` | 开启分页、设置启动栈、跳入 C 入口 |
| `code/kern/init/init.c` | 内核初始化顺序 |
| `code/kern/mm/default_pmm.c` | Best-Fit 物理页分配、释放与合并 |
| `code/kern/mm/pmm.c` | 物理内存初始化、页表查找、映射维护 |
| `code/kern/mm/kmalloc.c` | 为 PCB 等小对象提供内核堆分配 |
| `code/kern/mm/vmm.c` | VMA 区间组织及检查；不等于已实现换页 |
| `code/kern/process/proc.h` | PCB 和 context 的实际字段 |
| `code/kern/process/proc.c` | 创建、初始化、空闲循环、进程切换 |
| `code/kern/process/switch.S` | 最底层寄存器切换 |
| `code/kern/process/entry.S` | 新内核线程的函数调用入口 |
| `code/kern/schedule/sched.c` | 唤醒状态和可运行线程选择 |
| `code/kern/trap/trapentry.S` | 完整异常现场恢复，以及 `forkrets` |
| `code/kern/sync/sync.h` | 保存并恢复中断状态 |
| `code/tools/a_tests/`、`b_tests/` | 专项测试及故障注入工具 |
| `report/` | 分模块讲解、spec、提示词、验证日志 |

## 四、从上电到 initproc 的完整路线

```text
OpenSBI
  → kern_entry                       启动汇编、开启分页
  → kern_init                        C 初始化入口
      → dtb_init / cons_init         读取硬件信息、启用控制台
      → pmm_init                     物理页、页表检查、内核堆
      → pic_init / idt_init          中断与异常入口
      → vmm_init                     VMA 管理检查
      → proc_init                    建立 idleproc，创建 initproc
      → clock_init / intr_enable     时钟与中断使能
      → cpu_idle
          → schedule                 选择可运行线程
          → proc_run                 更新 current、页表
          → switch_to                装载新 context
          → forkret
          → forkrets
          → __trapret / sret          恢复首次 trapframe
          → kernel_thread_entry      取出函数与参数
          → init_main                打印线程信息和问候
```

源码入口：[init.c](https://github.com/KujoEnzo/OS2026/blob/a9b0d532afc57ed6342d1f5b407f585f650173bb/code/kern/init/init.c)。

这条路线分成两段：先把“线程对象和执行现场”准备好，再让调度器真正把 CPU 交给它。仅检查 PCB 存在，不能代替验证线程函数实际执行。

## 五、模块一：页映射与引用计数

### 1. 三种地址和对象

- **物理地址**：内存中的实际位置，例如根页表的 `boot_pgdir_pa`。
- **虚拟地址**：开启分页后程序使用的地址，处理器通过页表翻译。
- **`struct Page`**：描述一个物理页的管理对象，保存引用计数、标记和空闲链表信息；它不是页内数据本身。

`page2pa` 将描述对象转换为物理地址，`page2kva` 得到内核可直接访问该页的虚拟地址。`PADDR/KADDR` 处理内核线性映射换算，不能把任意用户虚拟地址都当作简单偏移转换。

### 2. get_pte 如何查找

Sv39 将基础页地址分解为三级索引和 12 位页内偏移。每级页表为 4 KiB，有 512 个 8 字节表项，每级索引为 9 位。

`get_pte(pgdir, la, create)` 走过两级中间表，最后返回目标叶表项的内核地址。中间表不存在且 `create` 为真时，它申请一页、清零并挂接；否则返回 NULL。

“获得 PTE 地址”还不等于“地址已经映射”：叶 PTE 是否有效、指向哪一页，仍由后续映射操作决定。

### 3. page_insert 的关键顺序

[page_insert 源码](https://github.com/KujoEnzo/OS2026/blob/a9b0d532afc57ed6342d1f5b407f585f650173bb/code/kern/mm/pmm.c#L330) 的行为是：

1. 取得目标 PTE；需要的页表页分配失败则返回 `-E_NO_MEM`。
2. 先增加新物理页的映射引用。
3. 如果旧映射也是同一物理页，抵消多加的引用，只更新权限。
4. 如果旧映射指向其他页，撤销旧映射及其引用。
5. 写入新 PTE，使对应 TLB 翻译失效。

例如，把地址 X 从页 A 换到页 B，应同时做到 `PTE(X)→B`、`ref(A)-1`、`ref(B)+1`。如果只是把 A 的权限从可写改为只读，A 的引用总数应保持不变。

先持有新引用，是为了避免同页重映射时先把引用减到零、意外将仍在使用的页交回分配器。

### 4. page_remove 与 TLB

`page_remove` 对无映射地址直接返回；有映射时先清除 PTE、使对应 TLB 失效，再递减引用，归零时释放页。

TLB 是处理器缓存的地址翻译。只修改内存中的 PTE，处理器可能继续使用旧翻译。项目用 `sfence.vma` 同步翻译变化，并添加编译器内存屏障约束页表写入顺序。

数据页引用与中间页表页的生命周期不同。当前框架 `get_pte` 在第二次分配失败时可能保留第一次申请的中间表供复用，不能把它描述为整个页表路径的事务回滚；此时目标叶映射没有建立，目标数据页也没有新增引用。

## 六、模块二：把一个函数包装成内核线程

### 1. PCB、context、trapframe 各自保存什么

| 对象 | 作用 | 典型内容 |
| --- | --- | --- |
| `proc_struct` | 线程身份、资源和生命周期 | PID、状态、父线程、栈、页表、链表 |
| `context` | 函数调用边界上的线程切换现场 | `ra`、`sp`、`s0…s11` |
| `trapframe` | 异常返回及首次启动所需的完整现场 | 通用寄存器、`status`、`epc`、异常信息 |

RV64 的 context 有 14 个 8 字节字段，即 112 字节；本项目 trapframe 为 36 个寄存器宽度槽，即 288 字节。内核栈为两页，即 8 KiB。

### 2. 六个创建函数的协作

创建模块位于 [proc.c](https://github.com/KujoEnzo/OS2026/blob/a9b0d532afc57ed6342d1f5b407f585f650173bb/code/kern/process/proc.c#L222)。

| 函数 | 在本项目中的实际职责 |
| --- | --- |
| `alloc_proc` | 申请 PCB、清零，设 UNINIT、PID=-1、内核物理页表根，初始化链表节点 |
| `copy_mm` | 当前只接受内核线程的共享内核地址空间；父 `mm` 非空则返回错误 |
| `copy_thread` | 把 trapframe 放到新内核栈顶，准备返回寄存器、特权状态和 context |
| `do_fork` | 组合 PCB、独立栈、地址空间和现场，最后发布 PID、链表、计数和可运行状态 |
| `kernel_thread` | 将函数放进 `s0`、参数放进 `s1`，把 `epc` 指向线程入口，再调用 `do_fork` |
| `proc_init` | 初始化全局结构，包装 idleproc，创建 PID=1 的 initproc |

`do_fork` 的失败清理遵循逆序回收：PCB 失败直接返回；栈失败释放 PCB；地址空间检查失败先释放栈再释放 PCB。成功准备完毕后才在中断保护下登记和唤醒，避免半初始化对象被调度。

### 3. idleproc 与 initproc 为什么不同

`idleproc` 是把当前已经执行的启动现场包装成线程，PID 为 0，复用 `bootstack`，不会再为它申请新线程栈，也不加入普通线程链表。

`initproc` 才是通过 `kernel_thread/do_fork` 新建的线程，PID 为 1，有独立内核栈，父线程为 idleproc。`current` 最初指向 idleproc，切换后指向 initproc。

### 4. 为什么第一次运行要构造现场

普通线程可以恢复“上次停下的位置”，新线程没有上次运行历史。因此代码主动设置：

```text
context.ra = forkret
context.sp = 新线程 trapframe 的地址
trapframe.epc = kernel_thread_entry
trapframe.s0 = 线程函数
trapframe.s1 = 函数参数
```

第一次 `switch_to` 的 `ret` 就跳到 `forkret`；它把 trapframe 交给 `forkrets`，后者设置栈并进入 `__trapret`。恢复 `sepc`、`sstatus` 等寄存器后，`sret` 才进入线程入口汇编。

线程入口把 `s1` 搬到参数寄存器 `a0`，通过 `s0` 调用线程函数。这解释了为什么创建模块中的寄存器赋值必须与入口汇编一致。

`copy_thread` 设置 `SPP=1、SPIE=1、SIE=0`，保证首次返回到 S 模式，并在 `sret` 后按预期使能中断。它不是简单的普通 C 函数返回。

## 七、模块三：选择线程与切换现场

### 1. schedule 只决定接下来运行谁

[schedule](https://github.com/KujoEnzo/OS2026/blob/a9b0d532afc57ed6342d1f5b407f585f650173bb/code/kern/schedule/sched.c#L15) 从当前线程的下一项沿链表循环，跳过哨兵和非 RUNNABLE 线程，选到目标后调用 `proc_run`。一圈没有其他可运行线程时，可以重新选择当前线程；找不到普通可运行线程则回到 idleproc。

它清除调用者的 `need_resched`，并增加选中对象的 `runs`。`runs` 表示调度选择次数，不能当作执行指令数或精确 CPU 时间。

注意：本项目的定时器处理更新时钟和 ticks，没有实现完整的按时间片抢占调度。不能仅因为存在 `clock_init`，就宣称已经实现完整的抢占式轮转。

### 2. proc_run 管理 current 和页表

[proc_run](https://github.com/KujoEnzo/OS2026/blob/a9b0d532afc57ed6342d1f5b407f585f650173bb/code/kern/process/proc.c#L359) 在目标不等于当前线程时：

```text
保存并关闭本地中断
→ prev = current
→ current = 目标线程
→ 写入 satp，执行 sfence.vma
→ switch_to(prev.context, current.context)
→ 将来恢复到这里时，恢复原有中断状态
```

Sv39 的编码为：

```c
satp = ((uintptr_t)8 << 60) | (proc->pgdir >> 12);
```

`pgdir` 必须是根页表的物理地址，`>>12` 取得物理页号。把内核虚拟地址直接放进去，会指向错误位置。当前内核线程通常共享同一内核页表，但接口仍负责维护正确的地址空间切换约定。

### 3. switch_to 只负责保存和恢复

[switch.S](https://github.com/KujoEnzo/OS2026/blob/a9b0d532afc57ed6342d1f5b407f585f650173bb/code/kern/process/switch.S#L8) 以旧 context 和新 context 指针为参数，保存旧 `ra/sp/s0…s11`，再恢复新线程对应寄存器，执行 `ret`。

`ret` 在这里可以回到另一个线程曾经暂停的位置；首次调度则进入人为设置的 `forkret`。`switch_to` 不查找线程、不决定公平性，也不负责完整异常现场的恢复。

### 4. 中断保护为什么要“恢复原状态”

`local_intr_save` 保存进入前的 `sstatus.SIE`，必要时关闭中断；`local_intr_restore` 只在原本开启时重新开启。

因此，外层 schedule 已关中断时，内层 proc_run 不会在完成局部工作后擅自提前开中断。保护目标是让 `current`、页表、运行栈和链表状态保持一致。此机制面向单 hart；关闭本地中断并不是多核锁。

## 八、B 合入时的兼容修改为什么必要

相对于 A 阶段版本，除新增 B 六函数外，还有以下框架修正：

| 修改 | 原因和边界 |
| --- | --- |
| 启动根表增加 index 2 恒等映射 | 开启 satp 后最初仍在物理 PC 附近取指，保留低物理地址映射以完成向高地址内核的过渡 |
| 中间页表使用 `ptd_create` | Sv39 非叶表项不能照搬叶 PTE 的 U/A/D 位；叶映射仍独立设置权限 |
| 更新一处非叶 U 位自检 | 原断言要求中间表 U=1，与新的合法非叶编码冲突；改为核实非叶保留位为零 |
| `kva2page((void *)kva)` | 给地址值到指针的转换补显式类型，兼容较新的编译器诊断 |
| 回调类型诊断降为 warning | 课程评分测试的 void 回调与 int 回调接口不一致，保留原评分测试内容 |

根表 index 511 的高地址内核映射依然保留。恒等映射在当前根表内继续存在，所以也不能把这套页表称为已经完成了严格的用户/内核隔离。

源码对照依据是本次固定提交与 A 阶段 `b5f6944` 的差异，具体调试经历见仓库的 [B 报告](https://github.com/KujoEnzo/OS2026/blob/a9b0d532afc57ed6342d1f5b407f585f650173bb/report/b-report.md)。

## 九、如何复现实验

在 Linux/WSL 中准备 RISC-V 交叉 GCC、QEMU、make、Python3 和支持 RISC-V 的 GDB。建议固定本次检查提交，避免以后分支更新影响复现：

```sh
git clone -b lab4 https://github.com/KujoEnzo/OS2026.git
cd OS2026
# 功能基线为 a9b0d53；直接使用 lab4 获取最终整理版本
cd code

make -j4
python3 tools/a_tests/run.py --suite all --logs ../report/logs/reproduce-a
python3 tools/b_tests/run.py --logs ../report/logs/reproduce-b
```

普通运行可显式使用内核装载参数：

```sh
qemu-system-riscv64 -machine virt -nographic -bios default -kernel bin/ucore.img
```

兼容评分：

```sh
make grade GDB=gdb-multiarch 'QEMU=qemu-system-riscv64 -kernel bin/ucore.img'
```

评分使用临时副本并清理构建文件。本次检查额外把 `GRADE_TMP` 指向唯一临时目录，避免与其他评分任务共用路径；具体命令记录在最终验收的 *-exit.json。

### 如何理解最后的 panic

普通线程完成后，入口汇编调用 `do_exit`。当前课程实现仍为：

```c
panic("process exit!!.\n");
```

因此，日志先出现 `pid = 1`、`Hello world!!`、`Bye`，随后出现 `process exit!!`，说明线程函数已执行并走到当前框架的退出占位。它不是线程创建失败的证据，也不是“完整退出和资源回收已实现”的证据。

不能把任意 panic 都解释为正常；必须核对出现位置和此前输出。A 测试运行器仍把意外 panic 视为失败。

## 十、建议阅读与答辩顺序

按以下顺序看代码，每一步都回答一个具体问题：

1. `init.c`：启动时先准备哪些资源，何时建立线程？
2. `proc.h`：一个线程至少需要保存什么？
3. `proc_init`：为什么 idle 不是 fork 出来的？
4. `kernel_thread/do_fork`：函数入口、PCB、内核栈和身份如何组合？
5. `copy_thread + entry.S + trapentry.S`：第一次运行为什么能跳到线程函数？
6. `schedule/proc_run/switch.S`：选择线程、切页表和切寄存器怎样分工？
7. `pmm.c`：映射、引用计数与 TLB 如何维持一致？
8. A/B 测试与 grade：每个 PASS 到底证明什么，还有哪些边界未覆盖？

答辩时尤其要能说明：**同页重映射为何不能先释放、context 与 trapframe 为何不同、pgdir 为何必须是物理地址、非叶 PTE 为何不能随意置 U 位，以及为什么本实验能运行内核线程，却还不是完整的进程管理系统。**
