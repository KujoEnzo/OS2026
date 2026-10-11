# 操作系统实验报告

## 实验基本信息

| 项目 | 内容 |
|---|---|
| **实验名称** | Lab 1：比麻雀更小的麻雀（最小可执行内核） |
| **小组成员** | 郝煦享、田茂炜 |
| **完成日期** | 2026/10/10 |

### 小组分工

练习与模块分工：

| 成员 | 负责的练习/模块 |
|---|---|
| 郝煦享 | 全部模块 |
| 田茂炜 | 全部模块 |

实验报告分工：两人共同完成环境配置、启动调试、源码分析、练习作答与结果复核。

---

## 一、实验目的

本实验的主要目的是：

1. 了解 Qemu 模拟器的启动流程。
2. 了解程序内存布局和编译流程相关知识。
3. 确保内核能够正确对接到 Qemu 模拟器。

---

## 二、实验环境

### AI 工具记录

| 成员 | AI 编程工具 | 底层模型 | 备注 |
|---|---|---|---|
| 郝煦享 | Codex | GPT-6Astra | 无 |
| 田茂炜 | Codex | GPT-6.1Sol | 无 |

### 系统与开发工具（一）：郝煦享

| 项目 | 本次记录 |
|---|---|
| Windows | Windows 11 23H2，22631.6199 |
| Linux 环境 | WSL 2；Ubuntu 22.04.1 LTS；x86_64 |
| Linux 用户 | kujoenzo |
| 编辑方式 | Windows VS Code，通过 SSH 连接 WSL |
| GCC | riscv64-unknown-elf-gcc 10.2.0 |
| 编译器目标 | riscv64-unknown-elf |
| Binutils | 2.35.1 |
| Make | GNU Make 4.3 |
| QEMU | qemu-system-riscv64 6.2.0 |
| GDB | gdb-multiarch 12.1 |
| 运行输出中的固件 | OpenSBI v0.9；Runtime SBI Version 0.2 |
| 源码压缩包 | `F:\OS2026\lab1.zip` |
| WSL 源码目录 | `/home/kujoenzo/os2026/code/lab1` |
| 编译产物 | `bin/kernel`；`bin/ucore.img` |
| 内核 ELF 信息 | ELF64；RISC-V；入口地址 `0x80200000` |
| 练习 2 原始日志 | [lab1-ex2-gdb.txt](lab1-ex2-gdb.txt) |

### 系统与开发工具（二）：田茂炜

| 项目 | 本次记录 |
|---|---|
| 宿主操作系统 | Windows |
| Linux 环境 | Ubuntu 虚拟机 `tmw-virtual-machine` |
| Linux 用户 | `tmw` |
| 编辑方式 | Windows VS Code，通过 Remote-SSH 连接虚拟机 |
| GCC | riscv64-unknown-elf-gcc 10.2.0 |
| 编译器目标 | `riscv64-unknown-elf` |
| Make | GNU Make 4.3 |
| QEMU | qemu-system-riscv64 6.2.0（Debian `1:6.2+dfsg-2ubuntu6.31`） |
| GDB | gdb-multiarch 12.1 |
| 运行输出中的固件 | OpenSBI v0.9；Runtime SBI Version 0.2 |
| 源码来源 | Git 仓库 `https://github.com/KujoEnzo/OS2026` 的 `lab1` 分支 |
| Linux 源码目录 | `/home/tmw/os-lab/OS2026` |
| 编译产物 | `bin/kernel`；`bin/ucore.img` |
| 内核 ELF 信息 | ELF64；RISC-V；入口地址 `0x80200000` |
| 调试日志 | [lab1-ex2-gdb.txt](lab1-ex2-gdb.txt) |

---

## 三、实验整体逻辑分析

### 3.1 本章节的逻辑主线

本实验解决的核心问题是：如何让一个最小内核在 RISC-V 模拟平台上获得控制权，并具备执行 C 代码和输出信息的基本条件。

