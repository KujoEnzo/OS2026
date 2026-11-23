# Lab4 最终验收

日期：2026-11-23。提交分支：lab4。郝煦享（2412695）、田茂炜（2412784）。功能基线 a9b0d53；本轮补齐身份、文档、截图与复测，不改变 A/B 算法。

## 实际结果

| 检查 | 结果 | 证据 |
| --- | --- | --- |
| 完整构建 | 退出 0 | [build.log](logs/final-lab4/build.log) |
| A PMM/Schedule | 两组 ALL PASS，退出 0 | [a-tests.log](logs/final-lab4/a-tests.log) |
| B PCB/现场/失败清理 | PASS，退出 0 | [b-tests.log](logs/final-lab4/b-tests.log) |
| 实际 initproc | pid1、Hello world、Bye | [结构化结果](logs/final-lab4/b-tests/b-results.json) |
| 课程兼容评分 | 30/30，退出 0 | [grade.log](logs/final-lab4/grade.log) |
| 姓名与代码标记 | A=郝煦享/2412695，B=田茂炜/2412784 | 核心源码无 YOUR CODE |
| 实际运行截图 | 已拍摄并引用 | [截图索引](images/README.md) |

命令和退出码另存于 logs/final-lab4/*-exit.json；当前环境见 [environment.log](logs/final-lab4/environment.log)。

## 复现

在 Linux/WSL 的 code/ 下：

```sh
make -j4
python3 tools/a_tests/run.py --suite all --logs ../report/logs/reproduce-a
python3 tools/b_tests/run.py --logs ../report/logs/reproduce-b
make grade GDB=gdb-multiarch 'QEMU=qemu-system-riscv64 -kernel bin/ucore.img'
```

实际评分另用唯一 GRADE_TMP 目录，避免并行任务共用固定路径，见 grade-exit.json。不能把结果写成原样默认无参数 make grade 已通过。课程回调签名 warning 保留，不宣称零警告。

四个独立评分文件与课程原包一致；框架非叶 PTE 自检有明确兼容修正，不将其掩盖为全部自检未变。

## 截图来源

在 WSL 私有虚拟显示上启动真实 Xterm，执行当前代码实际测试，使用 xwd 抓取 framebuffer，再仅无损转换像素格式为 PNG。没有把旧日志渲染成图片。截图逐张检查，均可见 PASS/30分与退出码；完整输出另存于 final-lab4。

X11 依赖仅下载并解压到本任务 scratch，不全局安装；临时显示、终端进程及辅助路径在拍摄结束后清理。

## 边界

- do_exit 保留课程 panic 占位，不声称完整退出回收。
- 当前单 hart 内核线程，不支持用户 mm 复制、SMP 或完整抢占时间片。
- 未新增真实时钟打断扫描的压力测试及全局 TLB 失效独立动态验证，限制已在报告说明。
- a-*、handoff-a 及旧日志为历史阶段记录，不代表当前功能缺失。
