# Lab3：Trap 上下文恢复与入口——模块一报告

> 最新状态（2026-11-05）：本文件保留模块一交接阶段记录。模块二已补全，Ubuntu完整构建、
> GDB及兼容运行检查通过，兼容评分100/100；原样评分失败另行记录。
> 后续实现、spec、验证和原理见[模块二报告](../report/module2.md)，
> [实测回传摘要](../report/logs/module2/ubuntu-reported.md)。下文“模块二待实现”等为历史状态。

## 一、范围、分工与来源

本报告是模块一的阶段报告。**模块一已实现并完成受控验证；正式模块二仍未实现，完整工程尚不能构建，未取得课程评分。** 接手人为田茂炜，负责模块二、完整联调和补齐模块二报告。模块一实现账户为 `KujoEnzo`；中文署名及两位成员学号提交前由本人确认，本文不从 Git 邮箱或旧实验资料推断学号。

课程 [Lab3 报告要求](http://oslab.mobisys.top/lab2026/_book/lab3/lab3_4_lab_requirement.html) 中的 `labcodes/lab3`，在本仓库对应工程根目录 `code/`，故报告放在 `code/lab3.md`；[report/report.md](../report/report.md) 提供入口。课程原包根目录 `lab3_new/` 的内容也对应此目录，没有换年份或换架构。

| 阶段 | 可追溯记录 |
| --- | --- |
| Lab2 已完成起点 | `7a79a86b81ef33301a28eef87db85b2ae45cb21f` |
| 官网 Lab3 基线与规格 | `3335e0e6e1663e33cba41c3d5645f6b9be0416f0` |
| 模块一最终实现与实测材料 | `c4d06288262fb8255cb3ebda06407ecc0da46b77` |
| 本次报告与交接 | 包含本文件的提交；用 `git log -1 --format=%H -- code/lab3.md` 查询，不把文档自身哈希写入自身内容 |

框架取自 [2026 课程代码页](http://oslab.mobisys.top/lab2026/labcode.html) 的 [lab3.zip](http://oslab.mobisys.top/lab2026/lab3.zip)，原包57,209字节、56个文件，SHA-256 为 `b2328d813abf194e458cc4dab68815a1f9280e6ac73c9cfdcb4d60ec10f630bb`。原包及逐文件清单在 [report/sources](../report/sources/lab3-2026-manifest.json)，来源与最小 Lab2 迁移见 [基线说明](../report/lab3-baseline.md)。没有把 Lab2 的 Buddy 扩展带入 Lab3。

## 二、整体 Trap 执行流与实验逻辑线

Lab2 提供启动映射、可用物理页描述和分配器；Lab3 在这个基础上建立“进入 Trap、保存现场、C 层处理、恢复现场、返回执行”的闭环。工作顺序是：先取得真实框架并迁移依赖，再固定结构布局与接口，随后只完成汇编模块一，用隔离测试核对底层契约，最后交由模块二补齐具体中断与异常策略。

```text
kern_entry → kern_init
  → BSS清零、DTB、控制台、内核信息
  → idt_init → pmm_init → 再次idt_init
  → clock_init → intr_enable → 等待事件

硬件把已委托的Trap送到stvec指向的入口
  → __alltraps → SAVE_ALL
  → a0 = 当前Trap frame地址 → trap(tf)
  → trap_dispatch(tf)
       cause最高位为1：interrupt_handler(tf)  [正式代码待实现]
       否则：exception_handler(tf)            [正式代码待实现]
  → C函数返回 → __trapret → RESTORE_ALL → sret
```

该图表示框架设计流程，**不表示当前正式内核已执行完整流程**：正式构建在模块二缺失函数处失败。模块一动态测试仅在副本中临时提供处理器，并在第二次 `idt_init()` 后、`clock_init()` 前执行，未启用真实时钟源。

硬件进入 S 态 Trap 时记录 sepc、scause 和特权/中断状态，不会自动保存所有 GPR，也不会替本框架切换到另一内核栈。`SAVE_ALL` 因而必须先在当前有效栈上建立帧；C 函数随后才有稳定的输入。这与普通函数调用不同：Trap 可以打断任何整数寄存器仍含有效数据的位置，不能只恢复 ABI 的 callee-saved 寄存器。

## 三、真实布局与模块一核心函数理解

### 3.1 Trap frame 与特殊的 sp 槽

`trap.h` 的 `struct trapframe` 由32个GPR槽和4个CSR快照组成。当前 RV64 的 `REGBYTES=8`、`LOAD=ld`、`STORE=sd`，总大小288字节。完整36槽表见 [module1-spec.md](../report/module1-spec.md)。关键偏移如下：

| 内容 | 相对帧首偏移 |
| --- | --- |
| xN 的 GPR 槽 | `N * 8`，包括保留的 x0 槽 |
| 旧 sp / x2 | 16 |
| status / sstatus | 256 |
| epc / sepc | 264 |
| badvaddr / sbadaddr（现称stval） | 272 |
| cause / scause | 280 |

设 Trap 进入时 sp=S。`SAVE_ALL` 先把 S 写入 sscratch，再分配288字节，得到帧首 F=S−288；随后通过 `csrrw` 取回 S 并清零 sscratch，最后把 S 存进槽2。槽2不是分配帧之后的当前 sp。s0…s4 虽被保存过程临时占用，但原值已在GPR槽中。

因此恢复期间必须一直保留 F，直到所有帧读取完成；最后再读 `16(sp)` 恢复 x2。初始帧中 S=F+288，不能由此把接口简化为“sp总是加288”，因为处理器可以合法更新帧中的栈指针。case2正是用另一有效栈顶检验这一点。

### 3.2 RESTORE_ALL

正式实现位于 [trapentry.S](kern/trap/trapentry.S)，只填了课程的第一处模块一占位：

1. 从槽32/33取出**处理后的** status/epc，用 s0/s1 写回 sstatus/sepc。
2. 从对应槽恢复 x1、x3…x31，包含 ra、gp、tp、caller-saved 参数/临时寄存器；临时借用的 s0/s1也从原槽恢复。
3. 最后从槽2恢复 sp，不再访问旧帧；由原有尾部 `sret` 返回。

x0不可改变，无需恢复可变值。badvaddr/cause是诊断快照，不是返回状态，不要求写回。返回PC取自更新后的 tf->epc；模块一不根据异常号猜测指令长度或自行统一增加2/4。

### 3.3 __alltraps、C ABI 与 __trapret

第二处占位在已有 `SAVE_ALL` 之后执行 `mv a0,sp` 和 `call trap`，把帧首地址作为 `void trap(struct trapframe *tf)` 的第一个参数。原a0和ra已经在帧中保存，调用覆盖当前寄存器不会丢失被打断现场。C返回后直接进入已有 `__trapret`，没有新增返回框架。

RV64标准ABI要求调用入口sp按16字节对齐；原栈满足该条件，减去288仍保持对齐。帧首不依赖a0跨越C调用保持，因为a0是caller-saved；C返回后sp仍应为F。最终用sret而不是普通ret：ret看ra，Trap返回看sepc及特权状态。

### 3.4 sret后的状态不等于逐位复制原status

进入Trap时，SPIE保存此前SIE，SIE清零，SPP记录来源特权级。恢复代码采用帧内status，不强制返回U态，也不无条件开中断。sret后SIE取恢复前SPIE，SPIE置1，SPP清零；本实验S态用例还直接核对了 `priv=1`。因此应分别检查sret前的CSR写回和sret后的架构转换，而非要求两时刻status逐位相同。

## 四、spec与实际prompt怎样约束实现

规格先从真实 `SAVE_ALL`、结构、宏、trap签名和现有返回路径反推契约，完整文件为 [module1-spec.md](../report/module1-spec.md)，先写的验收断言为 [module1-assertions.md](../report/module1-assertions.md)。

| 规格要素 | 具体约束 | 对应验证 |
| --- | --- | --- |
| RESTORE_ALL前置条件 | sp=F，帧有效，C可能更新epc | trap参数、帧首和处理后字段检查 |
| 后置条件 | GPR来自对应槽，sp来自槽2，sepc采用新epc | sret前真实GPR/CSR与返回后快照比较 |
| ABI | a0传帧指针，sp保持16字节对齐 | trap首指令处检查a0=sp=F、F%16=0 |
| 保护边界 | 不改SAVE_ALL/结构/trap签名/返回框架，不正式代做模块二 | 基准逐文件与允许区折回检查 |
| 失败可检测 | 不能只凭编译或后续输出 | 空实现RED；错误弹栈受控反例被拒绝 |

真实使用的用户任务正文和结果映射见 [prompt.md](../report/prompt.md)：L3-P01负责基线/规格；L3-P03明确先断言再实现、动态验证、测试副本隔离、受控反例与本地提交；L3-P04为本次报告交接。它们采用 `[PROMPT]/[RELY]/[GUARANTEE]/[SPECIFICATION]` 组织目标、依据、保证和条件。文档没有把根据结果整理的说明冒充新的用户提示词，也没有捏造额外提示词迭代。

## 五、确实发生的问题与修正

| 类别 | 真实证据与定位 | 修正及边界 |
| --- | --- | --- |
| 基线迁移缺依赖 | 首次构建报 `PHYSICAL_MEMORY_END`、`KERNEL_BEGIN_PADDR` 未定义，[原日志](../report/logs/lab3-baseline/pre-adaptation-build.log) | 核对Lab2及Lab3映射，仅迁入两个必要宏；不是Trap汇编恢复故障 |
| 空实现的预期失败 | [RED日志](../report/logs/module1/red/red-gdb.log)中无trap符号；入口未调用它，链接垃圾回收移除了函数 | 填写两处模块一实现后动态验证通过；RED不是已完成实现的随机崩溃 |
| 调试采样停点错误 | 首轮 `stepi` 后停在后续Trap而非返回首指令，[诊断日志](../report/logs/module1/diagnostic-stepi/positive-gdb.log) | 保留失败证据，改为返回目标临时断点，检查PC/状态/priv及事件数；没有因此修改正式汇编 |
| 测试证据表述不独立 | 快照为了保存t6会借用并清零sscratch，C读取槽34不能独立证明原返回值，[复核记录](../report/logs/module1/review.log) | 将C检查明确为采样清理检查，GDB在sret前和返回目标直接核对sscratch；复测通过 |
| 人工受控反例 | 副本中把槽2加载改成sp+288，case2得到错误栈顶，[反例日志](../report/logs/module1/dynamic/mutant-sp-gdb.log) | 检查正确拒绝，重新创建正确副本复测通过；这是故意注入的错误，不能写成正式代码曾误用弹栈的故障经历 |
| 仍存在的正式构建阻塞 | [完整构建日志](../report/logs/module1/formal/full-build.log)只有两个模块二函数隐式声明错误 | 本轮没有修复，交给田茂炜完成模块二；不加空处理器或降低-Werror |

现有记录没有显示模块一正式汇编完成后曾因错误偏移、提前恢复sp或临时寄存器污染而修改其逻辑。报告据实说明测试和集成问题，不补写不存在的调试故事。

## 六、验证结果及覆盖界限

详细命令、真实地址及原始输出见 [module1-validation.md](../report/module1-validation.md)；面向接手人的完整复现步骤见 [handoff.md](../report/handoff.md)。

| 检查 | 实际结论 |
| --- | --- |
| 正式对象编译、反汇编 | 退出0；检查了trap调用重定位、CSR偏移、sp最后加载、sret |
| RV64布局断言 | 43项通过，退出0 |
| 正确测试副本 | 构建0、GDB0；三组用例、每组全部32个GPR槽、处理后epc/status/sp检查通过 |
| 受控sp反例 | 构建0、GDB1；case2明确报告 `restored x2` 不符，这是检查有效的预期结果 |
| 正确副本复测 | 构建0、GDB0，事件数3、后继点检查数3 |
| 正式完整构建 | 退出2，模块二缺失，正式镜像未生成 |
| 正式原样make qemu/debug/grade | 未运行；无完整评分结论 |
| 真实异步时钟/外部中断、U态栈切换、递归Trap、FP/向量 | 未验证，不属于本轮模块一完成结论 |

两种断点分别是明确4字节的 `.4byte 0x00100073` 和2字节的 `.2byte 0x9002`。case0/1覆盖Trap前SIE=0/1；case2修改帧内sp、a5、SPIE和epc。代表性GPR具有非零高32位，gp/tp保持原有效值并比较。GDB核对trap首指令参数、C返回帧首、sret前全部整数寄存器、处理后CSR，以及返回首指令前的PC/状态/特权级；随后汇编快照和C检查提供另一层证据。这里的通过不是由某一句PASS输出单独判定。

## 七、与OS原理的对应、联系和差异

| 本实验知识点 | 原理含义与联系 | 本实验的具体边界/差异 |
| --- | --- | --- |
| Trap机制 | 异常是同步事件，中断是异步事件；内核通过共同入口组织控制转移 | 当前仅受控断点动态测试；cause最高位分类不等于已实现所有处理策略 |
| 现场保存与恢复 | 保护被打断计算的寄存器、PC和状态，才能透明继续 | Trap frame在栈上；没有PCB调度或进程地址空间切换，不能等同完整进程上下文切换 |
| ABI与栈 | 汇编/C边界需遵守参数寄存器、栈对齐和调用保存规则 | C可以破坏caller-saved寄存器，所以汇编依赖帧恢复全部整数现场 |
| 特权与中断状态 | SPP/SPIE记录返回所需状态，sret按硬件规则恢复 | 保留帧决定的状态；只有S态受控条件得到验证，未扩展通用U态栈切换 |
| 异常后的执行位置 | 可恢复异常处理必须决定重试、跳过或终止，避免重复陷入 | 模块二决定tf->epc；模块一只消费结果；RISC-V压缩指令使“全部加4”不成立 |
| 中断屏蔽与临界区 | 局部屏蔽可保护同一hart上的共享状态访问 | Lab3 PMM保留local_intr_save/restore；它不等同SMP互斥锁，不能推出多核安全 |
| 虚拟内存与启动环境 | 栈与代码访问依赖有效映射，Trap入口并不消除地址错误 | 使用既有Sv39启动映射，未实现缺页分配或交换；错误栈不是本模块可自动修复的输入 |

OS原理中重要但本轮没有对应实现/验证的内容包括：进程调度与PCB、系统调用与用户隔离、需求分页/换入换出、文件系统与持久化、多核同步与内存一致性、死锁处理、完整设备驱动，以及FPU/向量上下文管理。模块二将补上本实验要求的事件处理，也不会自动覆盖上述全部主题。

## 八、课程标记、待填项与模块二报告位置

当前保留 `lab3` 和 `YOUR CODE` 标记；不能用“占位标记还在”判断函数一定未实现，也不能把模块二空白写成完成：

- `kern/trap/trapentry.S` 两处 `#lab3 function1 YOUR CODE` 后已有模块一实现；模块一作者真实学号待填，最终提交课程前替换这两处 `YOUR CODE` 并保留 `lab3/function1`。
- `kern/trap/trap.c` 的 `//lab3 function2 YOUR CODE` 后仍为空；田茂炜完成模块二后填写其真实学号并保留 `lab3/function2`。
- 姓名/学号、模块二运行日志、规定关机次数、完整评分结果及课程要求的最终报告完整性仍待小组补齐。

田茂炜后续在本报告追加模块二章节：`interrupt_handler()`、`exception_handler()` 的规格与实际prompt、实现分析、持续时钟与规定输出/关机、真实问题修正、完整构建启动评分和原理联系。当前没有对应实现或结论可供代填。