编译器先把 C 与汇编源文件转换成目标文件，链接器根据链接脚本安排各段地址并确定入口，随后生成供运行使用的内核镜像。QEMU 建立模拟机器并装入固件与内核；处理器从复位代码开始执行，先进入 OpenSBI，再由 OpenSBI 将控制权交给位于 `0x80200000` 的内核入口。内核入口设置栈指针后跳转到 `kern_init`，清零 BSS 区域、打印启动信息，最后停留在循环中。

这一过程包含两条相互配合的主线：一条是“源码—目标文件—ELF—原始镜像”的构建过程；另一条是“复位代码—OpenSBI—内核汇编入口—C 初始化函数”的执行过程。链接地址、镜像加载地址和实际跳转地址必须相互匹配，才能把构建结果与运行过程连接起来。

### 3.2 功能的逐步实现

1. **确认交叉编译与调试环境。** 宿主机为 x86_64，内核目标为 RISC-V，因此需要目标为 `riscv64-unknown-elf` 的交叉编译器，以及支持 RISC-V 的 QEMU 和 GDB。
2. **构建并确定内存布局。** `Makefile` 完成编译、链接和镜像转换；`tools/kernel.ld` 将入口设为 `kern_entry`，将内核起始地址设为 `0x80200000`。布局是分析入口指令、栈与 BSS 地址的依据。
3. **使固件正确交接到内核。** 本次环境调整了 QEMU 启动参数。修改后 OpenSBI 输出的 `Next Address` 为 `0x80200000`，并出现内核启动字符串。
4. **建立 C 代码运行的基础。** `kern_entry` 设置 `sp` 并跳转到 `kern_init`；后者清零 BSS，保证未显式初始化的静态存储对象具有正确初值。
5. **验证最小输出路径。** `cprintf` 经格式化与控制台模块，将字符交给 SBI 服务，最终显示在终端。输出能够证明内核初始化已执行到打印位置。
6. **用 GDB 核对控制流与状态变化。** 从复位 PC 开始单步，观察进入固件，再设置内核入口断点，并检查入口处的栈指针变化。

上述顺序首先解决程序构建和启动问题，再分析内核如何建立运行条件。本次代码修改集中在 QEMU 启动参数。

---

## 四、实验内容与实现

### 功能模块：最小内核启动、初始化与控制台输出

**负责人：** 郝煦享、田茂炜（按小组分工，均涉及全部模块）。

#### 模块功能描述

**需要实现/修改的函数与涉及的接口：**

本次修改的是 `Makefile` 中 `qemu:` 与 `debug:` 的运行参数。源码分析涉及的主要接口如下：

```c
int kern_init(void) __attribute__((noreturn));
int cprintf(const char *fmt, ...);
int vcprintf(const char *fmt, va_list ap);
void cons_putc(int c);
void sbi_console_putchar(unsigned char ch);
void *memset(void *s, int c, size_t n);
```

汇编入口为 `kern_entry`；格式化核心为 `vprintfmt`，通过字符输出回调把格式化结果交给控制台。

**功能说明：**

该模块完成最小内核的启动与可见输出。汇编入口负责设置栈，C 初始化函数负责清零 BSS 并调用格式化输出函数。输出链路为：

```text
kern_init → cprintf → vcprintf → vprintfmt
         → cputch → cons_putc → sbi_console_putchar → sbi_call/ecall
         → OpenSBI 的控制台服务 → 模拟终端
```

本次内核尚不具备完整的线程、用户进程或文件系统；启动信息输出后进入 `while (1)`。持续停留在 QEMU 中符合该最小程序的行为。

**核心函数与功能模块分析：**

