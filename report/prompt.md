# Lab3 实际提示词与修正记录

## 2026-11-05：田茂炜接手模块二

后续实际提示：“补全项目并上传到github”。用户已回传Ubuntu验证结果及兼容评分100/100。
报告按回传更新；原始Ubuntu日志未直接取得，单独标注来源，不改写早期失败记录。
用户随后提供本人学号2412784，填写模块二标记与报告；未推断模块一作者学号。

本轮实际用户提示：

> 能不能直接完成实验
>
> 直接读取github即可

上下文为此前提供的 Lab3 双人分工及模块一 handoff：田茂炜实现 interrupt_handler、
exception_handler，完成联调并整理报告。以 GitHub lab3 提交 fbe7f5c 为读取基线。

实现规格见 [module2.md](module2.md)。执行中核对原始 grade.sh、test_lab3.c、clock.c、
trapframe 与 CSR 宏，保留课程测试；补全时钟续设/计数/统计/关机、中断分类、软件中断
确认、非法指令和断点16/32位返回及兜底打印。原包缺少 SHUTDOWN_AFTER，明确记录默认
10组输出的实现假设，不宣称其为教师指定数字。

桌面端验证：WSL GCC 主机桩测试通过；本机 RISC-V 工具包下载未成功，未在桌面端执行
目标集成验证。后续用户在Ubuntu运行验证并回传兼容评分100/100，记录于
[验证摘要](logs/module2/ubuntu-reported.md)，不把用户实测写成桌面端独立复测。


本页只记录本会话中实际使用的用户任务正文，以及有代码/日志证据的执行修正。转录仅规范段落换行和链接显示，不把事后归纳冒充原始提示词。Git没有保存模型内部对话；以下任务正文来自当前会话，结果用提交和仓库文件交叉追溯。未虚构用户曾单独发送的“修bug提示词”，也不照搬Lab2的模型型号登记为本轮记录。

| ID | 实际用途 | 结果/追溯 |
| --- | --- | --- |
| L3-P01 | 准备真实课程基线、最小Lab2迁移、模块一规格 | `3335e0e6e1663e33cba41c3d5645f6b9be0416f0`；[基线记录](lab3-baseline.md) |
| L3-P02 | 只将已完成基线推送GitHub | `lab3`远程曾指向3335e0e；没有因此新增功能提交 |
| L3-P03 | 实现模块一并进行独立验证和反例 | `c4d06288262fb8255cb3ebda06407ecc0da46b77`；[验证记录](module1-validation.md) |
| L3-P04 | 本轮报告与田茂炜交接 | 包含本文件的提交；`git log -1 --format=%H -- report/prompt.md` |
| L3-P05 | 对本轮的后续修正授权：验证后推送一次 | 覆盖P04的“不推送”；最终Git远程核对和交接消息在任务完成时提供，不据此声称模块二完成 |

## L3-P01：基线与规格任务正文

```text
[PROMPT]
请在当前 OS2026 本地仓库中，实际完成 Lab3 基线准备和模块一规格整理。本次不实现两个功能模块。

仓库：https://github.com/KujoEnzo/OS2026
课程练习：http://oslab.mobisys.top/lab2026/_book/lab3/lab3_2_1_exercise.html

分工：我负责完成模块一。保留已有改动，不重置历史，不推送远程。

[RELY]
先检查分支、工作区、README、现有 Lab2 实现和可用工具。
取得课程真实 Lab3 框架，不能用其他年份或其他架构的代码替代。若无法取得，只向我询问缺失的代码包来源。

读取框架中的 trapentry.S、trap.h、trap.c、riscv.h、初始化、内存管理、时钟和 SBI 接口。所有布局、宏和函数定义以真实文件为准。

[GUARANTEE]
1. 准备 lab3 分支，以课程 Lab3 框架为基础迁移 Lab2 必需的已完成部分。
2. 创建 report/lab3-baseline.md，记录框架来源、迁移清单、环境、构建命令和实际结果。
3. 创建 report/module1-spec.md，说明 RESTORE_ALL、__alltraps 的输入、输出、约束和验收方法。

[SPECIFICATION]
Pre-Condition：已有改动不能覆盖，已有 lab3 分支不能擅自重建。
Post-Condition：得到来源明确、迁移范围明确的基线和基于真实代码的模块一规格。

Requirements：
- 不整目录覆盖 Lab2，不无理由带入 Buddy 等扩展。
- 不修改评分脚本，不代做模块二。
- 实际运行构建；若占位代码导致失败，记录真实原因。
- 规格必须包含 Trap frame 布局与偏移、sp 特殊保存方式、参数传递、ABI 对齐、处理后的 epc 及返回状态。
- 只提交本任务相关文件，完成本地提交。

最后给出提交号、改动清单、构建结果和未解决问题。
```

