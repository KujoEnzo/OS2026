# 田茂炜：Lab3模块二与完整联调交接

> 2026-11-05接手更新：模块二现已实现。用户Ubuntu构建、GDB、持续时钟及正常关机检查通过，
> 独立-kernel兼容评分副本得到100/100；原样评分失败保持记录。当前结果见
> [模块二报告](module2.md)和[回传摘要](logs/module2/ubuntu-reported.md)。
> 以下是模块一交接时的历史状态，“模块二为空/待实现”等描述仅指当时，不代表当前源码。

接手人：**田茂炜**。先读本页，再看 [code/lab3.md模块一报告](../code/lab3.md)。当前交付的是可独立验证的模块一，正式模块二仍为空；不要把临时测试处理器合入正式源码，也不要把历史Lab2成绩或副本PASS写成完整Lab3评分。

## 1. 分支、提交与取得材料

| 项目 | 确切状态 |
| --- | --- |
| 仓库/分支 | `https://github.com/KujoEnzo/OS2026.git`，`lab3` |
| Lab2起点 | `7a79a86b81ef33301a28eef87db85b2ae45cb21f` |
| 课程Lab3基线 | `3335e0e6e1663e33cba41c3d5645f6b9be0416f0` |
| 模块一最终源码与原实测提交 | `c4d06288262fb8255cb3ebda06407ecc0da46b77` |
| 本次交接提交 | 用 `git log -1 --format=%H -- report/handoff.md` 精确取得；最终交接消息会给出完整哈希，文档不自引用自己的内容哈希 |
| 上传授权 | 用户本轮追加要求“经过验证后，推送一次提交到云端仓库”；执行一次正常push，不强推；远程是否已更新以实查refs为准 |

首次取得（在Linux/WSL终端）：

```sh
git clone --branch lab3 --single-branch https://github.com/KujoEnzo/OS2026.git OS2026
cd OS2026
git status --short --branch
git log -3 --oneline
git log -1 --format=%H -- report/handoff.md
```

已有干净检出时，使用 `git fetch origin lab3`、`git switch lab3`、`git merge --ff-only origin/lab3`；如有个人改动先自行保存，不reset、不强制覆盖。拿到的代码至少应包含c4d0628及本交接文档提交。课程用的 `labcodes/lab3` 在本仓库对应 `code/`，无需寻找另一份工程。