| 源码文件 | 核心函数 / 模块 | 输入、输出或涉及的数据 | 核心作用与实现理解 | 与其他模块的联系 |
|---|---|---|---|---|
| `Makefile` | 构建规则，`qemu`、`debug` 目标 | 源码、目标文件、ELF、镜像、运行参数 | 组织交叉编译；链接生成 `bin/kernel`；用 objcopy 生成 `bin/ucore.img`；组织运行与调试 | 采用链接脚本，向 QEMU 提供镜像；调试时配合 GDB |
| `tools/kernel.ld` | `ENTRY(kern_entry)`、段布局、`edata` 与 `end` | 起始地址 `0x80200000`，text、rodata、data、bss | 指定程序入口和各段位置；以符号标记初始化数据结束与内核结束 | 入口地址须与启动交接匹配；边界符号供 `kern_init` 清零使用 |
| `kern/init/entry.S` | `kern_entry`、`bootstack`、`bootstacktop` | `sp`，大小为 8192 字节的启动栈 | 设置内核栈后跳转到 C 初始化入口；栈按页对齐 | 先提供栈，再进入 `kern_init` |
| `kern/init/init.c` | `kern_init` | `edata`、`end`、启动字符串 | 调用 `memset` 清零 BSS；调用 `cprintf` 输出；进入无限循环 | 使用链接符号、字符串库和输出库 |
| `libs/sbi.c`、`libs/sbi.h` | `sbi_call`、`sbi_console_putchar` | 服务号、参数寄存器、返回值 | 本份源码的字符输出使用旧式 SBI 接口；把服务号放入 `a7`、参数放入 `a0`～`a2`，执行 `ecall` | 将内核请求交给 OpenSBI，而非自行直接控制 UART |
| `kern/driver/console.c` | `cons_putc` | 一个字符 | 将字符转换为 `unsigned char` 后调用 SBI 输出接口 | 对上提供控制台接口，对下使用 SBI；本次未验证完整输入与中断功能 |
| `kern/libs/stdio.c` | `cprintf`、`vcprintf`、`cputch` | 格式字符串、可变参数、输出计数 | 管理可变参数列表；调用格式化核心；经回调输出并累计字符数 | 把格式化处理与具体字符输出连接起来 |
| `libs/printfmt.c` | `vprintfmt` | 格式字符串、参数列表、字符回调 | 解释格式说明并逐字符输出，避免格式化代码依赖具体设备 | 通过 `cputch` 回调接入控制台 |
| `libs/string.c` | `memset` | 起始地址、字节值、长度 | 将指定范围设为给定字节值；本实验用于置零 | 为 `kern_init` 建立静态数据的初始状态 |

#### 最终提示词

最终任务描述如下：

````markdown
请依据 OS2026 Lab1 实验手册检查当前 lab1 分支：
1. 核对最小 RISC-V 内核的链接地址、入口、启动栈、BSS 清零和 SBI 输出链路；
2. 检查 QEMU 是否把控制权交给 0x80200000，并说明必要的 Makefile 修正；
3. 以 Markdown 完善实验报告，区分实测事实与机制分析；
4. 给出可复现的 make、QEMU 和 GDB 验证命令与验证范围。
````

#### 实现迭代过程

本次完成了 **1 次 QEMU 启动配置修正**。

##### 第一次迭代：修正固件到内核的启动交接

**遇到的问题与定位依据：**

`make` 已生成内核 ELF 与原始镜像，但 `make qemu` 只显示 OpenSBI 信息，未显示 `(THU.CST) os is loading ...`。当时 OpenSBI 的 `Domain0 Next Address` 为 `0x0`。这说明编译成功尚不足以证明执行已进入内核，需检查固件与内核之间的启动交接。

原配置使用 generic loader 将镜像放到指定地址。仅把镜像预先放入内存，并不能保证本次默认固件同时获得正确的下一阶段入口。修改后的入口信息和 GDB 断点结果支持这一定位。

**问题解决策略：**

在 `qemu:` 和 `debug:` 两处将：

```makefile
-device loader,file=$(UCOREIMG),addr=0x80200000
```

替换为：

```makefile
-kernel $(UCOREIMG)
```

`debug:` 对应行末尾保留续行符 `\`，下一行保留 `-s -S`。使用 `-kernel` 让 QEMU 的内核启动路径处理镜像及下一阶段交接信息。QEMU 官方文档介绍了 `virt` 平台通过 `-kernel` 配合默认 OpenSBI 启动的方式；本次版本是否适配则以实际运行结果为准。[QEMU virt 启动选项](https://www.qemu.org/docs/master/system/riscv/virt.html)

本次调整限于实验的运行和调试入口，未改变内核函数或链接地址。与课程原始命令不同之处保留在报告中，以便复现。

**最终结果：**

| 配置 | OpenSBI 的 Next Address | 内核启动字符串 |
|---|---|---|
| 修改前 | `0x0000000000000000` | 未出现 |
| 修改后 | `0x0000000080200000` | 出现 |

修改后 GDB 能命中 `0x80200000` 的 `kern_entry` 断点，进一步验证了实际控制流已到达内核。



---

### 练习 1：理解内核启动中的程序入口操作

**负责人：** 郝煦享、田茂炜。

#### 源码与实测数据

```asm
kern_entry:
    la sp, bootstacktop
    tail kern_init
