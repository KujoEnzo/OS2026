# Lab3 模块一实现与验证记录

基准提交：`3335e0e6e1663e33cba41c3d5645f6b9be0416f0`。本次实现与验收基于 [基线记录](lab3-baseline.md)、[模块一规格](module1-spec.md)；初始 `lab3` 工作区干净。所有操作均在 WSL `CosyVoice_Env` 的 `/home/kujoenzo/OS2026`，用户为 `kujoenzo`。目录与请求相同，正式实现路径就是 `code/kern/trap/trapentry.S`。

本次只完成模块一，在本地提交；未实现正式模块二，未推送远程。先于实现保存的 [验收断言与执行清单](module1-assertions.md) 覆盖本次检查条件。原始基线报告与来源清单保持历史含义，不将其基线哈希改写为当前实现哈希。

## 1. 正式实现与不变量

正式 `code/` 只有 `kern/trap/trapentry.S` 有变化，共增加 42 行，全部位于两处 `#lab3 function1 YOUR CODE` 后。其他 55 个文件与基准提交相同；`SAVE_ALL`、`struct trapframe`、`trap()` 签名和函数体、既有 `__trapret`/`sret` 框架、Makefile、评分工具均未改变。

- `RESTORE_ALL` 从槽32/33加载处理后的 `status/epc`，用 s0/s1 写入 `sstatus/sepc`；随后从对应槽恢复 x1、x3…x31。临时使用的 s0/s1 最终从原 GPR 槽恢复，x0 保持硬连线零。
- 保持 `sp` 为帧基址直到所有其他读取结束，最后从槽2加载 x2；不以简单加288替代旧 sp 的保存值。
- `__alltraps` 在现有 `SAVE_ALL` 后用 `a0=sp` 传递 `struct trapframe *`，正常调用 `trap`，C 返回后自然进入已有 `__trapret`。288 是16的倍数，不引入额外栈调整。
- 不在模块一推进 epc、不强制设置 SPP、不无条件打开 SIE、不写回诊断 CSR `scause/stval`。最终通过已有 `sret` 按帧中状态返回。

正式实现 SHA-256：

```text
d37c6e50bd3fa1ea0db7374336472be5ee5d57905d04246d84a7368019bb5caf
```

完整性检查程序为 [formal_check.py](tests/module1/formal_check.py)，结果为 [integrity.json](logs/module1/formal/integrity.json)。它逐文件对照基准提交，并把两个新增实现区折回占位，要求得到与基准完全一致的原文；同时检查正式源码文件集合中没有测试源文件。

## 2. 先失败，再实现

测试与断言先写，正式实现后写。初始空占位版本的 [static-gate.log](logs/module1/red/static-gate.log) 缺少 CSR 写回、sp 槽恢复和调用 `trap`；测试副本可编译，但 GDB 检查退出1，见 [red-gdb.log](logs/module1/red/red-gdb.log)。原因是入口没有调用 `trap`，启用 `--gc-sections` 后 `trap` 符号被移除。该结果确认检查不会把“空宏能汇编/内核能启动”误判为模块一完成。

随后仅填入两处正式实现，未修改正式模块二以绕过编译。

## 3. 实际工具与正式构建

环境为 Ubuntu 22.04.1 / WSL2，GCC `riscv64-unknown-elf-gcc` 10.2.0，GNU Make 4.3，QEMU 6.2.0，`gdb-multiarch` 12.1。编译目标沿用课程默认 RV64IMAFDC/LP64D。工具输出见 [environment.log](logs/module1/formal/environment.log)。未使用缺失的 `riscv64-unknown-elf-gdb`，未改框架调试目标。

正式源码实际执行：

```sh
cd /home/kujoenzo/OS2026
python3 report/tests/module1/formal_check.py --repo . --out report/logs/module1/formal
```

该脚本运行的命令与结果：

| 检查 | 实际结果 |
| --- | --- |
| `make clean` | 退出0 |
| `make -k -j2` | 退出2，仅 `trap.c` 中 `interrupt_handler`、`exception_handler` 隐式声明报错；正式内核/镜像未产生 |
| `make -B obj/kern/trap/trapentry.o` | 退出0 |
| `riscv64-unknown-elf-objdump -dr obj/kern/trap/trapentry.o` | 退出0；已检查调用重定位、CSR 偏移和恢复顺序 |
| 既有 `trapframe-layout-check.c` | 43项断言通过，退出0 |

反汇编见 [trapentry-disassembly.log](logs/module1/formal/trapentry-disassembly.log)：参数传递为 `mv a0,sp`，有到 `trap` 的 `R_RISCV_CALL`；恢复从 `256(sp)`、`264(sp)` 读取状态和 PC，写回后再恢复 GPR；最后两条指令为 `ld sp,16(sp)` 和 `sret`。对象文件中调用地址尚未链接，测试副本的已链接反汇编另行保存在动态日志中。

