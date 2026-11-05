# Lab3 模块二：中断和异常处理

负责人：田茂炜，学号2412784。基线为原仓库 lab3 的
`fbe7f5c5847a4d3b182168a615bf2e68c3f6e887`。

## 规格与实现

正式功能改动限于 `code/kern/trap/trap.c` 的 function2 区域，增加 SBI 声明引用。
不修改模块一汇编、trapframe、trap_dispatch、trap、课程注入器与评分脚本。

`interrupt_handler(struct trapframe *)`、`exception_handler(struct trapframe *)`
保持全局函数接口，供真实 Trap 和课程测试共同调用。

中断由 scause 最高位识别；处理器清除最高位后按 IRQ 编号分类。软件、时钟、外部中断
分别输出类型，未知中断打印原因号及完整 Trap frame。S 软件中断清除 sip.SSIP，避免
同一软件中断持续挂起；本实验其他外部中断只分类，不实现 PLIC 设备确认。

S 时钟中断先调用 `clock_set_next_event()`，然后每次递增 ticks。在 TICK_NUM=100
的整数倍调用现有 print_ticks() 输出 `100 ticks`。不调整 tf->epc，因此从被打断位置
继续执行。SHUTDOWN_AFTER 表示累计输出组数；原框架没有提供该常量，本实现使用
可编译覆盖的默认值 10，即1000次时钟中断后关机。这是明确的实现选择，不能声称
指导书给出了数字10；若教师另有规定，应通过 DEFS=-DSHUTDOWN_AFTER=实际值配置。
不要设为1或2后据此判断课程200 ticks测试失败，阈值应晚于测试完成。

非法指令和断点异常先检查 EPC 为非零、半字对齐的 S 态地址，然后读取前16位编码。
低2位不全为1时长度为2字节；低2位为11且低5位不全为1时长度为4字节。
打印 `Illegal instruction at ...` 或 `Breakpoint at ...` 后按长度更新 tf->epc。
更长编码拒绝恢复并诊断，避免猜测返回位置。未知异常仅打印 Trap frame，不修改 EPC，
与课程兜底接口一致；真实不可恢复异常若直接返回仍可能重入，不能当作通用恢复器。

该读取要求 EPC 对应代码在内核中可读。当前实验的 S 态映射满足测试前提，但没有
安全访存恢复机制，也不支持任意用户地址、仅执行映射或未映射故障代码的通用解码。

## 完整 Trap 流程

CPU 保存 sepc/scause 等状态并进入 stvec；模块一 SAVE_ALL 在栈上构造288字节帧，
令 a0=sp 后调用 trap。trap_dispatch 根据 cause 符号位调用模块二。处理器修改的是
内存帧中的返回状态，随后模块一从帧恢复 sstatus/sepc、通用寄存器、最后恢复 sp，
通过 sret 返回。模块二不能擅自调整帧布局或使用普通 C return 替代 sret。

## 验证状态（2026-11-05）

- 实际通过：WSL 主机 GCC 编译实际 trap.c 的处理函数测试，使用硬件接口桩。
  原始输出见 [host-check.log](logs/module2/host-check.log)。正式构建尝试的缺少工具错误
  保存在 [build-attempt.log](logs/module2/build-attempt.log)。
- 覆盖：1000次时钟续设与计数、每100次输出、10组输出后调用关机；中断过程中帧不变；
  软件中断确认；各已知中断分类与未知中断兜底；16/32位 EPC 前进；帧其他字段保持；
  未知异常不改变帧；长指令拒绝恢复。
- Ubuntu 用户实测：完整构建退出0，三组 `-kernel` 行为检查标记全部通过，GDB返回检查退出0，
  正常版输出10组后 QEMU退出0；原样课程评分退出2。后续独立兼容评分副本得到100/100。
  详见 [用户回传的验证摘要](logs/module2/ubuntu-reported.md)。
- 环境限制：本机 WSL 只有主机 GCC，安装 RISC-V/QEMU/GDB 时软件源下载重试，尚未获得
  可运行的完整目标工具链。主机接口桩测试不验证 CSR 指令、汇编或 sret。
- 原样课程评分不通过与兼容评分100/100分别记录；完整Ubuntu原始日志仍在用户虚拟机，
  本工作区仅保存用户在聊天中回传的结果摘要，不将摘要冒充原始日志文件。

## 复现

在 Ubuntu、仓库根运行主机逻辑测试（需 gcc、python3）：

```sh
python3 report/tests/module2/host_check.py
```

在已有课程工具链的 Ubuntu 运行完整验证，每次指定新的输出目录：

```sh
python3 report/tests/module2/validate.py --out /tmp/lab3-module2-validation --compat
```

脚本先在副本执行正式构建和未经改动的 `make grade`，再在独立副本使用原注入器和
`-kernel` 补充验证三组课程行为及正常版10组输出后退出。所有阶段记录退出码和命令。
补充验证不修改评分脚本、不冒充原样评分；若原样评分不通过，脚本整体仍返回失败。
GDB 补充检查在课程 context 副本中停到 exception_handler、__trapret 以及恢复后的 EPC，
核对 a0 指向的帧、EPC 增量和 sret 后的 sp。用户回传 gdb-return 退出0。
当前原 Makefile/grade 使用 generic loader，Ubuntu6.2/OpenSBI0.9可能再次出现
Next Address=0；兼容结果与正式结果分别记录，提交前需与教师确认运行兼容设置。

## 实现前后的问题

实现前正式编译因两个处理器缺失而报隐式声明错误；现在已在调用前提供全局定义。
常量 SHUTDOWN_AFTER 缺失通过显式默认值和覆盖接口解决，并记录假设。
恢复位置不能统一加4；实现根据编码区分压缩指令与标准指令，避免跳过正常指令。

## 与操作系统原理的关系

中断是异步事件，通常不改变 EPC；异常由当前指令触发，本实验选择跳过指定故障指令。
周期定时器必须持续续设，为后续抢占调度提供时间事件。上下文的完整保存和恢复是
中断返回及后续任务切换的基础，但本实验尚未实现调度、设备中断服务、用户态隔离、
缺页恢复或并发锁机制。

## 后续资料补齐

- 将虚拟机中的原始构建、QEMU、评分及GDB日志追加归档；本提交已有可追溯回传摘要。
- 模块二 function2 标记已填写2412784，保留lab3注释；模块一学号由原作者填写。
- 确认教师要求的关机组数；核查正式评分启动兼容性。
- 交叉阅读模块一与二，再进行 Git 提交和推送。
