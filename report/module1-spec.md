# Lab3 模块一规格：RESTORE_ALL 与 __alltraps

状态：**模块一已按本规格实现并完成受控验证**，当前结果见 [模块一验证记录](module1-validation.md)。本文件保留基线时确定的布局、契约与验收要求。分工：本人负责模块一；模块二不在本次工作范围。规格绑定 [基线记录](lab3-baseline.md) 中 SHA-256 为 `b2328d813abf194e458cc4dab68815a1f9280e6ac73c9cfdcb4d60ec10f630bb` 的 2026 课程框架及本地最小 Lab2 迁移。

## 1. 依据与允许修改范围

直接依据：`code/kern/trap/trapentry.S` 的 `SAVE_ALL` 和模块一两处占位、`trap.h` 的结构和签名、`trap.c` 的 `idt_init/trap/trap_dispatch`、`libs/riscv.h` 与 `defs.h`、启动栈及课程上下文测试。课程要求见 [2026 Lab3 练习](http://oslab.mobisys.top/lab2026/_book/lab3/lab3_2_1_exercise.html)。通用调用规则参考 [RISC-V psABI，整数调用约定](https://riscv-non-isa.github.io/riscv-elf-psabi-doc/#_integer_calling_convention)；Trap 状态语义参考 [RISC-V Supervisor-Level ISA](https://docs.riscv.org/reference/isa/v20260120/priv/supervisor.html)。文件定义决定布局，架构规范用于解释调用与返回语义。

模块一实现范围仅限填写 `trapentry.S` 中 `RESTORE_ALL` 与 `__alltraps` 的两处 `#lab3 function1 YOUR CODE`。不得修改 `SAVE_ALL`、`struct pushregs`、`struct trapframe`、`void trap(struct trapframe *tf)` 签名、课程测试/评分文件，或在模块一实现中代做 `interrupt_handler/exception_handler`。保留既有 `__trapret` 和尾部 `sret`；不在文档中规定唯一指令序列。

## 2. 实际框架前置条件

- 目标为 RV64，`riscv.h` 给出 `STORE=sd`、`LOAD=ld`、`LOG_REGBYTES=3`、`REGBYTES=8`；`defs.h` 的 `uintptr_t` 为 64 位无符号整数。当前交叉编译器默认 `rv64imafdc/lp64d`，不能误按 RV32 生成 4 字节槽。
- `idt_init()` 将 `sscratch` 置零，并把 `stvec` 设置为 `__alltraps`；入口 `.align(2)` 保证 4 字节地址对齐，使用 Direct 模式。硬件在已委托的 Trap 进入 S 态时更新 `sepc/scause/sstatus` 等 CSR，不会自动保存 GPR 或自动改写 `sp` 为另一内核栈。
- `entry.S` 使用 `bootstacktop`，栈区页对齐，`KSTACKSIZE = 2 * 4096 = 8192`。本实验处理来自内核 S 态的 Trap，前置条件为当前 `sp` 指向有效、可写、16 字节对齐且有足够剩余空间的内核栈；至少需要 288 字节 Trap frame 加后续 C 调用栈。
- 当前 `SAVE_ALL` 不提供 U 态到内核栈的切换，也没有完整用户线程/FPU/向量上下文协议。因此不宣称模块一支持任意用户栈、任意坏栈、浮点/向量上下文或任意递归异常恢复。不增加这些功能。
- Trap frame 和 C 调用访问的地址必须在当前 Sv39 映射中有效。入口保存、恢复阶段不能发生栈访问异常；正常处理期间不主动提前开中断。

## 3. Trap frame 完整布局

以下偏移均相对 `F = (struct trapframe *)sp`（执行完 `SAVE_ALL` 后的栈指针），单位为字节。每槽 8 字节，`gpr` 起始偏移为 0。GPR 槽序号严格等于 x 寄存器编号，禁止因 sp 特殊处理而挤掉槽 2。

| 槽 | 字段 / 寄存器 | 偏移 | 槽 | 字段 / 寄存器 | 偏移 |
| --- | --- | --- | --- | --- | --- |
| 0 | `gpr.zero` / x0 | 0 | 16 | `gpr.a6` / x16 | 128 |
| 1 | `gpr.ra` / x1 | 8 | 17 | `gpr.a7` / x17 | 136 |
| 2 | `gpr.sp` / x2 | 16 | 18 | `gpr.s2` / x18 | 144 |
| 3 | `gpr.gp` / x3 | 24 | 19 | `gpr.s3` / x19 | 152 |
| 4 | `gpr.tp` / x4 | 32 | 20 | `gpr.s4` / x20 | 160 |
| 5 | `gpr.t0` / x5 | 40 | 21 | `gpr.s5` / x21 | 168 |
| 6 | `gpr.t1` / x6 | 48 | 22 | `gpr.s6` / x22 | 176 |
| 7 | `gpr.t2` / x7 | 56 | 23 | `gpr.s7` / x23 | 184 |
| 8 | `gpr.s0` / x8 | 64 | 24 | `gpr.s8` / x24 | 192 |
| 9 | `gpr.s1` / x9 | 72 | 25 | `gpr.s9` / x25 | 200 |
| 10 | `gpr.a0` / x10 | 80 | 26 | `gpr.s10` / x26 | 208 |
| 11 | `gpr.a1` / x11 | 88 | 27 | `gpr.s11` / x27 | 216 |
| 12 | `gpr.a2` / x12 | 96 | 28 | `gpr.t3` / x28 | 224 |
| 13 | `gpr.a3` / x13 | 104 | 29 | `gpr.t4` / x29 | 232 |
| 14 | `gpr.a4` / x14 | 112 | 30 | `gpr.t5` / x30 | 240 |
| 15 | `gpr.a5` / x15 | 120 | 31 | `gpr.t6` / x31 | 248 |
| 32 | `status` / 保存的 sstatus | 256 | 34 | `badvaddr` / 保存的 sbadaddr | 272 |
| 33 | `epc` / 保存的 sepc | 264 | 35 | `cause` / 保存的 scause | 280 |

`sizeof(struct pushregs)=256`；`sizeof(struct trapframe)=36*8=288=0x120`；C 结构对齐为 8 字节，没有额外尾部字段或填充。C 结构对齐 **不等于** C 调用栈的 16 字节 ABI 对齐要求。本次已用 RV64 编译器的 43 项编译期断言核实，探针见 [sources/trapframe-layout-check.c](sources/trapframe-layout-check.c)。

## 4. sp 的特殊保存与恢复约束

设 Trap 刚进入 `__alltraps` 时 `sp=S`。真实 `SAVE_ALL` 的关系是：

1. 先将 `S` 写入 `sscratch`，再令 `sp=S-288=F`。
2. 保存 x0、x1、x3…x31 到各自槽位；**没有**直接把已经下移的 x2 写进槽 2。
3. `csrrw s0, sscratch, x0` 取回旧栈指针 `S`，同时清零 `sscratch`；随后把这个 `s0` 写入 `F+16`。原先 x8/s0 早已保存在 `F+64`，不会丢失。
4. 此时 `F->gpr.sp=S`，当前 `sp=F`，`sscratch=0`。s0…s4 已用于取得 CSR，但它们的 Trap 前值仍在 GPR 槽中。

因此恢复必须保持帧地址可用直到最后一次帧访问，并从 **槽 2 的最终值** 恢复 x2。不能在尚有 `offset(sp)` 读操作时提前恢复 sp，也不能先恢复其他寄存器后又用它们作临时值却不还原。初始帧满足 `S=F+288`，但仅加 288 不能替代恢复 `F->gpr.sp` 的接口语义，尤其不能忽略处理后帧中允许更新的寄存器值。

模块一不从 `sscratch` 恢复旧 sp：它在保存末尾已是零，旧 sp 的权威副本在槽 2。对本实验 S 态内核 Trap，返回时应维持 `sscratch=0` 的内核约定。清零 scratch 的源码注释不等于入口已经实现了通用的用户态栈切换或可恢复的嵌套异常协议。

## 5. RESTORE_ALL 契约

**输入**：正常返回的 `trap(tf)` 处理后的有效 Trap frame，入口 `sp=F`。`F` 指向上述 288 字节，仍在当前内核栈上，帧内 `gpr/status/epc` 是应恢复状态；帧可以被 C 处理函数合法更新。待恢复状态必须适合本实验平台，包括有效返回 PC、正确 SPP/SPIE 和可用栈。

**输出（到达随后 sret 前）**：

- x1…x31 从相应 GPR 槽恢复，其中 x2 来自 `F+16`；x0 仍为硬连线零，无需赋予可变状态。恢复范围包含 caller-saved 寄存器以及 ra/gp/tp，不仅是 s0…s11。
- `sstatus` 和 `sepc` 分别采用 **处理后的** `F->status`、`F->epc`（受 CSR 可写位/合法值限制），不从入口暂存寄存器或旧 CSR 再取回过期值。
- `badvaddr/cause` 是异常诊断快照；模块一不需要把它们写回 `sbadaddr/scause`，也不把它们当返回地址。保存的 `sbadaddr` 是该框架使用的旧 CSR 名称（对应现今 stval）。
- 所有临时工作完成后，被打断执行流不应观察到恢复代码额外留下的 GPR 改动。不得破坏帧外内存，也不得越界读取第 36 槽之后的数据。

**约束**：宏本身完成恢复，不另行加入普通函数 `ret`；后续由框架的 `__trapret` 尾部 `sret` 返回。恢复 CSR 使用的临时寄存器最终也必须恢复；恢复栈指针以后不得继续使用旧帧相对寻址。正常保存的 status 中 SIE 为 0，处理及恢复阶段不主动开启 SIE；不能把 SIE/SPIE 一律置 1。

## 6. __alltraps 契约与 C ABI

**输入**：硬件进入 `stvec` 所指入口时的 Trap 状态和 GPR，满足前述有效内核栈条件；不是一个普通 C 函数调用，不依赖 ra 指向中断前 PC。

**行为 / 输出**：

1. 使用已有 `SAVE_ALL` 完成整个帧之后，才建立 C 参数。
2. 在调用 `void trap(struct trapframe *tf)` 的瞬间，`a0` 必须等于 **帧首地址 F**，不是 `F->gpr.sp`、`&F->epc` 或帧内容的副本。原 x10/a0 已保存在 `F+80`，作为参数覆盖当前 a0 不得覆盖该槽。
3. 使用能够正常返回到统一恢复路径的调用关系，每个正常 Trap 调用一次 `trap`；C 返回后转入 `__trapret`。C 函数返回类型为 void，不能把返回时的 a0 当作 F。
4. RV64 标准 ABI 要求调用入口 `sp % 16 == 0`，并在 C 执行期间保持对齐。框架启动栈满足对齐，保存量 288 是 16 的倍数，故 `F=S-288` 保持对齐。不得加减一个 8 字节槽而破坏对齐，也不能盲目向下取整导致丢失帧地址。
5. `trap` 可按 ABI 改写 a0…a7、t0…t6、ra。帧与其基址不能仅依靠 caller-saved 寄存器跨越调用保存。若入口有额外栈调整，必须维护对齐、保证参数仍是 F，且在 `__trapret` 恢复前令 `sp` 精确回到 F。框架要求的关键不变量是 C 调用前后的帧基址一致。
6. 调用指令对当前 ra 的改写不影响帧内保存的原 ra；恢复原 ra 后由 `sret` 使用 `sepc` 跳转。禁止用普通 `ret` 代替 Trap 返回。

调用闭环为：硬件 → `__alltraps` → `SAVE_ALL` → `trap(F)` → `trap_dispatch(F)` → 模块二处理函数 → 返回 → `__trapret` → `RESTORE_ALL` → `sret`。

## 7. 处理后的 epc 与返回状态

模块一消费模块二交回的帧，**不判断异常指令长度、不处理时钟计数、也不自行调整 epc**。

- 异步中断通常不改 `tf->epc`，返回后在被打断位置继续；模块一不能统一加 2 或加 4。
- 模块二对可跳过的断点/非法指令处理后可能更新 `tf->epc`；模块一必须将这个新值写入 `sepc`，不能恢复刚进入时的旧 PC。具体应该前进 2 字节还是 4 字节由实际指令编码及模块二规则决定，不能只看异常编号。本规格验收可用已知编码长度的测试指令。
- 硬件进入 S 态 Trap 时，保存到帧的 sstatus 已经反映 Trap 进入语义：SPIE 得到 Trap 前 SIE，SIE 清零，SPP 记录原特权级。本实验来自 S 态时 `SPP=1`。
- 设恢复前的帧 status 为 Q，正常 `sret` 后 PC 来自恢复的 sepc；返回特权级由 Q.SPP 决定（本实验返回 S 态）；SIE 取 Q.SPIE，SPIE 置 1，SPP 清零。因此 **sret 后的 sstatus 不必逐位等于 Q**。恢复代码不得硬编码 SPP、强制开中断或以 `mret` 取代 `sret`。
- `trap_in_kernel(tf)` 实际通过 `tf->status & SSTATUS_SPP` 判断，宏值分别为 `SSTATUS_SIE=0x2`、`SSTATUS_SPIE=0x20`、`SSTATUS_SPP=0x100`。验收区分“sret 前 CSR 写回正确”和“sret 后架构状态转换正确”。

## 8. 验收方法与判据

### A. 当前基线即可执行的静态检查

1. 对照来源清单，确认 `SAVE_ALL`、`trap.h`、`trap.c` 和评分文件原样；实施模块一后仅两处允许区发生变化。
2. 在 `code/` 编译 `../report/sources/trapframe-layout-check.c`，命令见基线记录。43 项断言全部通过才接受布局；若目标架构、字段或宏改变，重新核对规格，不继续套用偏移。
3. 单独构建 `make -B obj/kern/trap/trapentry.o`；检查 `riscv64-unknown-elf-objdump -dr obj/kern/trap/trapentry.o`，确认调用 trap、CSR 写回、寄存器恢复、sp 最后恢复和 sret 的路径。该对象可能显示调用重定位，不能把未链接的地址当最终运行地址。

基线准备时 A.2 和汇编编译通过，而 A.3 因占位尚空不满足。当前实现后的编译、反汇编及动态验收结果见 [模块一验证记录](module1-validation.md)；不能仅凭对象编译成功宣称上下文恢复正确。

### B. 模块一后续独立动态验收（不依赖交付模块二实现）

在另建的临时验证副本/测试工具中提供受控的 `trap(tf)` 测试替身，或在调试器中接管进入 C 后的验证路径；不得将测试替身提交到正式 `code/`，不得修改课程评分文件。测试替身只用于检验本模块的 ABI、帧与返回行为，不算完成模块二。

| 场景 | 检查 / 通过条件 |
| --- | --- |
| 帧传参 | 在 `trap` 入口检查 a0 等于 F，`a0 % 16 = 0`；原 ra、a0、s0 与其他寄存器都能从正确槽读取；C 调用返回后 sp 精确为 F |
| 全部整数上下文 | 使用汇编测试工具设置可安全改变寄存器的不同哨兵值，保存调用者状态并遵守测试工具自身 ABI；gp/tp 保持有效并比较，sp 使用合法对齐栈。进入断点后检查全部 x1…x31，尤其 ra、sp、a0、s0…s4 和 t 寄存器；x0 恒零 |
| C 调用破坏 | 测试 C 替身故意按 ABI 使用 caller-saved 寄存器；返回后不得把其 a0/ra/t 寄存器残值误作为帧地址或执行流上下文 |
| 已处理 epc | 用已知 16 位和 32 位断点编码分别触发，替身将 `tf->epc` 指向各自后继标记；sret 必须到指定位置且仅触发一次，不重复陷入，也不跳过额外指令 |
| 帧的最终值 | 在受控合法条件下更新一个 GPR 槽，确认返回后采用新值；用构造帧检查 sp 来自槽 2，而非只依赖 F+288 的初始关系 |
| 状态返回 | 对 Trap 前 SIE=0 和 SIE=1 分别取样，检查保存的 SPIE、sret 前 status/sepc、返回 S 态后 SIE=原 SIE、SPIE=1、SPP=0。用指令级断点或跟踪取样，避免调试工具触发的额外 Trap 污染结论 |
| 栈和重复进入 | 帧及调用栈边界设哨兵，连续两次受控 Trap 后检查帧外哨兵未变、栈未漂移、sscratch=0、返回位置正确；不据此宣称支持任意嵌套异常 |

使用标准 ABI 的 C 替身时，检查的是调用指令落入 C 的瞬间对齐；C 函数序言可正常向低地址分配自己的栈。帧仍应保持有效，不能误把序言之后的 C 局部栈底当成原 F。

### C. 两个模块完成后的课程集成验收

在完整源码可以构建后，于 `code/` 运行 `make clean`、`make`、`make grade`；调试可用 `make debug` 配合 `gdb-multiarch -ex 'file bin/kernel' -ex 'set arch riscv:rv64' -ex 'target remote localhost:1234'`。课程命令均保留原样；实际能否启动和通过需另行记录。

课程 `test_lab3_context.S` 以 `ebreak` 检查多数 t/s/a 寄存器；`test_lab3.c` 还检查非法指令返回和持续时钟。评分第一项也依赖模块二正确处理断点并输出 `Breakpoint at`，所以不能仅凭该项失败断定模块一错误。现有寄存器测试未完整覆盖 ra/sp/gp/tp/a0、所有 status/epc 更新边界或任意 GPR 修改，必须结合 B 中的定点检查，不能把课程 PASS 当作完整上下文证明。

## 9. 基线阶段结论与集成依赖

基线提交 `3335e0e` 时，已完成真实代码接口、布局和验收规格整理，尚未填写两个模块；当时布局断言和迁移文件编译通过，完整构建因模块二占位缺失函数而退出2。随后本轮仅实现模块一，受控副本动态验证通过；正式模块二仍为空，正式完整构建仍受阻，尚无课程评分结论。

模块二边界接口：真实 `clock.h` 提供 `volatile size_t ticks` 与 `clock_set_next_event()`；`clock.c` 通过 `sbi_set_timer(get_cycles()+100000)` 设定下一次事件并设置 STIE；`sbi.h/c` 提供 `void sbi_shutdown(void)`。`TICK_NUM=100` 只在 `trap.c` 定义，真实框架没有 `SHUTDOWN_AFTER` 定义。它们用于说明集成依赖，不属于模块一要实现的行为，也不在本次补齐。
