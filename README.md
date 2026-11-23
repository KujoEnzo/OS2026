# OS2026 · Lab4

本分支 **lab4** 是小组 Lab4 最终提交分支，包含页映射、内核线程创建、调度与上下文切换的完整实现、报告、提示词、原始日志和真实运行截图。

| 成员 | 学号 | 主要工作 |
| --- | --- | --- |
| 郝煦享（A） | 2412695 | 前置迁移、页映射、调度切换、A 测试与最终整理 |
| 田茂炜（B） | 2412784 | PCB、线程创建与初始化、失败清理、首次运行联调 |

最终功能基线为已合并 B 的 a9b0d53；随后补齐署名、学号、文档和最终验收。开发分支 lab4-a-mm-sched 保留追溯历史，课程提交请使用 lab4。

## 阅读入口

- [完整实验报告](report/report.md)
- [项目讲解与代码阅读路线](report/project-guide.md)
- [最终验收、命令和验证边界](report/final-validation.md)
- [实际提示词记录](report/prompt.md)
- [真实运行截图](report/images/README.md)
- [最新原始日志](report/logs/final-lab4/)

## 构建与验证

需要 Linux/WSL、RISC-V GCC、QEMU、make、Python3，以及评分使用的 gdb-multiarch。

```sh
git clone -b lab4 https://github.com/KujoEnzo/OS2026.git
cd OS2026/code
make -j4
python3 tools/a_tests/run.py --suite all --logs ../report/logs/reproduce-a
python3 tools/b_tests/run.py --logs ../report/logs/reproduce-b
make grade GDB=gdb-multiarch 'QEMU=qemu-system-riscv64 -kernel bin/ucore.img'
```

最终复测：完整构建及 A/B 测试通过，initproc 实际执行，课程**兼容评分 30/30**。不是原样默认无参数评分已通过。

普通运行：

```sh
qemu-system-riscv64 -machine virt -nographic -bios default -kernel bin/ucore.img
```

当前课程 do_exit 保留 panic 占位；线程打印问候后到达 process exit!!，不代表已经实现完整退出回收。本项目不声称支持用户 mm 复制或多核调度。

## 目录与历史

code/ 对应课程 labcodes/lab4；code/report.md 提供报告入口。report/ 包含正文、讲解、提示词、images/ 截图和 logs/final-lab4/ 最终日志。

report/a-*.md、handoff-a.md、旧日志和 docs/superpowers/plans/ 保留历史过程。其中“B 待完成”等属于阶段记录，以当前 README、最终报告与验收为准。
