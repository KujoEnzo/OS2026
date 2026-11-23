# Lab4 A 模块规格
> 历史阶段记录（2026-11-22 A 交付时）。最终版本请阅读 [实验报告](report.md) 和 [最终验收](final-validation.md)。A 已署名郝煦享（2412695），B 已合入并联合通过兼容评分；下文保留当时的开发状态和失败证据，不作为当前待办。

范围：page_insert/page_remove、cpu_idle/schedule/proc_run/switch_to；不实现 B 六函数。

映射成功返回 0，失败 -E_NO_MEM；同页重映射不改变引用计数，不同页替换释放旧引用；删除幂等。页表项、引用计数和 TLB 保持一致。框架 get_pte 在第二次分配失败时保留第一个中间页表供复用；失败不产生目标叶映射，不增加目标页引用，暂不改变此非事务式中间表分配约定。

调度从 current 下一项开始循环，跳过哨兵和不可运行线程；允许选择唯一可运行的 current，否则回退 idleproc。idleproc 不挂入 proc_list。清除调用者 need_resched，每次选中增加目标 runs。临界区关闭中断，完成后恢复原状态。

proc->pgdir 保存根页表物理地址；Sv39 satp=(8UL<<60)|(pgdir>>12)，刷新 TLB。context 为 14 个 RV64 字段：ra、sp、s0…s11。B 首次运行须设置 context.ra=forkret，context.sp 指向自己的 trapframe。

在临时副本替换启动入口，用人工 PCB 和独立栈测试真实 A 函数。不实现 B、不改变正式启动流程，不代表 initproc 完整联调。
