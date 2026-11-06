#!/usr/bin/env python3
"""Run unchanged course grading, then optional -kernel compatibility checks.

All builds run in copies; the repository and its module-one logs stay intact.
This script does not award a score for compatibility runs.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import signal
import socket
import subprocess
import tempfile

parser = argparse.ArgumentParser()
parser.add_argument("--out", required=True, type=Path)
parser.add_argument("--compat", action="store_true")
parser.add_argument("--defs", default="", help="explicit extra compiler flags, recorded in logs")
args = parser.parse_args()
repo = Path(__file__).resolve().parents[3]
out = args.out.resolve()
out.mkdir(parents=True, exist_ok=False)
results = {}


def run(name, command, cwd, timeout=120):
    with (out / (name + ".log")).open("w") as log:
        log.write("COMMAND: " + repr(command) + "\nCWD: " + str(cwd) + "\n")
        log.flush()
        proc = subprocess.Popen(command, cwd=cwd, stdout=log, stderr=subprocess.STDOUT,
                                start_new_session=True)
        try:
            rc = proc.wait(timeout=timeout)
        except subprocess.TimeoutExpired:
            os.killpg(proc.pid, signal.SIGTERM)
            try:
                proc.wait(timeout=3)
            except subprocess.TimeoutExpired:
                os.killpg(proc.pid, signal.SIGKILL)
                proc.wait()
            rc = 124
        results[name] = {"returncode": rc, "command": command}
        return rc


def copy_code(destination):
    shutil.copytree(repo / "code", destination,
                    ignore=shutil.ignore_patterns("obj", "bin", ".qemu*.out"))


results["baseline"] = "fbe7f5c5847a4d3b182168a615bf2e68c3f6e887"
results["trap_sha256"] = hashlib.sha256((repo / "code/kern/trap/trap.c").read_bytes()).hexdigest()
try:
    run("environment", ["sh", "-c", "uname -a; riscv64-unknown-elf-gcc --version; "
        "qemu-system-riscv64 --version; gdb-multiarch --version"], repo)
    with tempfile.TemporaryDirectory(prefix="lab3-module2-") as temporary:
        base = Path(temporary)
        formal = base / "formal"
        copy_code(formal)
        build = run("build", ["make", "-j2", "DEFS=" + args.defs], formal)
        if build != 0:
            raise RuntimeError("formal build failed; inspect build.log")
        run("course-grade", ["make", "grade", "DEFS=" + args.defs], formal, 180)
        if args.compat:
            injected = base / "injected"
            copy_code(injected)
            if run("inject", ["python3", str(injected / "tools/inject.py"), str(injected)], injected):
                raise RuntimeError("course injector failed")
            cases = [
                ("context", "GRADE_LAB3_CONTEXT", 8,
                 ["[Test 1] Trap entry and breakpoint return: PASS", "Breakpoint at"]),
                ("illegal", "GRADE_LAB3_ILLEGAL", 8,
                 ["[Test 2] Illegal instruction return: PASS", "Illegal instruction at",
                  "Supervisor software interrupt", "Unknown interrupt: cause 15", "cause    0x00000014"]),
                ("timer", "GRADE_LAB3_TIMER", 15,
                 ["[Test 3] Sustained supervisor timer: PASS", "100 ticks", "ticks=200"]),
            ]
            for name, define, seconds, markers in cases:
                run(name + "-clean", ["make", "clean"], injected)
                rc = run(name + "-build", ["make", "-j2", "DEFS=" + args.defs + " -D" + define], injected)
                if rc != 0:
                    raise RuntimeError(name + " build failed")
                run(name + "-qemu", ["qemu-system-riscv64", "-machine", "virt", "-nographic",
                    "-bios", "default", "-kernel", "bin/ucore.img", "-monitor", "none",
                    "-no-reboot"], injected, seconds)
                output = (out / (name + "-qemu.log")).read_text(errors="replace")
                results[name + "-markers"] = {m: m in output for m in markers}
                if name == "context":
                    with socket.socket() as sock:
                        sock.bind(("127.0.0.1", 0))
                        port = sock.getsockname()[1]
                    command = ["qemu-system-riscv64", "-machine", "virt", "-nographic",
                        "-bios", "default", "-kernel", "bin/ucore.img", "-monitor", "none",
                        "-S", "-gdb", "tcp:127.0.0.1:" + str(port)]
                    with (out / "gdb-qemu.log").open("w") as log:
                        proc = subprocess.Popen(command, cwd=injected, stdout=log,
                            stderr=subprocess.STDOUT, start_new_session=True)
                        try:
                            run("gdb-return", ["gdb-multiarch", "-q", "-nx", "-batch",
                                "bin/kernel", "-ex", "set $module2_port=" + str(port),
                                "-x", str(repo / "report/tests/module2/gdb_return.gdb")], injected, 20)
                        finally:
                            if proc.poll() is None:
                                os.killpg(proc.pid, signal.SIGTERM)
                                proc.wait(timeout=5)
            run("shutdown-clean", ["make", "clean"], formal)
            if run("shutdown-build", ["make", "-j2", "DEFS=" + args.defs], formal) != 0:
                raise RuntimeError("shutdown build failed")
            rc = run("shutdown-qemu", ["qemu-system-riscv64", "-machine", "virt", "-nographic",
                "-bios", "default", "-kernel", "bin/ucore.img", "-monitor", "none",
                "-no-reboot"], formal, 25)
            output = (out / "shutdown-qemu.log").read_text(errors="replace")
            results["shutdown"] = {"qemu_exit": rc, "reports": output.count("100 ticks"),
                "passed": rc == 0 and output.count("100 ticks") == 10 and "kernel panic" not in output}
            if not all(all(results[n + "-markers"].values()) for n, _, _, _ in cases):
                raise RuntimeError("compatibility markers missing")
            if not results["shutdown"]["passed"]:
                raise RuntimeError("shutdown check failed")
            if results["gdb-return"]["returncode"] != 0:
                raise RuntimeError("GDB return check failed")
        results["complete"] = results["course-grade"]["returncode"] == 0
except (RuntimeError, OSError) as error:
    results["error"] = str(error)
    results["complete"] = False
finally:
    (out / "results.json").write_text(json.dumps(results, indent=2, ensure_ascii=False) + "\n")
print(json.dumps(results, indent=2, ensure_ascii=False))
raise SystemExit(0 if results.get("complete") else 1)
