# OS2026

## Lab4 A 阶段交付

当前开发分支 `lab4-a-mm-sched`。A 已实现页映射和调度切换，独立 QEMU 测试通过；B 的线程创建模块保留待填，正常构建及完整评分尚不能通过。接手请读 [交接说明](report/handoff-a.md)，报告入口在 [report/report.md](report/report.md) 和 [code/report.md](code/report.md)。

在 Linux/WSL 的 `code/` 下执行：

```sh
python3 tools/a_tests/run.py --suite all --logs ../report/logs/reproduce
```

本分支未代填姓名、学号或正式评分结果。

OS2026 实验小组仓库。每个小组使用一个公开仓库；每次实验使用一个独立分支，命名为 `lab1`、`lab2` 等。

每个实验分支应包含：

```text
code/                 # 本次实验实现后的源代码
report/
  report.md           # 实验报告（按课程模板撰写）
  prompt.md           # 本次实验使用的所有提示词汇总
  images/             # 报告中提及的测试结果截图
```

提交前检查报告引用的截图在 `report/images/` 中，且代码与报告都位于对应的 `labx` 分支。Git 不跟踪空目录，`images/` 尚无截图时可在实际提交时创建。