```

本次反汇编：

```text
0x80200000 <kern_entry>:   auipc sp,0x3
0x80200004 <kern_entry+4>: mv    sp,sp
0x80200008 <kern_entry+8>: j     0x8020000a <kern_init>
```

源码中 `KSTACKPAGE = 2`，`PGSIZE = 4096`，因此启动栈保留 `8192` 字节；`.align PGSHIFT` 中 `PGSHIFT = 12`，使栈区域按 4096 字节对齐。本次 `&bootstacktop = 0x80203000`。

以下为终端显示的练习 1 数据，与附录 A 的练习 2 日志分别记录：

| 观察时点 | PC | SP |
|---|---|---|
| `kern_entry` 入口断点 | `0x80200000` | `0x80017ee0` |
| 在入口处执行 `si 2` 后 | `0x80200008` | `0x80203000` |
| 再执行 `si` 后 | `0x8020000a <kern_init>` | `0x80203000` |

#### `la sp, bootstacktop`

该指令将启动栈顶的地址写入栈指针 `sp`。内核进入时继承的 `sp = 0x80017ee0` 属于此前启动阶段的状态；内核需要使用自身预留的栈，供 C 代码保存局部变量、寄存器和调用信息。RISC-V 栈通常向低地址增长，因此以栈区域末端 `bootstacktop` 作为初始栈指针。

`la` 是伪指令，实际机器指令由汇编与链接结果决定。本次首先执行 `auipc sp,0x3`，将当前 PC `0x80200000` 与 `0x3000` 相加，得到 `0x80203000`；随后显示为 `mv sp,sp`，表示本次地址的低位修正为零。执行这两条后，PC 为 `0x80200008`，SP 与 `&bootstacktop` 相等，说明内核栈设置符合预期。

#### `tail kern_init`

该伪指令将执行转移到 `kern_init`，采用尾调用方式，不为 `kern_entry` 建立新的返回地址。`kern_init` 被声明为不返回，并在输出后进入无限循环，因此入口无需安排返回后继续执行的路径。

本次链接后的指令为 `j 0x8020000a`，即不写入返回地址寄存器的跳转。单步后 PC 从 `0x80200008` 变为 `0x8020000a <kern_init>`，SP 保持 `0x80203000`。这说明控制权进入了 C 初始化函数，并继续使用刚建立的内核栈。

GDB 将地址 `0x80203000` 注记为 `<SBI_CONSOLE_PUTCHAR>`，不代表 SP 指向的是服务号的值。应以数值地址及 `&bootstacktop` 对照判断；符号注记可能来自同一地址附近的其他符号。

---

### 练习 2：使用 GDB 验证启动流程

**负责人：** 郝煦享、田茂炜。

#### 调试启动与观察方法

终端 1 启动暂停等待调试器的 QEMU：

```bash
cd ~/os2026/code/lab1
make debug
```

终端 2 加载带符号的 ELF 文件并连接 QEMU：

```bash
cd ~/os2026/code/lab1
gdb-multiarch -q bin/kernel
```

已执行的 GDB 命令包括：

```gdb
target remote 127.0.0.1:1234
info registers pc
x/6i $pc
break *0x80200000
continue
```

`debug` 保留 `-s -S`：`-s` 提供默认 1234 端口的调试服务，`-S` 使 CPU 在启动执行前暂停。GDB 加载 `bin/kernel` 用于内核符号解析；复位 ROM 与 OpenSBI 不属于该 ELF，因此早期显示 `??` 不等于执行错误。

观察分为三个阶段：检查复位 PC 和指令；单步跨过 ROM 最后一条跳转，确认进入 OpenSBI；在内核入口设置断点，检查栈指针和控制流变化。

复现命令如下：

```gdb
info registers pc
x/6i $pc
x/gx 0x1018
si 5
info registers pc t0 a0 a1 a2
si
info registers pc
x/6i $pc
break *0x80200000
continue
info registers pc sp
si
info registers pc sp
```

#### A. 初始 PC 与附近指令

日志记录初始 `pc = 0x1000`。本次 QEMU virt 的复位代码从该地址执行。

| 地址 | 指令 | 操作与功能 |
|---|---|---|
| `0x1000` | `auipc t0,0x0` | 将当前 PC 写入 `t0`，得到 `0x1000`，作为后续相对寻址的基址 |
| `0x1004` | `addi a2,t0,40` | 得到 `a2 = 0x1028`，向固件传递动态启动信息结构的地址 |
| `0x1008` | `csrr a0,mhartid` | 读取当前 hart 的编号；本次为 `0` |
| `0x100c` | `ld a1,32(t0)` | 从 `0x1020` 读取地址，得到 `a1 = 0x87000000`，用作设备树地址 |
| `0x1010` | `ld t0,24(t0)` | 从 `0x1018` 读取下一跳地址，得到 `t0 = 0x80000000` |
| `0x1014` | `jr t0` | 跳转到 `0x80000000`，开始执行 OpenSBI 固件 |

附件中的内存数据为：

```text
0x1018: 0x0000000080000000
```

这里读取的值是地址常量，不应将其作为指令继续解释。OpenSBI 的 FW_DYNAMIC 接口通过 `a2` 接收前一阶段准备的动态信息结构地址；这与本次 ROM 中计算 `0x1028` 的指令相吻合。[OpenSBI FW_DYNAMIC 文档](https://github.com/riscv-software-src/opensbi/blob/master/docs/firmware/fw_dynamic.md)

#### B. PC 为 `0x1014` 时的寄存器

| 寄存器 | 日志值 | 含义 |
|---|---|---|
| `pc` | `0x1014` | 当前停在跳转到固件的指令，尚未执行该跳转 |
| `t0` | `0x80000000` | 即将跳转的固件入口地址 |
| `a0` | `0x0` | 当前 hart 的编号 |
| `a1` | `0x87000000` | 传给下一阶段的设备树地址；本次日志未进一步解析其内容 |
| `a2` | `0x1028` | 传给 FW_DYNAMIC 的动态启动信息结构地址；本次日志未展开其字段 |

#### C. 进入 OpenSBI

执行 `jr t0` 后，日志中的 PC 为 `0x80000000`，与 `t0` 的值一致。该处的指令为：

```text
0x80000000: add s0,a0,zero
0x80000004: add s1,a1,zero
0x80000008: add s2,a2,zero
0x8000000c: jal ra,0x800006a0
0x80000010: add a6,a0,zero
0x80000014: add a0,s0,zero
```

前三条将启动参数保存到 `s0`、`s1`、`s2`；随后用 `jal` 调用固件中的一段代码，并将返回地址写入 `ra`。后续指令保存返回值并恢复启动参数。这段观察表明处理器已从复位 ROM 进入固件。

#### D. 内核入口断点与栈变化

```text
Breakpoint 1 at 0x80200000: file kern/init/entry.S, line 7.
Breakpoint 1, kern_entry () at kern/init/entry.S:7
7    la sp, bootstacktop
```

| 观察时点 | PC | SP |
|---|---|---|
| 内核入口断点 | `0x80200000 <kern_entry>` | `0x80017ee0` |
| 后续单步 | `0x80200004 <kern_entry+4>` | `0x80203000` |

断点命中说明 CPU 的实际控制流已到达内核入口，且 GDB 中的内核符号能够对应到入口源码。后续单步证明设置栈指针的第一条机器指令已将 SP 改为栈顶地址。练习 1 的独立记录进一步显示执行完 `la` 并跳转到 `kern_init` 的结果。

入口断点不能单独证明所有内核功能正确，也没有逐条记录 OpenSBI 到内核之间的全部指令。应将其与固件输出和内核启动字符串结合判断。

#### 题目回答：最初几条指令位于什么地址

本次从暂停的 QEMU 启动会话观察，最初几条指令位于 `0x1000` 起的复位 ROM 区域。随后进入 `0x80000000` 的 OpenSBI，再进入 `0x80200000` 的内核。三个地址分别对应不同启动阶段，不能把内核入口当作处理器最初执行的地址。

#### 题目回答：最初几条指令主要完成哪些功能

这些指令完成启动参数准备和控制权转移：建立 PC 相对寻址基址，计算动态信息结构地址，读取 hart 编号与设备树地址，读取固件入口，最后跳转到固件。它们为 OpenSBI 提供启动上下文，尚未建立本实验的内核栈或调用 `kern_init`。

#### 启动阶段之间的关系与控制权交接

```text
QEMU 准备模拟内存、复位代码、固件及内核镜像
  → 0x1000：复位 ROM 准备参数并跳转
  → 0x80000000：OpenSBI 初始化并准备下一阶段
  → 0x80200000：S-mode 内核 kern_entry 设置栈
  → 0x8020000a：kern_init 清零 BSS、输出信息
  → 0x8020003a：执行 while (1) 对应的自跳转循环