## L3-P02：基线推送请求

```text
向github提交一次基线准备完成
```

实际结果是推送现有3335e0e到 `origin/lab3`，没有人为再造一个相同内容的提交。该授权只对应当时基线，随后P03重新要求本地提交、不推送。

## L3-P03：模块一实现任务正文

```text
[PROMPT]
基于当前 Lab3 基线、report/lab3-baseline.md 和 report/module1-spec.md，实际实现并验证模块一：RESTORE_ALL 与 __alltraps。

正式实现只修改 code/kern/trap/trapentry.S 的规定位置；若工程目录不同，先记录对应关系。不要实现正式 interrupt_handler()、exception_handler()。

[RELY]
重新读取真实 SAVE_ALL、struct trapframe、trap(struct trapframe *tf)、__trapret、REGBYTES/LOAD/STORE、Makefile 和调试支持。
所有偏移、宏和接口以当前框架为准。

[GUARANTEE]
1. RESTORE_ALL 正确恢复要求的通用寄存器、返回位置和状态。
2. __alltraps 将当前 Trap frame 按 RISC-V ABI 传给 trap()，C 返回后进入统一恢复流程，最终经 sret 返回。
3. 保存测试材料、复现步骤及真实日志。

[SPECIFICATION]
RESTORE_ALL：
Pre-Condition：sp 指向 SAVE_ALL 建立的 Trap frame，C 处理函数可能修改了 tf->epc。
Post-Condition：恢复过程保持帧基址有效，正确恢复 sp，并采用处理后的 tf->epc 恢复返回位置。

__alltraps：
Pre-Condition：处理器进入框架 Trap 入口。
Post-Condition：保存上下文后，trap() 收到正确的 Trap frame 指针，处理完毕衔接已有返回路径。

Requirements：
- 不修改 SAVE_ALL、struct trapframe、trap() 签名和已有返回框架。
- 核对栈对齐、寄存器恢复顺序及临时寄存器覆盖问题。
- 不一律强制返回 U 态，不无条件开启中断。
- 先列验收断言，再实现；实际编译并检查反汇编。
- 验证 trap() 参数、具有已知期望值的代表性通用寄存器和 sp、处理后的 epc，以及 sret 后的返回状态。
- 模块二未实现时，可以在测试副本加入最小临时处理器，使用明确长度的测试异常验证返回；临时处理器不得进入正式交接代码。
- 对最可能的恢复缺陷，在测试副本中做一次受控反例，确认检查能够发现错误。
- 不能仅凭编译成功或出现后续输出，就宣称上下文恢复正确。
- 缺少 QEMU/GDB 时明确记录未运行项，不编造结果。

失败时先定位原因，再最小化修复。最后确认无临时测试代码残留，完成相关文件的本地提交，不推送远程。

输出提交号、改动摘要、验证结果及仍需完整联调的项目。
```

## L3-P04：报告与交接任务正文

