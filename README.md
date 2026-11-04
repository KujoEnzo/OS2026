# OS2026

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

## Lab3 基线准备

本分支使用 2026 课程 Lab3 原始框架，仅迁移必要的 Lab2 代码；模块一已实现并完成受控验证，模块二仍待实现。
来源、迁移边界和实际构建结果见 [基线记录](report/lab3-baseline.md)，本人负责的模块一接口与验收要求见 [模块一规格](report/module1-spec.md)。
模块一实现、复现命令、反例及实际日志见 [模块一验证记录](report/module1-validation.md)。
`report/report.md`、`report/prompt.md` 及原有截图、日志为保留的 Lab2 历史材料，不代表 Lab3 完成情况。
