# Lab3 基线准备记录

日期：2026-11-04（Asia/Shanghai）。本次只准备课程基线、迁移必要的 Lab2 完成内容并整理模块一规格；两个 Lab3 功能模块都未实现。

## 1. 仓库与起点

- 实际本地仓库：WSL 发行版 `CosyVoice_Env` 中的 `/home/kujoenzo/OS2026`，操作用户 `kujoenzo`。
- 远程：<https://github.com/KujoEnzo/OS2026.git>。
- 开始时在 `lab2`，HEAD 为 `7a79a86b81ef33301a28eef87db85b2ae45cb21f`，跟踪 `origin/lab2`；暂存区、工作区均干净。
- 检查了所有本地/远程跟踪分支与 worktree：当时没有 `lab3`；仅有一个工作区。通过 `git switch -c lab3` 从上述 Lab2 提交建立分支，保留完整父提交历史，未 reset、rebase、重建已有分支或推送远程。
- 已阅读根目录 `README.md`，沿用 `code/`、`report/` 的目录约定；仓库内未发现 `AGENTS.md`。
- 原有 Lab2 报告、提示词、截图和日志均保留，属于历史材料。它们不代表 Lab3 的实现或成绩。Lab2 原分支未移动。

## 2. 真实课程框架来源

来源链为课程官网首页 → “2026年的实验代码” → `lab3`：