```

OpenSBI 作为机器模式固件提供底层服务，并将控制权交给监督者模式内核。本次输出中的 `Next Address = 0x80200000` 与 `Next Mode = S-mode` 给出了交接目标；GDB 的入口断点验证 CPU 随后确实到达该地址。日志未直接读取特权级状态，因此特权交接的说明结合固件输出与启动机制给出。

---

## 五、测试与验证

### 编译与运行记录

以下命令已有本次实际执行记录：

```bash
cd ~/os2026/code/lab1
make
make qemu
```

构建输出中的产物记录：

```text
+ ld bin/kernel
riscv64-unknown-elf-objcopy bin/kernel --strip-all -O binary bin/ucore.img
```

运行输出中的关键数据：

```text
Firmware Base        : 0x80000000
Domain0 Next Address : 0x0000000080200000
Domain0 Next Mode    : S-mode
(THU.CST) os is loading ...
```

`bin/kernel` 保留 ELF 的结构和调试信息，供 GDB 解析符号；`bin/ucore.img` 是供当前运行参数使用的原始镜像。原始镜像不具有与 ELF 相同的入口、段表和符号元数据，运行地址需由启动机制正确安排。

田茂炜在 Ubuntu 虚拟机上重新构建后得到以下符号地址：

```text
0x80200000  kern_entry
0x8020000a  kern_init
0x80201000  bootstack
0x80203000  bootstacktop
0x80203008  edata
0x80203008  end
```

`bootstacktop - bootstack = 0x2000 = 8192` 字节，与 `KSTACKPAGE = 2`、`PGSIZE = 4096` 一致。本次 `edata` 与 `end` 相等，因此当前镜像没有非空 BSS 范围，`memset(edata, 0, end - edata)` 的长度为零；这不影响清零逻辑本身的正确性。

### 结果与覆盖范围

| 验证项 | 证据 | 判断 |
|---|---|---|
| 交叉编译与链接 | 已记录生成 `bin/kernel` 与 `bin/ucore.img` 的输出 | 本次构建完成 |
| 固件到内核交接 | `Next Address = 0x80200000`；入口断点命中 | 本次启动能到达内核 |
| 初始化与输出 | 出现 `(THU.CST) os is loading ...`；中断后 PC 位于 `0x8020003a` 的自跳转指令 | 已执行完打印路径并进入 `while (1)` |
| 复位到固件跳转 | PC 从 `0x1014` 变为 `0x80000000` | ROM 跳转目标与实测一致 |
| 内核栈设置 | SP 改为 `0x80203000`，与栈顶符号一致 | 入口栈设置符合预期 |
| 汇编到 C 入口 | 练习 1 单步后 PC 为 `0x8020000a <kern_init>` | 控制流进入 C 初始化入口 |
| BSS 清零边界 | 本次 `edata = end = 0x80203008` | 当前镜像 BSS 长度为零；本次未验证非空 BSS 的清零效果 |
| 自动评分 | 未提供 `make grade` 成功输出 | 不记为自动评分通过 |

以上验证覆盖最小启动链和输出路径，不涉及后续实验的内存管理、中断、调度、进程与文件系统功能。

### 测试证据与自动评分说明

练习 2 的 GDB 输出保存在 [lab1-ex2-gdb.txt](lab1-ex2-gdb.txt)，关键内容收录于附录 A。

`Makefile` 定义了 `grade` 目标，但仓库中缺少 `tools/grade.sh`，因此未执行自动评分。

---

## 六、实验总结与收获

### 对操作系统的理解

#### 1. 实验重要知识点与 OS 原理的对应关系

| 本实验重要知识点 | 对应的 OS 原理知识点 | 实验知识点的含义 | OS 原理知识点的含义 | 二者的关系 | 二者的差异 |
|---|---|---|---|---|---|
| 复位 ROM、OpenSBI 与内核交接 | 系统引导与特权级 | CPU 先执行复位代码，经固件进入内核；本次内核以 S-mode 运行 | 系统建立初始执行环境，并由具有适当权限的代码管理资源 | 是系统获得控制权和建立特权边界的具体过程 | 本次镜像由 QEMU 预加载，未实现完整磁盘引导与复杂引导器 |
| 设置 `sp` 后进入 C 函数 | 调用约定与执行上下文 | 在独立启动栈上运行 C 代码 | 函数调用需要栈与寄存器约定；任务切换需保存、恢复执行状态 | 栈是执行环境的重要组成部分 | 本次只有启动栈，未实现多个线程的栈管理和上下文切换 |
| 链接脚本与镜像地址 | 内存布局与程序装载 | 安排各段及入口，使加载位置与跳转目标一致 | 程序运行前需要建立代码、数据、栈等内存区域 | 链接布局决定本次内核在内存中的组织 | 本次主要为固定地址布局，未实现用户程序装载、动态链接或虚拟地址空间管理 |
| BSS 清零 | 程序运行时初始化 | 将未显式初始化的静态存储区设为零 | 运行环境应保证语言层面要求的初始状态 | 裸机内核需自行建立普通程序由运行时提供的条件 | 清零已有区域不等于物理内存分配或页表管理 |
| 通过 SBI 输出字符 | 分层接口与异常调用 | 内核执行 `ecall`，由固件提供字符输出 | 高层软件经接口请求更高权限层的服务，隐藏底层设备细节 | 展示了跨层服务调用及设备接口抽象 | 此处为 S-mode 内核调用 M-mode 固件，尚非用户进程向内核发起系统调用 |

#### 2. OS 原理中重要、但本实验未对应涉及的知识点

| 知识点 | 内容与重要性 | 与本实验范围的比较 |
|---|---|---|
| 物理内存分配与虚拟内存 | 管理可用页、映射关系与访问权限，为隔离和高效利用内存提供基础 | 本次只使用链接确定的固定布局，未实现分配器、页表或缺页处理 |
| 中断与异常处理 | 处理外设事件、计时与错误，使系统能响应异步事件并控制执行 | 本次使用固件提供的服务，尚未实现完整内核中断与异常处理框架 |
| 线程、进程与调度 | 组织多个执行实体，通过上下文切换共享 CPU，并提供运行隔离 | 本次仅启动一个最小内核，没有创建线程、用户进程或调度器 |
| 并发与同步 | 保证多个执行实体访问共享数据时的正确性，避免竞态和死锁 | 本次没有实际验证并发共享资源场景 |
| 文件系统与持久存储 | 组织长期保存的数据，并提供命名、访问与一致性机制 | 本次未实现文件管理，也未从文件系统动态装载程序 |

### AI 协作开发的经验

本次协作中，AI 帮助梳理工具版本、启动命令和 GDB 观察点，并辅助定位了“编译成功但没有内核输出”的问题。比较修改前后的 `Next Address`，再用入口断点验证，比仅凭终端是否出现固件信息更能确定执行到了哪一阶段。

使用 AI 时，应提供实际工具版本、运行参数和完整输出，避免把其他环境的命令直接当作适用于本机的结论。修改建议需要落到具体文件和目标上，并通过重新运行确认。对于本次启动参数调整，还应在报告中说明与课程原命令的差异，保证他人能够复现。

源码、机器指令和寄存器值也需相互核对。例如，`la` 在源文件中是一条伪指令，反汇编却对应两条机器指令；`sp` 后面的符号注记不能代替实际地址判断。AI 的解释可作为分析起点，最终结论应以代码和实测数据为依据。

---

## 附录 A：练习 2 原始日志

以下内容来自两次实际调试记录：原始日志验证复位 ROM、OpenSBI 与内核入口，本次在 `tmw-virtual-machine` 上继续单步到 `kern_init` 和最终循环。GDB 的符号注记保持原样。

```text
pc             0x1000	0x1000
=> 0x1000:	auipc	t0,0x0
   0x1004:	addi	a2,t0,40
   0x1008:	csrr	a0,mhartid
   0x100c:	ld	a1,32(t0)
   0x1010:	ld	t0,24(t0)
   0x1014:	jr	t0
