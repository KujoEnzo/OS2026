# Lab3 实验报告入口

当前分支为 `lab3`。工程根目录是 `code/`（对应课程的 `labcodes/lab3`），正文见 **[code/lab3.md：模块一报告](../code/lab3.md)**。

- [模块一规格](module1-spec.md)、[基线来源与迁移](lab3-baseline.md)
- [实际提示词与关键修正](prompt.md)
- [模块一验证记录](module1-validation.md)、[先写的验收断言](module1-assertions.md)
- **[田茂炜交接说明](handoff.md)**：环境、命令、日志、已知问题和模块二清单

模块一已实现并通过原受控测试；模块二现已补全 C 处理器并通过主机接口桩测试。
用户已在 Ubuntu 完成 RISC-V 构建、三组兼容行为检查、GDB返回检查及正常版关机验证，
并回传独立 `-kernel` 兼容评分副本100/100。原样课程评分失败，两个结果分别保留。
模块二作者田茂炜（2412784），function2标记已填写；模块一作者学号待其本人补齐。
完整Ubuntu原始日志待从虚拟机追加，当前归档回传摘要。

- [模块二实现、规格、原理和当前验证状态](module2.md)
- [模块二主机测试](tests/module2/host_check.py)
- [Ubuntu 完整验证脚本](tests/module2/validate.py)
- [Ubuntu实测和100分回传摘要（非原始日志）](logs/module2/ubuntu-reported.md)

本文件此前是Lab2报告。其原文及相关源码保留在Git历史，不将Lab2成绩算作Lab3结果：[固定Lab2提交的报告](https://github.com/KujoEnzo/OS2026/blob/7a79a86b81ef33301a28eef87db85b2ae45cb21f/report/report.md)、[原提示词](https://github.com/KujoEnzo/OS2026/blob/7a79a86b81ef33301a28eef87db85b2ae45cb21f/report/prompt.md)。原有Lab2截图和日志仍保留在仓库。
