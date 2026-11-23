# Lab4 B：PCB、内核线程创建与联合验证

成员B：田茂炜，学号2412784。日期：2026-11-22。
个人Fork：tianmaowee/OS2026，lab4；A基线来自KujoEnzo/OS2026的
lab4-a-mm-sched，固定提交b5f69440da38e60e507d3f926db6b35dc87f5cd5。

## 整体逻辑与分工

A完成page_insert/page_remove、schedule/proc_run/cpu_idle/switch_to；B保留这些算法，
实现proc.c中模块二六个函数，组织创建现场、资源回收、联调和报告。
Lab2/3提供物理页、内核堆、页表、Trap返回；Lab4将这些资源组织为可被调度的线程。

执行链：kern_init → pmm_init → vmm_init → proc_init → cpu_idle → schedule →
proc_run → switch_to → forkret → forkrets → __trapret → kernel_thread_entry → init_main。

## B的spec与验收条件

1. alloc_proc返回确定的空白PCB：UNINIT、pid=-1、零引用字段、空context/name、独立链表节点。
2. idleproc包装现有启动线程：pid0、RUNNABLE、启动栈、current指向它、不加入proc_list。
3. kernel_thread在临时trapframe中放入函数/参数和入口地址，将创建工作交给do_fork。
4. do_fork只有在PCB、独立栈、地址空间和现场均就绪后才发布PID、哈希、链表和运行状态。
5. copy_thread将trapframe复制到新线程自己的栈顶，子线程a0=0，sp正确且16字节对齐；
   context.ra=forkret、context.sp=帧地址。S-mode返回用SPP=1、SPIE=1、SIE=0。
6. copy_mm仅支持本阶段内核线程；父mm非空返回-E_INVAL，不伪装用户地址空间复制。
7. PCB/内核栈/地址空间检查失败应释放已分配资源；失败对象不进入链表、不改变线程总数。
8. 实际initproc必须通过A调度进入函数，而不通过测试直接调用init_main替代。

## 六函数实现分析

位置均为code/kern/process/proc.c的`//LAB4 function2 2412784`区域。

| 函数 | 输入/输出及关键操作 |
| --- | --- |
| alloc_proc | kmalloc取得PCB，memset清零，补UNINIT/pid=-1/物理pgdir，初始化链表；不足返回NULL |
| proc_init | 初始化全局链表、PID哈希；idle复用bootstack；kernel_thread创建pid1并命名init |
| kernel_thread | fn/arg装入tf.gpr.s0/s1，tf.epc=kernel_thread_entry，使用CLONE_VM调用do_fork |
| do_fork | 校验参数/上限，申请PCB与栈，copy_mm/copy_thread，关闭中断分配PID、挂链并唤醒；返回PID或负错误码 |
| copy_thread | 栈顶减sizeof(trapframe)得到帧位置；复制父帧，不修改调用方帧；建立首次恢复链 |
| copy_mm | 当前只有共享内核地址空间，pgdir=boot_pgdir_pa物理地址、mm=NULL；不分配用户mm |

idle不是通过fork创建的，因此其context最初为零，tf为NULL；第一次切出时switch_to保存真实现场。
新线程没有既往执行历史，必须人为构造forkret链。context只保存ABI切换所需的ra/sp/s0…s11；
trapframe负责首次sret的完整寄存器与特权状态，二者不能混用或重排。

线程入口汇编位于code/kern/process/entry.S：将s1放入a0，jalr s0调用线程函数；
函数返回后调用do_exit。当前课程do_exit本身是panic占位，所以正常init_main打印问候后
进入`process exit!!`监控是本实验边界，不代表创建失败，也不能宣传为实现了完整退出回收。

## 失败清理与原子发布

alloc_proc失败：直接返回-E_NO_MEM。setup_kstack失败：仅释放PCB。
copy_mm失败：先put_kstack，再kfree。到发布阶段前没有PID哈希、运行链表或nr_process变化。
成功时将parent、tf、context、pgdir准备完毕，再在关中断区域分配PID并挂链、增加计数、唤醒。
本实现针对单hart课程内核；关闭本地中断不等于SMP锁，不声称支持多核并发fork。

## 中断思考题

`code/kern/sync/sync.h`的local_intr_save读取sstatus.SIE，将原先是否开启中断保存在intr_flag，
必要时调用intr_disable清SIE。local_intr_restore只在进入前开启时重新打开，进入前关闭则
保持关闭，因此可嵌套使用。关键不是“最后总是开中断”，而是恢复调用前状态。

保护对象包括current、页表切换、链表与PID登记及切换现场的一致性。若只更新current就被
打断，处理器实际使用的栈或地址空间可能仍属旧线程。首次线程的SPIE=1使sret后SIE开启，
而帧中的SIE=0避免恢复尚未结束时提前允许中断；这与普通C return不同。

