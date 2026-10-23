# 开发与验证日志索引

- `lab2-module1-run2.log`：仓库原有模块一运行记录，原文件保留。
- `best-fit/`：Best-Fit编译、启动、评分与补充验证记录。
- `buddy/`：Buddy开发阶段构建、启动、容量及模型验证记录。
- `buddy-check/`：隔离测试矩阵、完整状态恢复、独立负面测试和错误变体验证。
- `archive-manifest.json`：早期48份日志的复制来源、字节数及SHA256；不是执行时间或当时代码版本清单，原清单保留。
- [final-verification-20261023/](final-verification-20261023/README.md)：2026-10-23 09:48对 `7db2f61` 源码的复测，含干净构建、两模式启动、原样评分0/20和兼容参数20/20。

本次文档整理归档已有日志，未再次运行实验。QEMU的设定时间终止需结合成功输出解释。`wrong-count`、`no-coalesce` 为故意错误副本的预期断言结果。官方评分覆盖物理内存初始化和Best-Fit，Buddy结论来自专项验证。

完整上下文和测试范围见 [实验报告](../report.md)，实际终端截图见 [图片索引](../images/README.md)。