原框架来自 [2026课程代码页](http://oslab.mobisys.top/lab2026/labcode.html) → [lab3.zip](http://oslab.mobisys.top/lab2026/lab3.zip)，包内 `lab3_new/`，56文件、57,209字节；归档为 [sources/lab3-2026.zip](sources/lab3-2026.zip)。

```text
框架ZIP SHA-256
b2328d813abf194e458cc4dab68815a1f9280e6ac73c9cfdcb4d60ec10f630bb

模块一正式trapentry.S SHA-256
d37c6e50bd3fa1ea0db7374336472be5ee5d57905d04246d84a7368019bb5caf
```

用 `sha256sum report/sources/lab3-2026.zip code/kern/trap/trapentry.S` 核对。迁移边界及原包逐文件哈希见 [基线说明](lab3-baseline.md)、[manifest](sources/lab3-2026-manifest.json)。manifest的baseline哈希固定于3335e0e，不能要求其中旧trapentry哈希等于实现后的文件。

## 2. 已交付改动与接口

模块一相对3335e0e的正式功能改动只有 `code/kern/trap/trapentry.S` 两处允许区，新增42行：先用槽32/33恢复sstatus/sepc，恢复x1/x3…x31，最后从槽2恢复sp；入口将a0设为当前sp并call trap，随后衔接原__trapret和sret。没有修改SAVE_ALL、trapframe、trap()函数体、评分逻辑或模块二占位。

本轮新增 `code/lab3.md` 仅为报告，不参与编译；更新 `report/report.md`、`prompt.md`、本页及README入口。为使已有模块一检查器在这个文档增量后仍可复现，只允许它识别 `code/lab3.md` 这一报告文件，未放宽其他生产源码检查。所有其他运行时源码保持c4d0628原样。

模块二必须遵守的边界：

- `trap(struct trapframe *tf)` 经 `trap_dispatch()` 按cause最高位分类；现有测试文件声明全局 `void interrupt_handler(struct trapframe *)` 和 `void exception_handler(struct trapframe *)`，不要擅改接口或改成测试不可见的static函数。
- frame大小288字节，status/epc偏移256/264，gpr.sp偏移16；完整布局见 [规格](module1-spec.md)。模块二只修改要交给返回流程的字段，模块一已消费处理后epc/status。
- 中断返回通常保留epc；断点/非法指令是否前进及前进长度由模块二根据实际编码和课程规则决定。不能全都加4。
- `clock.h/c` 提供 `volatile size_t ticks`、`clock_set_next_event()`；`clock_init()` 设置STIE、首次事件和ticks=0。持续时钟必须逐次重设下一事件，不能只增加计数。
- `sbi.h/c` 提供 `void sbi_shutdown(void)`。`trap.c` 的 `TICK_NUM=100` 已有，`SHUTDOWN_AFTER` 在当前框架中**未定义**；不要从旧年份或评分片段猜关机次数。

## 3. 环境与可以立即复现的检查

原实测环境：Windows/WSL2，发行版 `CosyVoice_Env`、用户 `kujoenzo`，本地仓库 `/home/kujoenzo/OS2026`；Ubuntu22.04.1，GCC交叉编译器10.2.0，Make4.3，QEMU6.2.0/OpenSBI0.9，gdb-multiarch12.1，Python3.10.6。详细版本见 [环境日志](logs/module1/formal/environment.log)。命令在Linux/WSL中执行；Windows侧可先运行 `wsl -d CosyVoice_Env -u kujoenzo`。

### 3.1 保留原日志，输出到新的目录

```sh
cd /home/kujoenzo/OS2026
python3 report/tests/module1/formal_check.py --repo . --out /tmp/lab3-module1-formal-repro
python3 report/tests/module1/run.py --repo . --out /tmp/lab3-module1-dynamic-repro
```

其他机器从其仓库根运行同样命令即可，不必使用原用户绝对路径。输出选 `/tmp` 是为了不覆盖Git中已经归档的实测证据；换新目录可保留多轮结果。

formal_check的“脚本退出0”表示结果符合交接阶段预期：布局/对象/反汇编通过，且**完整make仍然因模块二未实现退出2**。它不是完整构建成功。它严格绑定模块一基准；模块二一旦改动trap.c，会拒绝超出模块一的差异，这不自动意味着模块一回归，应以c4d0628为保留参照，另做模块二完整测试。

### 3.2 手工执行正式编译与反汇编

```sh
cd code
make clean
make -k -j2
# 当前预期退出2，只有interrupt_handler/exception_handler的隐式声明错误。
make -B obj/kern/trap/trapentry.o
riscv64-unknown-elf-objdump -dr obj/kern/trap/trapentry.o
riscv64-unknown-elf-gcc -mcmodel=medany -std=gnu99 -Werror -Wall -nostdinc \
  -Ilibs -Ikern/trap -c ../report/sources/trapframe-layout-check.c \
  -o obj/module1-layout.o
cd ..
```

反汇编应看到a0=sp、到trap的调用重定位、256/264偏移的CSR恢复、最后 `ld sp,16(sp)` 和sret。43项布局断言通过。不要为了完整构建绿灯临时在正式trap.c塞空处理器。

### 3.3 动态测试到底做了什么

run.py会在 `/tmp/os2026-module1-<label>-*/code` 建立新副本；只在副本改init入口与trap.c并增加测试C/汇编/头文件，测试调用位于第二次idt_init后、clock_init前。临时异常处理器仅识别已知测试断点、核对帧并设定预期返回；临时中断处理器直接失败。正式code没有这些处理器和测试栈。

依次运行正确版本、单条sp恢复变异版本、正确版复测。动态结果矩阵应是：构建均0，GDB分别**0、1、0**；变异版的1是故意使检查拒绝错误版本。成功组的串口有 `MODULE1_DYNAMIC_PASS cases=3 events=3 posts=3`，GDB另有逐项的帧/GPR/epc/status/priv检查。不能只grep串口一句PASS就认定恢复正确。

复现脚本记录精确QEMU/GDB命令，采用本环境的：

```sh
qemu-system-riscv64 -machine virt -nographic -bios default \
  -kernel bin/ucore.img -monitor none -no-reboot -S -gdb tcp:127.0.0.1:端口
gdb-multiarch -q -nx -batch -ex 'set $module1_port=端口' \
  -x /仓库绝对路径/report/tests/module1/gdb_check.py
```

以上两行需在**测试副本code/**执行，并把同一个端口实际数值及仓库路径代入；推荐直接用run.py自动分配端口、管理进程和超时。程序在验证完成后由驱动停止QEMU，不代表实现了关机。

### 3.4 原实现前的失败需要时才复现

```sh
tmp=$(mktemp -d /tmp/lab3-module1-red-XXXXXX)
git archive 3335e0e6e1663e33cba41c3d5645f6b9be0416f0 code | tar -x -C "$tmp"
python3 report/tests/module1/run.py --repo "$tmp" --out /tmp/lab3-module1-red-logs --red
```

预期检查确认空实现失败；不要通过移动当前HEAD或重置当前代码来制造RED。

## 4. 通过、失败、未运行及原始日志索引

| 状态 | 项目 | 证据 |
| --- | --- | --- |
| 通过 | 对象编译、43项布局断言、反汇编 | [formal/](logs/module1/formal/integrity.json) |
| 通过 | 三组正确副本、32GPR槽、高位值、sp、a0参数、对齐、处理后epc/status和sret后S态 | [positive-gdb.log](logs/module1/dynamic/positive-gdb.log)、[串口](logs/module1/dynamic/positive-qemu.log) |
| 通过 | 正确版重新建副本复测；正式源码哈希不变 | [recheck-gdb.log](logs/module1/dynamic/recheck-gdb.log)、[汇总](logs/module1/dynamic/results.json) |
| 预期失败 | 空占位未call trap，链接后无该符号 | [RED](logs/module1/red/red-gdb.log) |
| 预期失败 | 人工sp+288反例，case2恢复了错误栈顶 | [mutant-sp-gdb.log](logs/module1/dynamic/mutant-sp-gdb.log) |
| 已定位并调整测试 | stepi停点不在返回首指令，改目标临时断点采样 | [diagnostic-stepi](logs/module1/diagnostic-stepi/positive-gdb.log) |
| 当前失败/阻塞 | 正式完整构建退出2，模块二两个函数缺失 | [full-build.log](logs/module1/formal/full-build.log) |
| 未运行 | 正式原样make qemu/debug/grade、完整课程评分 | 没有对应Lab3通过成绩或日志，不沿用Lab2成绩 |
| 未验证 | 真实持续时钟/外部中断、U栈、递归异常、FPU/向量 | 测试将sie清零，仅验证受控S态断点返回 |

原始日志来自c4d0628提交；本轮文档和检查兼容性的核对记录另存 [logs/handoff/checks.json](logs/handoff/checks.json)，不会覆盖旧证据。原始串口CRLF保持原样，git空白提示不等于新功能错误。

## 5. 已知问题与学号待填

1. **正式模块二空白**：`trap.c` 中function2标记后没有两个处理器定义；当前完整构建失败符合这个事实，不能标为全部完成。
2. **关机阈值未确定**：课程练习提及SHUTDOWN_AFTER而原包没有定义。请先按当前课程要求确认，且不要让关机早于评分需要观察的持续时钟行为。
3. **启动/调试差异待联调**：课程Makefile和评分使用 `-device loader`，本环境受控测试使用 `-kernel`；课程gdb目标硬编码 `riscv64-unknown-elf-gdb`，本环境实际用gdb-multiarch。正式联调保留原样失败证据，不修改评分逻辑以刷通过。
4. **GDB采样**：本组合中stepi越过sret没有停在期望首指令；用新epc处的临时断点直接检查返回状态。不要把诊断日志里的错误停点误认为最终未修复的汇编故障。
5. **真实学号尚未提供**：`trapentry.S` 两处 `#lab3 function1 YOUR CODE` 后已有模块一代码，但YOUR CODE仍待模块一作者填真实学号；`trap.c` 一处 `//lab3 function2 YOUR CODE` 后仍为空，由田茂炜完成后填真实学号。课程要求保留lab3标记，不能编造数字。本次不替换标记，也不伪装最终全实验提交已满足。
6. **身份登记**：模块一实现账户KujoEnzo，中文署名提交前确认；田茂炜学号待本人提供。不要照搬其他年份或Lab2中未经核实的型号、身份信息。

## 6. 田茂炜后续任务清单

- [ ] 按 [课程练习](http://oslab.mobisys.top/lab2026/_book/lab3/lab3_2_1_exercise.html) 阅读真实trap.c、clock、riscv、SBI接口，先写模块二规格/验收断言。
- [ ] 在function2规定位置实现 `interrupt_handler()` 和 `exception_handler()`，保留已有接口和模块一；不从测试副本照搬测试专用处理器。
- [ ] 持续时钟：IRQ_S_TIMER每次重设下一事件、ticks逐次增加，按TICK_NUM输出统计；确认规定累计次数后用框架sbi_shutdown关机，记录真实行为。
- [ ] 异常：区分非法指令与断点，按实际指令长度更新tf->epc，输出规定信息；未要求恢复的异常调用print_trapframe兜底，不让返回反复执行同一故障指令。
- [ ] 结合未改动的 `tools/grade.sh` 检查输出：上下文组35分要求 `Breakpoint at`；非法指令组25分涉及 `Illegal instruction at`、`Supervisor software interrupt`、`Unknown interrupt: cause 15` 和兜底cause20；时钟组40分观察 `100 ticks` 及 `ticks=200`。这是脚本检查范围，不替代课程完整关机要求。
- [ ] 模块二完成后运行 `cd code; make clean; make`，再按课程运行 `make qemu`、`make debug`/GDB、`make grade`，完整保存退出码、串口及评分输出；本轮尚未运行这些集成检查，不预填分数。因编译器标志切换，评分前使用干净构建。
- [ ] 若启动受环境影响，定位装载入口/OpenSBI/调试器差异，保留原样运行结果；如做补充兼容运行，明确区分，不改评分脚本或以兼容结果替代原样结果。
- [ ] 在 `code/lab3.md` 追加模块二规格/prompt、核心函数分析、真实问题修复、持续时钟/规定输出/关机和完整评分；同步更新 `report/prompt.md` 的实际提示词记录。
- [ ] 填齐两位真实学号，保留所有lab3注释；核对课程报告六类要求、相对链接和正式目录无临时测试代码，再提交最终实验版本。

模块一可作为稳定参照继续使用；如果模块二改变了frame、返回约定或开始主动改变中断状态，须先检查与 [module1-spec.md](module1-spec.md) 的兼容性并新增覆盖相应输入的测试，不能只看总评分输出。