```text
[PROMPT]
请根据真实代码、Git 历史和实际日志，整理模块一报告及交接材料。接手人是田茂炜，负责模块二和完整联调。本次不扩展功能。

[RELY]
读取模块一规格、基线说明、实际代码、测试材料、日志和提交历史。
报告要求：http://oslab.mobisys.top/lab2026/_book/lab3/lab3_4_lab_requirement.html

[GUARANTEE]
1. 在 Lab3 工程根目录 code/lab3.md 整理模块一报告，report/report.md 提供入口。
2. 更新 report/prompt.md，记录实际使用且可追溯的提示词与关键修正。
3. 创建 report/handoff.md，供田茂炜接手。

[SPECIFICATION]
Pre-Condition：实现可能存在未验证项或遗留问题，文档必须如实反映。
Post-Condition：田茂炜无需依赖聊天历史，就能复现检查并继续模块二。

报告包含：
整体 Trap 执行流、模块一核心函数理解、spec 与 prompt、真实 bug 和修复、验证结论，以及与 OS 原理的联系。

交接说明包含：
- 分支、最终提交号和框架来源。
- 环境、精确构建运行命令和日志位置。
- 正式改动文件与临时测试方式。
- 已通过、失败、未运行的检查。
- 已知问题、学号待填项。
- 田茂炜后续任务：interrupt_handler()、exception_handler()、持续时钟、规定输出与关机、完整评分及模块二报告。

不编造故障经历、测试结果或学号；不把占位代码写成完成。
核对课程标记、文档链接及测试残留，只提交相关文件，不推送远程。

最后输出交接提交号，以及一段我可以直接发给田茂炜的交接消息，不要替我发送。
```

## L3-P05：本轮后续推送授权

```text
经过验证后，推送一次提交到云端仓库
```

执行解释：先完成文档和复现核验，再创建本次交接提交，最后一次正常推送 `lab3`，同时带上此前未推送的c4d0628实现提交；不改写历史，不替用户发送交接消息。文档内不提前捏造推送成功，最终以远程refs实查为准。

## 关键修正：区分真实故障、测试调整与人工反例

这些是执行过程中实际发生的修正，不是用户后来发送的额外提示词：

1. **补齐迁移依赖。** P01首次构建出现两个物理地址宏缺失，核对真实文件后只迁移必要宏；见 [pre-adaptation-build.log](logs/lab3-baseline/pre-adaptation-build.log) 与基线清单。
2. **先验证空实现确实失败。** P03的RED阶段未调用trap，链接回收后GDB找不到符号，检查失败。这不是把编译通过当作实现通过；见 [red-gdb.log](logs/module1/red/red-gdb.log)。
3. **调整GDB采样。** `stepi` 实测未停到sret后第一条指令，改为返回目标临时断点；保留 [原失败日志](logs/module1/diagnostic-stepi/positive-gdb.log)，最终 [positive-gdb.log](logs/module1/dynamic/positive-gdb.log) 核对实际PC、状态和特权级。没有因此改动正式汇编。
4. **澄清scratch证据。** C快照借用并清理scratch，故将该项标为采样清理检查；GDB在sret前及返回目标直接比较真实scratch，见 [review.log](logs/module1/review.log)。
5. **人工变异只作检查能力证明。** 只在副本故意把槽2加载改成sp+288，第三组失败、正确版复测通过，见 [反例](logs/module1/dynamic/mutant-sp-gdb.log) 与 [结果汇总](logs/module1/dynamic/results.json)。不把它写成正式代码曾出现的事故。
6. **本轮文档兼容性。** P04要求新增工程根下的 `code/lab3.md`；原模块一检查器把它误当新增生产源文件拒绝。仅允许这个明确的报告路径，继续严格拒绝额外测试源码；本轮核验日志在 [logs/handoff](logs/handoff/checks.json)。正式内核功能和既有实测日志不改。

Lab2原提示词全文仍可从Git读取：`git show 7a79a86b81ef33301a28eef87db85b2ae45cb21f:report/prompt.md`。本页不将Lab2的Best-Fit/Buddy任务列作本次Lab3模块一提示词。