## 真实发现和修复的Bug

| 问题 | 证据与修复 |
| --- | --- |
| GCC14把整数传给void*诊断为错误 | build.log；kmalloc.c仅补kva2page((void*)kva)显式转换 |
| 开分页后立即取指故障 | GDB观察scause=12、stval=0x80200000；entry.S根表增加index2的1GiB恒等映射，保留原index511高地址映射 |
| 页表自检向0x100写入发生页故障 | debug-gdb.log显示scause=15；框架get_pte原来给非叶PTE置U位，新QEMU拒绝保留位，改用ptd_create；对应内置自检改为检查非叶U/A/D均零，叶权限与课程测试保持原样 |
| 课程测试线程返回类型与接口不匹配 | GCC14报void(*)(void*)传给int(*)(void*)；仅降级该类诊断为warning，不修改课程测试函数或分值 |

恒等映射当前保留在内核根表中，本阶段不是用户态隔离实现。A的page_insert/page_remove/
schedule/proc_run/cpu_idle/switch_to算法均未修改；前述框架兼容差异需要A交叉阅读。
尝试关闭Svade、旧priv_spec未解决非叶PTE问题，失败输出保留，不宣称这些尝试成功。

## 验证与复现

实测环境为本机WSL Ubuntu、GCC14.2.0、QEMU10.2、OpenSBI1.8、gdb-multiarch17.1；
完整版本输出见logs/b/environment.log。与原Ubuntu虚拟机的GCC10/QEMU6环境不同。

在工程code/下执行：

```sh
make clean
make -j4
python3 tools/b_tests/run.py --logs ../report/logs/b/reproduce-b
python3 tools/a_tests/run.py --suite all --logs ../report/logs/b/reproduce-a
python3 tools/b_tests/debug.py --init
make grade GDB=gdb-multiarch 'QEMU=qemu-system-riscv64 -kernel bin/ucore.img'
```

评分使用未经改写的tools/grade.sh及inject.py，通过QEMU命令参数兼容装载，GDB改用已安装的
多架构版本，未改变检查项/期望输出/评分。日志grade-compat.log实测**30/30**，不是100分制。
不能把该结果写成不带兼容参数的make grade通过。

| 检查 | 实测结果 | 原始证据 |
| --- | --- | --- |
| 完整构建 | 通过，修复前失败日志保留 | logs/b/build-fixed.log |
| PCB、首次现场、失败回收 | PASS | logs/b/checks-sv39/b-results.json、b-qemu.log |
| PCB申请失败、栈申请失败、copy_mm失败、进程上限、无效参数 | 不泄漏PCB/页、不改变进程数 | 同上；注入仅在临时副本 |
| 显式子栈/父帧保持 | 通过，创建对象测试后显式回收 | 同上 |
| A页映射和调度回归 | 两组ALL PASS，QEMU退出0 | logs/b/a-final/ |
| initproc实际执行 | pid1、Hello world、Bye输出 | logs/b/checks-sv39/b-qemu.log |
| GDB首次入口 | current==initproc、parent==idleproc、pgdir物理地址、satp正确 | logs/b/init-gdb.log，B_GDB_INIT_ENTRY_PASS |
| 完整兼容评分 | PMM/Process Create/Schedule全部OK，30/30 | logs/b/grade-compat.log |
| 干净Git归档复现 | 提交0019826的code归档，构建/评分退出0，30/30 | logs/b/clean-checkout-build.log、clean-checkout-grade.log |

测试没有实现真实用户地址空间复制、SMP同步或完整do_exit。测试用copy_mm非空父mm只是人为
错误输入，不是真实用户进程。日志为实际执行结果；本次没有采集桌面截图，不把文本渲染图
伪装成运行截图，最终课程要求截图时需在复现终端补拍。

## 与OS原理的联系

PCB描述生命周期和资源归属；context/trapframe构成执行上下文；独立栈隔离线程调用现场；
PID哈希支持身份查找；可运行链表与状态支持调度；事务式资源申请/逆序回收避免泄漏；
中断临界区保护单核状态。尚未涉及用户进程、地址空间复制、等待退出、SMP、完整文件系统。

## 提交边界

B模块标记为2412784；最终整理时，A郝煦享已确认学号2412695，相关代码标记已补齐。
主实现提交0019826；后续文档/验证器/归档日志提交不改变B函数逻辑。
报告按A/B章节组织：A分页思考题保留a-report.md，B中断题见本文；提示词见prompt.md。
本文件保留B阶段记录；最终复测、代码复核和真实终端截图见 [最终验收](final-validation.md)。