1. [课程首页](http://oslab.mobisys.top/)
2. [2026 代码下载页](http://oslab.mobisys.top/lab2026/labcode.html)
3. [Lab3 原始代码包](http://oslab.mobisys.top/lab2026/lab3.zip)
4. [本次练习要求](http://oslab.mobisys.top/lab2026/_book/lab3/lab3_2_1_exercise.html)

下载日为 2026-11-04；包大小 **57,209 字节**，包内根目录为 `lab3_new/`，共 **56 个文件**。

```text
SHA-256: b2328d813abf194e458cc4dab68815a1f9280e6ac73c9cfdcb4d60ec10f630bb
```

原包保存为 [sources/lab3-2026.zip](sources/lab3-2026.zip)，逐文件原始/基线 SHA-256 及迁移标记保存于 [sources/lab3-2026-manifest.json](sources/lab3-2026-manifest.json)。下载页提供的是 ZIP，没有课程 Git 提交号；因此以下载 URL、日期和内容哈希固定本次来源，不将学生仓库的 Lab2 提交冒充课程框架版本。

原包中的 `trapentry.S` 确实留有两处模块一占位，`trap.c` 留有模块二占位，符合 2026 练习。没有使用其他年份、x86 或其他 RISC-V 项目代码补齐框架。课程说明和源码注释中残留的 `iret`、8253 等旧表述不能覆盖实际 RISC-V 指令与接口。

## 3. 迁移清单与明确边界

先逐文件采用课程 ZIP 的 Lab3 内容，再仅向需要补齐的 Lab2 区域迁移代码；没有把 Lab2 整个目录覆盖到 Lab3。最终 `code/` 的文件集合等于课程包的 56 个文件，其中 **53 个与原包逐字节一致**，仅以下三个文件有必要迁移差异：

| 文件 | 来自 Lab2 的内容 | 保留的 Lab3 行为 / 理由 |
| --- | --- | --- |
| `code/kern/mm/pmm.c` | `page_init()` 整个函数，来自起点提交 | 保留课程的默认 `default_pmm_manager` 选择，以及 `alloc_pages/free_pages/nr_free_pages` 的 `local_intr_save/restore`；未复制 Lab2 的 Buddy/Best-Fit 选择开关 |
| `code/kern/mm/memlayout.h` | `PHYSICAL_MEMORY_END = 0x88000000`、`KERNEL_BEGIN_PADDR = 0x80200000` 两个必需宏 | 它们是迁移的 `page_init()` 的真实依赖；与框架的 `KERNTOP - PHYSICAL_MEMORY_OFFSET`、`KERNBASE - PHYSICAL_MEMORY_OFFSET` 分别一致。保留课程 `atomic.h` 和原子 Page 标志操作；未带入无用的 `KERNEL_BEGIN_VADDR` |
| `code/kern/mm/best_fit_pmm.c` | 空闲链表定义/宏、初始化、插入合并、分配释放、计数函数和 `best_fit_pmm_manager` 定义 | 该文件由 Makefile 编译，即使默认未选择 Best-Fit，框架缺失的 `free_list/nr_free` 等定义仍须补齐；保留课程 `basic_check()`、`best_fit_check()` 原文，不用 Lab2 文件替换整文件 |

`page_init()`、Best-Fit 实现区和管理器定义与指定 Lab2 提交核对一致；两项地址宏核对一致。其余初始化、DTB、时钟、中断、同步、SBI、调试、头文件和 Makefile 均使用课程包。特别保留 Lab3 `entry.S` 的精确栈顶设置、`init.c` 的 BSS 清零后 DTB 初始化顺序、两次 `idt_init()` 及随后 `clock_init()/intr_enable()`。

以下 Lab2 专用文件从 **Lab3 分支当前树** 移除：

- `code/kern/mm/buddy_pmm.c`、`buddy_pmm.h`、`buddy_pmm_check.h`：课程 Lab3 不依赖，也不默认启用 Buddy，避免 Makefile 通配符将无关扩展编入。
- `code/tools/test_lab2.c`、`test_lab2.h`：由课程自带的 Lab3 测试文件取代，避免混合两次实验的测试体系。

它们仍可从 `lab2` 及本分支父提交读取，未删除历史。保留原有 README 和历史报告，只给 README 增加 Lab3 基线入口。

`tools/grade.sh`、`inject.py`、`test_lab3.c`、`test_lab3.h`、`test_lab3_context.S`、Makefile 均与课程 ZIP 逐字节一致。Git 中显示评分文件相对父提交变化，是从 Lab2 版本更新到官网 Lab3 原版，没有修改课程的评分逻辑、测试或编译参数。

`trapentry.S`（包含 `SAVE_ALL`、`RESTORE_ALL`、`__alltraps`、`__trapret`）、`trap.h`、`trap.c` 全部与原包一致；没有实现模块一，也没有替模块二添加声明、空实现或其他绕过构建的代码。

## 4. 已读取的接口与环境

已核对 `trapentry.S`、`trap.h`、`trap.c`、`libs/riscv.h`、`defs.h`、`entry.S`、`init.c`、`pmm.c/h`、`memlayout.h`、`mmu.h`、两个分配器、DTB、`sync.h`、`intr.c/h`、`clock.c/h`、`sbi.c/h`，以及 Makefile、链接脚本与课程测试/注入接口。模块一结论见 [module1-spec.md](module1-spec.md)。

| 项目 | 本次实际环境 |
| --- | --- |
| 主机入口 | Windows PowerShell；通过 WSL 执行 Linux 工具；聊天目录不是仓库 |
| 系统 | Ubuntu 22.04.1 LTS，WSL2，Linux `6.18.54.1-microsoft-standard-WSL2`，x86_64 主机 |
| Git / Make | Git 2.34.1 / GNU Make 4.3 |
| 交叉编译器 | `/usr/bin/riscv64-unknown-elf-gcc` 10.2.0 |
| 编译器默认目标 | `rv64imafdc`、`lp64d`；框架未显式设置 `-march/-mabi`，实测默认值已记录 |
| 链接器 | GNU ld 2.35.1，框架使用 `-m elf64lriscv` |
| 模拟器 | `/usr/bin/qemu-system-riscv64` 6.2.0 |
| 调试器 | `gdb-multiarch` 可用；未找到 `riscv64-unknown-elf-gdb`，框架 gdb 目标硬编码后者，不能仅设置 GDB 变量替代 |
| Python | Python 3.10.6 |

完整工具输出见 [logs/lab3-baseline/environment.log](logs/lab3-baseline/environment.log)。后续调试可在 `code/` 直接执行 `gdb-multiarch -ex 'file bin/kernel' -ex 'set arch riscv:rv64' -ex 'target remote localhost:1234'`；本次不改 Makefile 适配工具名。

## 5. 构建命令与实际结果

均在上述 WSL 用户环境执行。迁移前先在 `lab2` 的 `code/` 运行 `make -B -j2`：退出 **0**，成功链接并生成镜像，见 [lab2-preflight-build.log](logs/lab3-baseline/lab2-preflight-build.log)。

在 `lab3` 上：

```sh
cd /home/kujoenzo/OS2026/code
make clean
make -k -j2
```

`make clean` 退出 **0**。使用 `-k` 是为了在已知占位报错后继续检查独立编译单元，没有降低错误检查。最终构建退出 **2**，实测错误仅为课程 `kern/trap/trap.c` 的：

```text
trap.c:89:9: error: implicit declaration of function 'interrupt_handler'
trap.c:92:9: error: implicit declaration of function 'exception_handler'
cc1: all warnings being treated as errors
```

模块二占位没有这两个函数的定义/声明，框架启用 `-Werror`，因此在 C 编译阶段失败。**没有生成 `bin/kernel` 或 `bin/ucore.img`，没有运行 QEMU 或 `make grade`，没有 Lab3 运行成功或评分通过结论。** 模块一空宏虽可汇编，但尚不能完成正确 Trap 返回；汇编成功不代表模块一完成。

独立验证：

```sh
make -B obj/kern/mm/pmm.o obj/kern/mm/best_fit_pmm.o obj/kern/trap/trapentry.o
riscv64-unknown-elf-gcc -mcmodel=medany -std=gnu99 -Werror -Wall -nostdinc \
  -Ilibs -Ikern/trap -c ../report/sources/trapframe-layout-check.c \
  -o obj/trapframe-layout-check.o
```

两项均退出 **0**。布局探针仅包含 43 项目标编译期断言，检查全部 36 个字段偏移、RV64/8 字节寄存器、结构大小/对齐和栈增量；不链接进内核，不实现任何模块。原始日志与机器可读检查结果分别见 [build.log](logs/lab3-baseline/build.log)、[migrated-objects.log](logs/lab3-baseline/migrated-objects.log)、[layout-check.log](logs/lab3-baseline/layout-check.log)、[verification.json](logs/lab3-baseline/verification.json)。

首次迁移构建还发现 `page_init()` 引用的两项 Lab2 地址宏缺失；核对 Lab2 与 Lab3 映射后，仅迁入这两个必要宏。初始失败日志保留为 `pre-adaptation-build.log` 和 `pre-adaptation-migrated-objects.log`；最终日志已确认这些错误消失，不将其列为未解决问题。

可用 `sha256sum ../report/sources/lab3-2026.zip` 核对归档；清单记录每个源文件哈希，可将 ZIP 中的 `lab3_new/<path>` 与 `code/<path>` 对照。三个迁移文件应只出现上表列明的差异，其余 53 个文件应完全一致。

额外执行了 `git diff --check`：退出 2，指出课程原版 Best-Fit 检查代码、`default_pmm.c` 和 `tools/inject.py` 自带的行尾空格/文件末尾空行。已对照原包确认，未为消除格式提示而修改课程测试或评分文件；这不属于编译失败原因。新写报告无这些空白问题。

## 6. 未解决问题与下一步边界

1. 模块一 `RESTORE_ALL` 与 `__alltraps` 仍待本人根据规格实现；模块二两个处理函数仍待对应分工完成，完整内核尚不能构建。
2. 官网练习提及 `SHUTDOWN_AFTER`，但本次真实框架未定义该宏；`trap.c` 只有 `TICK_NUM = 100`。没有擅自确定关机次数，此项属于后续模块二规格确认。
3. 尚未验证 QEMU/OpenSBI 启动、Trap 返回、时钟持续触发或课程评分。必须在两个模块完成后另行验证，不能沿用 Lab2 成绩。
4. 框架调试目标硬编码 `riscv64-unknown-elf-gdb`，本环境只有 `gdb-multiarch`；模块一规格给出直接调试命令，不更改课程工具。

本次本地提交的范围为课程 Lab3 基线、三项必要迁移、上述 Lab2 专用源码的移除、两份规格/记录、来源包/清单、布局探针和验证日志，以及 README 入口。没有远程推送。
