# Lab4 A 提示词记录

本次真实用户执行指令为：

> 完成A部分的工作

此前用户依次请求：

> 我将要完成lab4，[http://oslab.mobisys.top/lab2026/\_book/lab4](http://oslab.mobisys.top/lab2026/_book/lab4) ，仓库[https://github.com/KujoEnzo/OS2026](https://github.com/KujoEnzo/OS2026)，给出一份双人计划书

> 给出一份A可用的prompt

## 执行上下文摘要（不是新的逐字用户指令）

助手先给出五天双人计划，再提供 A 的执行 prompt；用户随后要求完成 A。A 范围为基线迁移、page_insert/page_remove、cpu_idle/schedule/proc_run/switch_to、验证和交接；B 的 alloc_proc/proc_init/kernel_thread/do_fork/copy_thread/copy_mm 保留待填。要求读取真实课程及仓库、维护 spec、检查映射引用计数和 Sv39、执行真实测试、保存实际日志，不编造完整通过结果或学号。

模块 spec 见 a-spec.md。编码由本会话助手直接完成，没有另外调用代码生成服务或人为编造“让 AI 修复某 bug”的多轮用户提示。实际遇到的问题与修复见 a-baseline.md、a-validation.md 和 a-review.md。

## 独立审查实际委派提示

请独立审查 Lab4 A 实现（只读，不修改）。先读 report/a-spec.md、docs/superpowers/plans/2026-11-22-lab4-a.md，再审查 pmm.c 的 page_insert/page_remove/tlb_invalidate，proc.c 的 cpu_idle/proc_run，sched.c 和 switch.S；并审查 tools/a_tests 是否实际验证声称行为。前置迁移来自 origin/lab3，特别检查兼容性。B 的六函数故意未实现，不作为缺陷。重点：同页重映射、失败引用计数、链表回绕、自选择、嵌套中断恢复、satp 物理地址/模式、汇编布局。区分严重级别，引用文件行；只报告可行动问题，说明验证限制。

上段是实际委派内容的路径简化摘录；不含执行环境的绝对路径等重复信息。审查结果及采取的修改单独保存。
