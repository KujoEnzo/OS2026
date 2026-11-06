# Ubuntu 验证回传摘要

日期：2026-11-05。来源：田茂炜在本次聊天粘贴的终端输出。
本文件是整理后的回传证据，不是从Ubuntu复制出的完整原始日志。
桌面端尝试SSH读取虚拟机时因没有登录凭据失败，未能直接取得原始文件。

## 第一轮 validate.py

用户执行：

```sh
python3 report/tests/module2/validate.py --out /tmp/lab3-module2-validation --compat
```

回传基线：fbe7f5c5847a4d3b182168a615bf2e68c3f6e887。
回传 trap.c SHA-256：59fa9af0ffeb7b99cb657954140d236d06876a2b8d41ce48859c919b9db33b73。

| 检查 | 原回传结果 |
| --- | --- |
| build | returncode=0 |
| course-grade | returncode=2 |
| context-markers | Trap entry and breakpoint return: PASS；Breakpoint at 均true |
| context-qemu | returncode=124，达到驱动时限；不是自然退出 |
| gdb-return | returncode=0 |
| illegal-markers | Illegal instruction return: PASS、Illegal instruction at、Supervisor software interrupt、Unknown interrupt: cause 15、cause 0x00000014 均true |
| illegal-qemu | returncode=124，达到驱动时限 |
| timer-markers | Sustained supervisor timer: PASS、100 ticks、ticks=200 均true |
| timer-qemu | returncode=0 |
| shutdown | qemu_exit=0，reports=10，passed=true |
| complete | false，因原样课程评分失败而保留 |

原样评分三组均编译成功，但未找到运行通过标记，Total Score: 0/100。
原样串口文件未能取回，不能断言其Next Address的实际值。

## 独立兼容评分

副本路径：`/tmp/lab3-grade-compat-kA5o7O`。
仅将副本tools/grade.sh的generic loader参数改为`-kernel bin/ucore.img`，
保留测试代码、评分标记、分值和时限。用户回传grep确认副本确实使用-kernel。

首次兼容评分仍为0/100：三份串口能看到Next Address=0x80200000、内核启动和100 ticks，
没有课程测试标记，说明测试未执行。随后指导检查测试注入/编译宏，用户回传最终结果。
未收到中间检查输出，无法确定是注入缺失还是宏未生效，不能写成已确认的唯一原因。

用户最终粘贴的原文：

```text
Running re-armed supervisor timer and repeated trap return...
  - re-armed supervisor timer and repeated trap return: PASS (40/40)
Total Score: 100/100
```

结论：兼容启动副本的课程评分100/100；正式原样评分保持失败记录。
完整grade-compat-run2.log和三个.qemu输出位于用户虚拟机，应追加归档至course-compat/。

## 建议追加归档

在原Ubuntu终端、GRADE_COPY变量仍有效时：

```sh
cd /home/tmw/os-lab/OS2026
mkdir -p report/logs/module2/course-compat
cp "$GRADE_COPY/grade-compat-run2.log" report/logs/module2/course-compat/
cp "$GRADE_COPY"/.qemu*.out report/logs/module2/course-compat/
diff -u code/tools/grade.sh "$GRADE_COPY/tools/grade.sh" > report/logs/module2/course-compat/startup-change.diff
```

不要覆盖第一轮results.json的complete=false，也不要将本摘要重命名为原始评分日志。