0x1018:	0x0000000080000000
   0x80200000 <kern_entry>:	auipc	sp,0x3
   0x80200004 <kern_entry+4>:	mv	sp,sp
   0x80200008 <kern_entry+8>:	j	0x8020000a <kern_init>
0x0000000000001014 in ?? ()
pc             0x1014	0x1014
t0             0x80000000	2147483648
a0             0x0	0
a1             0x87000000	2264924160
a2             0x1028	4136
=> 0x1014:	jr	t0
0x0000000080000000 in ?? ()
pc             0x80000000	0x80000000
=> 0x80000000:	add	s0,a0,zero
   0x80000004:	add	s1,a1,zero
   0x80000008:	add	s2,a2,zero
   0x8000000c:	jal	ra,0x800006a0
   0x80000010:	add	a6,a0,zero
   0x80000014:	add	a0,s0,zero
Breakpoint 1 at 0x80200000: file kern/init/entry.S, line 7.
Continuing.

Breakpoint 1, kern_entry () at kern/init/entry.S:7
7	    la sp, bootstacktop
pc             0x80200000	0x80200000 <kern_entry>
sp             0x80017ee0	0x80017ee0
=> 0x80200000 <kern_entry>:	auipc	sp,0x3
   0x80200004 <kern_entry+4>:	mv	sp,sp
   0x80200008 <kern_entry+8>:	j	0x8020000a <kern_init>
