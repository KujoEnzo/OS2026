# OS2026 实验小组仓库

## 小组成员

| 姓名 | 学号 |
| --- | --- |
| 郝煦享 | 2412695 |
| 田茂炜 | 2412784 |

## 实验导航

每次实验使用独立 labx 分支。main 作为导航入口，不混合不同实验的源码。

| 实验 | 提交分支 | 说明 |
| --- | --- | --- |
| Lab1 | [lab1](https://github.com/KujoEnzo/OS2026/tree/lab1) | 查看对应分支的代码和材料 |
| Lab2 | [lab2](https://github.com/KujoEnzo/OS2026/tree/lab2) | 查看对应分支的代码和材料 |
| Lab3 | [lab3](https://github.com/KujoEnzo/OS2026/tree/lab3) | 查看对应分支的代码和材料 |
| **Lab4** | **[lab4](https://github.com/KujoEnzo/OS2026/tree/lab4)** | A/B 完整实现，兼容评分 30/30，含报告、实际日志和运行截图 |

## Lab4 最终材料

- [完整实验报告](https://github.com/KujoEnzo/OS2026/blob/lab4/report/report.md)
- [项目讲解](https://github.com/KujoEnzo/OS2026/blob/lab4/report/project-guide.md)
- [最终验收与复现命令](https://github.com/KujoEnzo/OS2026/blob/lab4/report/final-validation.md)
- [真实运行截图](https://github.com/KujoEnzo/OS2026/tree/lab4/report/images)
- [实际提示词](https://github.com/KujoEnzo/OS2026/blob/lab4/report/prompt.md)

Lab4 的 30/30 使用 -kernel 启动参数与 gdb-multiarch，准确条件以最终验收为准。开发分支 lab4-a-mm-sched 保留历史，正式提交请使用 lab4。

## 分支目录约定

```text
code/                 # 对应实验源代码及报告入口
report/report.md      # 实验报告
report/prompt.md      # 实际提示词
report/images/        # 真实测试截图
report/logs/          # 原始验证日志
```

Lab4 的 code/ 对应课程 labcodes/lab4；报告在 code/report.md 提供相对链接入口。其他实验历史分支保持原有内容。