完整构建失败原文见 [full-build.log](logs/module1/formal/full-build.log)。没有为使它通过而添加正式处理器、禁用 `-Werror` 或修改评分脚本。

## 4. 测试副本的隔离和复现

运行：

```sh
cd /home/kujoenzo/OS2026
python3 report/tests/module1/run.py --repo . --out report/logs/module1/dynamic
```

脚本在 `/tmp/os2026-module1-<run>-*/code` 创建副本，排除已有 obj/bin；每次均干净构建。正式代码不被注入。测试源码、GDB 断言和驱动均保存在 [tests/module1/](tests/module1/run.py)：

- `module1_harness.S`：专用对齐栈、明确编码的断点、已知寄存器值和返回后快照。
- `module1_test.c/h`：帧、返回快照、计数和边界哨兵检查。
- `gdb_check.py`：在真实 `trap` 首地址、`__trapret`、`sret` 和返回目标首地址核对寄存器、PC、CSR、特权级。
- `run.py`：副本注入、编译、QEMU/GDB 启停、日志和受控反例。

相对正式 `code/`，正确测试副本只改 `kern/init/init.c` 和 `kern/trap/trap.c`，并增加三个测试源文件到 `kern/init/`。副本中的最小 `exception_handler()` 只校验已知断点并设置预定 epc/测试帧；`interrupt_handler()` 直接报告意外中断，不实现正式模块二功能。原 `trap()`、`trap_dispatch()`、`idt_init()`、启动汇编和链接脚本均沿用真实课程代码。

测试入口插在第二次 `idt_init()` 后、`clock_init()` 前；汇编将 `sie` 清零，因而 SIE=1 的返回检查不会被待处理时钟打断。该限制意味着测试验证 SIE/SPIE/SPP 的返回语义，**不验证时钟中断处理或重设事件**。

QEMU 命令形如：

```sh
qemu-system-riscv64 -machine virt -nographic -bios default \
  -kernel bin/ucore.img -monitor none -no-reboot \
  -S -gdb tcp:127.0.0.1:<自动分配端口>
```

使用 `-kernel` 让本环境 OpenSBI 得到实际入口；课程 Makefile 中的 `-device loader` 命令没有被改动。本次没有运行原样 `make qemu/debug/grade`，不能据此声称这些命令已通过。实际测试保留课程 Sv39 初始化、DTB、物理内存初始化和分配器检查；串口日志中 `check_alloc_page() succeeded!` 只证明这些启动步骤执行，本模块的通过依据是后述逐项检查。

GDB 通过 `gdb-multiarch -q -nx -batch -ex 'set $module1_port=<端口>' -x <gdb_check.py>` 连接；脚本限制超时，结束后回收自己启动的 QEMU。日志中的 `QEMU_STOPPED_BY_TEST_DRIVER` 是驱动在检查完成后停止等待循环，不是系统实现了关机。

## 5. 动态用例与观测

GPR 期望值以 `0x1122334400000100 + x编号` 为主，非零高32位用于识别错误的窄恢复；x0期望0，sp期望合法测试栈顶，gp/tp保持进入测试前的真实值并比较。既比较帧内保存值，也比较 sret 前真实寄存器和返回后汇编快照。

| 用例 | 断点与处理前状态 | 处理器的受控修改 | 通过条件 |
| --- | --- | --- | --- |
| case0 | `.4byte 0x00100073`，4字节，SIE=0 | epc 指向紧随断点的后继标号 | GPR精确恢复，S态返回，SIE/SPIE/SPP掩码为 `0x020` |
| case1 | `.2byte 0x9002`，2字节，SIE=1 | epc 指向对应后继标号 | 同上，返回状态掩码为 `0x022` |
| case2 | 4字节断点，SIE=0 | epc更新；槽2改为另一合法栈顶；a5改为 `0xfedcba9876543210`；SPIE置1 | 使用全部处理后值，返回状态掩码为 `0x022`，排除只恢复原CSR/简单弹栈 |

每组在 `trap` 第一条机器指令之前确认 `a0=sp=F`、F是原栈顶减288、16字节对齐，32个GPR槽、epc、cause和状态与预期相同；C 返回到 `__trapret` 时再次确认 sp仍是F。临时 C 处理器还显式覆盖所有 caller-saved 参数/临时寄存器。

在 sret 前确认 x0…x31、处理后的 sepc/status 和 sscratch；在返回目标**第一条指令执行前**确认 PC 为新epc、SIE=帧SPIE、SPIE=1、SPP=0、`priv=1`（S态）、sscratch=0，事件数没有额外增加。返回后还执行独立汇编快照与C比较，检查栈上下界哨兵和计数。快照为保留 t6 而暂借并清零 sscratch，所以C侧槽34只检查采样辅助步骤完成清理；真实返回的 scratch 值由GDB直接采样，不把C侧清零后的值当独立证明。