0x0000000080200004 in kern_entry () at kern/init/entry.S:7
7	    la sp, bootstacktop
pc             0x80200004	0x80200004 <kern_entry+4>
sp             0x80203000	0x80203000 <SBI_CONSOLE_PUTCHAR>
=> 0x80200004 <kern_entry+4>:	mv	sp,sp

9           tail kern_init
pc             0x80200008       0x80200008 <kern_entry+8>
sp             0x80203000       0x80203000 <SBI_CONSOLE_PUTCHAR>

kern_init () at kern/init/init.c:8
8           memset(edata, 0, end - edata);
pc             0x8020000a       0x8020000a <kern_init>
sp             0x80203000       0x80203000 <SBI_CONSOLE_PUTCHAR>

Program received signal SIGINT, Interrupt.
kern_init () at kern/init/init.c:12
12         while (1)
pc             0x8020003a       0x8020003a <kern_init+48>
=> 0x8020003a <kern_init+48>:   j       0x8020003a <kern_init+48>
```

## 附录 B：模板与记录来源

- 报告结构：老师提供的《实验报告模板.md》。主章节沿用老师模板，小节编号统一为所在章节编号。
- [Lab1 实验报告要求](http://oslab.mobisys.top/lab2026/_book/lab1/lab1_5_requirement.html)
- [Lab1 练习题](http://oslab.mobisys.top/lab2026/_book/lab1/lab1_2_1_exercise.html)
- 环境、编译输出、Makefile 变更与练习 1 数据：本次对话中的实际操作记录。
- 练习 2 数据：[lab1-ex2-gdb.txt](lab1-ex2-gdb.txt)，包含原始日志及 2026-10-10 在 `tmw-virtual-machine` 上的补充复现；关键内容在附录 A 中保留。

仓库地址：https://github.com/KujoEnzo/OS2026

报告路径：`report/report.md`

提交分支：`lab1`
