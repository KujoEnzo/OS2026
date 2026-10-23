# Lab2 代码生成提示词

以下仅归档用于生成Best-Fit、Buddy分配器及Buddy专项测试代码的三份完整提示词。正文保留原文，仅去除首尾空行。实验规格的规范化说明另见实验报告。

## 1. Best-Fit 分配器

````text
你正在协助完成 RISC-V ucore Lab2 的模块2任务二。
在WSL中先读取 code/kern/mm/{pmm.h,memlayout.h,pmm.c,default_pmm.c,
best_fit_pmm.h,best_fit_pmm.c.pending} 和 code/libs/list.h。

目标：实现完整 Best-Fit 连续物理页分配器，并让内核默认使用它。
保留组员已经完成的 page_init()。不要修改原有 basic_check() 与
best_fit_check()，不要修改评分脚本或注入测试来使测试通过。

设计：使用按物理地址升序的空闲块链表。维护 free_area_t free_area，
以及原测试可直接使用的 free_list、nr_free。提供 init、init_memmap、
alloc_pages、free_pages、nr_free_pages、check 对应方法和
const struct pmm_manager best_fit_pmm_manager 注册对象。
分配时选择所有满足请求的块中最小者，同大小选择低地址；
完整匹配可提前结束。正确拆分、摘链并维护 PageProperty、property、
ref、nr_free。释放后按地址插入，并合并物理连续的前后空闲块。
n=0 按现有接口断言约定处理，失败返回 NULL 且不改状态。

请先展示实现设计、涉及文件和验证办法，再修改代码。
实现完成后把 best_fit_pmm.c.pending 恢复为 best_fit_pmm.c，
确保原两个测试函数逐字保留；在 pmm.c 的 init_pmm_manager()
选择 best_fit_pmm_manager。保留 lab2 标记；学号由我提供，勿编造。
检查 Makefile 是否自动收集该目录 .c 文件，避免重复加入源文件。

验证：编译并在 QEMU 中触发原 manager.check，确认打印的 manager
确实是 best_fit_pmm_manager，再运行 make grade。补充边界测试可另写，
不要改原测试。解释每处修改及失败原因；没有运行的检查请明确说明。
````

## 2. Buddy System 分配器

````text
在现有 RISC-V ucore Lab2 仓库实现 Buddy System。
先读取 pmm.h、memlayout.h、pmm.c、list.h、default_pmm.c、
已完成的 best_fit_pmm.c 和 Makefile。保留 page_init 与 Best-Fit 原测试。

新增 buddy_pmm.c、buddy_pmm.h，注册 buddy_pmm_manager，满足
struct pmm_manager 所有方法。采用每阶空闲链表，块大小为 2^order 页。
初始区间可能起点不对齐、长度不是 2 的幂；必须完整覆盖其全部可用页，
不管理区间外或保留页，支持框架多次 init_memmap 的语义。

先明确并解释伙伴编号的坐标系。建议以绝对物理页号计算伙伴
buddy_ppn = ppn XOR (1 << order)，转换为 Page 指针前做上下界检查，
并验证伙伴处于已注册管理范围、同阶、空闲且是块首。
只能合并真正伙伴，不能把任意相邻同大小块合并。

约定 alloc_pages(n) 将 n 向上取整为 2 的幂并占用整个块，记录该块
实际阶数及原始请求，free_pages(base,n) 使用原请求但释放实际完整块。
nr_free_pages 统计真实空闲物理页数；申请 3 页占 4 页。
说明元数据怎样存储、容量如何受管理内存上界约束。
不要调用依赖当前分配器的动态分配函数来分配自身元数据。
n=0 断言；不足时返回 NULL 且状态不变；防止取整移位溢出。

接入时默认仍为 Best-Fit，以便保留当前官方评分兼容性；
增加 PMM_USE_BUDDY 编译宏以选择 Buddy，并说明通过现有 Makefile 的
DEFS 参数运行的方法。切换后启动应打印实际使用的 manager 名称。
先给出设计与验证办法，再实施。不要声称未执行的测试已通过。
```
````

## 3. Buddy 专项测试

````text
请先阅读刚生成的 buddy_pmm.c/.h 和它的调用、元数据及释放契约，
再设计并实现相应测试，接到 buddy_pmm_manager.check。
不要改 Best-Fit 两个原测试，不要依赖偶然的物理起点或地址顺序。

覆盖：单页和多阶分配；非 2 的幂请求及真实占用计数；连续拆分；
正序、逆序释放及递归合并；相邻但非伙伴不得合并；非对齐起点、
非幂次长度初始区域的完整覆盖；分配失败状态不变；碎片导致总量
足够但无目标阶块时失败；全释放后计数和可分配能力恢复。
检查同时存活的分配区间互不重叠、均处于管理范围且满足块对齐。

用“计数守恒 + 区间不重叠 + 可重新分配大块”交叉验证，
不要只检查非 NULL。明确每项测试怎样触发对应行为。
测试应隔离状态并完整恢复，不破坏全局 Page 数组和正常启动状态。
初始化边界场景可使用专门的受控测试区域，说明其取得与恢复方法。
无效参数若触发 panic，应作为独立负面测试，不能中断主测试后冒充全部通过。
先给测试矩阵，再实现并实际编译运行；给出真实日志和未验证项目。
````