本次正确用例的代表性实测地址：

```text
trap a0 = sp = F           0xffffffffc0208f00
初始 / case0、case1 sp    0xffffffffc0209020
case2 处理后 sp           0xffffffffc020b030
case0 新 epc              0xffffffffc020023e
case1 新 epc              0xffffffffc020049e
case2 新 epc              0xffffffffc02006fc
```

地址是这次测试副本的链接结果；重跑应按符号解析，不在实现中硬编码。完整观测见 [positive-gdb.log](logs/module1/dynamic/positive-gdb.log)、[positive-qemu.log](logs/module1/dynamic/positive-qemu.log)。三个用例均通过，GDB退出0，事件数=3，后继点检查数=3，无失败。

### 调试采样问题与最小调整

首轮 GDB 使用 `stepi` 越过 sret，实际下一次停点已到后续 Trap，不能作为“返回后第一条指令”的样本；串口记录和后继点检查显示被打断状态已正确返回。该轮失败记录保留于 [diagnostic-stepi/positive-gdb.log](logs/module1/diagnostic-stepi/positive-gdb.log) 和同目录串口日志。

这不是据后续输出忽略失败：先确认 sret 前寄存器/CSR均通过，定位到采样停点不符，再将测试改为返回目标处的临时断点。正式汇编未因此改动。最终日志明确记录在返回目标第一条指令前采集的 PC、状态、特权级和事件数。此调整记录为验收操作的变更，不宣称已成功单步 sret，也不将该工具组合的观察推广为所有GDB版本的行为。

## 6. 受控反例与复测

仅在 `mutant-sp` 副本中，将一条 `LOAD x2, 2*REGBYTES(sp)` 替换为 `addi sp,sp,36*REGBYTES`。它能编译，且前两个“不修改槽2”的用例能通过，但第三组在 sret 前被拒绝：

```text
MODULE1_GDB_FAIL: case2 restored x2:
got=0xffffffffc0209020 expected=0xffffffffc020b030
EXIT=1
```

原始记录见 [mutant-sp-gdb.log](logs/module1/dynamic/mutant-sp-gdb.log)。说明检查能够区分“只在简单情况下能返回”和“真正采用帧内sp恢复”的实现。

然后重新创建未带该变异的副本复测，三组全部通过，GDB退出0；见 [recheck-gdb.log](logs/module1/dynamic/recheck-gdb.log)、[recheck-qemu.log](logs/module1/dynamic/recheck-qemu.log)。汇总 [results.json](logs/module1/dynamic/results.json) 记录正确版0、反例1、复测0，且正式源码在测试前后哈希一致。每组副本差异也保存于对应 `*-copy-edits.json`，变异版只额外改变副本的 trapentry.S。

如需复现实现前的失败，先在任意临时目录导出基准提交的 code，再使用当前保存的测试驱动；不要回退当前分支：

```sh
tmp=$(mktemp -d /tmp/os2026-module1-red-repro-XXXXXX)
git archive 3335e0e6e1663e33cba41c3d5645f6b9be0416f0 code | tar -x -C "$tmp"
python3 report/tests/module1/run.py --repo "$tmp" --out /tmp/module1-red-logs --red
```

## 7. 正式交接与未验证范围

正式 `code/` 没有临时处理器、测试入口、测试栈或变异；测试仅作为 `report/tests/module1/` 下的复现材料保留。`trap.c` 的模块二占位仍与基准逐字节相同。最终差异/完整性记录见 formal 日志；正式源码、手写测试和文档的空白检查通过。全量暂存区检查会将原始QEMU串口日志的CRLF行尾标为空白问题，这些日志保留原始字节，没有为格式检查改写实测输出。

已验证模块一在本实验 S 态、有效内核栈和已知断点的受控条件下完成保存→C调用→采用处理后帧恢复→sret闭环。仍需完整联调：

1. 正式模块二的中断分类、时钟重设/计数/关机、非法指令与断点策略完成后，重新进行正式完整构建、启动和课程评分。
2. 尚未验证真实异步时钟/外部中断、任意异常恢复、U态栈切换、递归异常、浮点/向量上下文；本模块及原SAVE_ALL没有增加这些协议。
3. 原样 Makefile 的 `-device loader` 启动、硬编码调试器名的适配仍属环境联调；本次测试的 `-kernel` 与 `gdb-multiarch` 命令已明确记录。
4. 课程要求提及但框架未定义的 `SHUTDOWN_AFTER` 仍由模块二后续确认，本次没有指定值或实现行为。
