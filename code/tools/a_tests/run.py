#!/usr/bin/env python3
"""Run real A functions in QEMU, without implementing B's creation functions."""
import argparse
import os
from pathlib import Path
import re
import shutil
import subprocess
import tempfile


def run(suite, logs, mutation=None):
    source = Path(__file__).resolve().parents[2]
    tests = Path(__file__).resolve().parent
    logs.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="lab4-a-") as directory:
        root = Path(directory) / "code"
        shutil.copytree(source, root, ignore=shutil.ignore_patterns("obj", "bin", "__pycache__"))
        if mutation == "s11-lw":
            switch = root / "kern/process/switch.S"
            text = switch.read_text()
            old = "LOAD s11, 13*REGBYTES(a1)"
            if text.count(old) != 1:
                raise RuntimeError("Cannot uniquely apply the s11-lw negative control")
            switch.write_text(text.replace(old, "lw s11, 13*REGBYTES(a1)"))
        target = root / "kern/a_test"
        target.mkdir()
        for name in ("a_test.c", "registers.S"):
            shutil.copyfile(tests / name, target / name)
        # Use the unmodified official PMM test, discarding its unused B tests.
        shutil.copyfile(root / "tools/test_lab4.c", target / "official.c")
        (root / "kern/init/init.c").write_text(
            '#include <defs.h>\n#include <console.h>\n#include <dtb.h>\n'
            '#include <pmm.h>\n#include <trap.h>\n#include <vmm.h>\n'
            'void a_test_main(void) __attribute__((noreturn));\n'
            'int kern_init(void) { extern char edata[], end[];\n'
            'for (char *p = edata; p < end; p++) *p = 0;\n'
            'dtb_init(); cons_init(); idt_init(); pmm_init(); vmm_init(); a_test_main(); }\n'
        )
        makefile = root / "Makefile"
        makefile.write_text(makefile.read_text().replace(
            "KCFLAGS\t\t+=", "KINCLUDE += kern/a_test\nKSRCDIR += kern/a_test\nKCFLAGS\t\t+=", 1))
        command = ["make", "-j4", "DEFS=-DA_TEST_" + suite.upper()]
        result = subprocess.run(command, cwd=root, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        (logs / (suite + "-build.log")).write_bytes(result.stdout)
        if result.returncode:
            print(result.stdout.decode(errors="replace")[-5000:])
            print(f"{suite}: BUILD FAIL ({result.returncode})")
            return False
        assembly = (root / "obj/kernel.asm").read_text()
        excerpts = []
        for symbol in ("proc_run", "switch_to", "schedule", "cpu_idle"):
            start = re.search(r"(?m)^[0-9a-f]+ <" + symbol + r">:\n", assembly)
            if start:
                following = re.search(r"(?m)^[0-9a-f]+ <[^>]+>:\n", assembly[start.end():])
                end = start.end() + following.start() if following else len(assembly)
                excerpts.append(assembly[start.start():end])
        (logs / (suite + "-disassembly.log")).write_text("\n".join(excerpts))
        command = [os.environ.get("QEMU", "qemu-system-riscv64"), "-machine", "virt", "-nographic",
                   "-bios", "default", "-kernel", "bin/ucore.img"]
        try:
            result = subprocess.run(command, cwd=root, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=15)
            output, exitcode = result.stdout, result.returncode
        except subprocess.TimeoutExpired as error:
            output, exitcode = error.stdout or b"", 124
        (logs / (suite + "-qemu.log")).write_bytes(output)
        success = exitcode == 0 and f"[A {suite.upper()}] ALL PASS".encode() in output and b"kernel panic" not in output
        print(output.decode(errors="replace")[-4500:])
        print(f"{suite}: {'PASS' if success else 'FAIL'} (QEMU exit {exitcode})")
        return success


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--suite", choices=("pmm", "schedule", "all"), default="all")
    parser.add_argument("--logs", type=Path, default=Path("a-test-logs"))
    parser.add_argument("--mutation", choices=("s11-lw",), help="negative control in temporary copy only")
    args = parser.parse_args()
    suites = ("pmm", "schedule") if args.suite == "all" else (args.suite,)
    results = [run(suite, args.logs.resolve(), args.mutation) for suite in suites]
    raise SystemExit(0 if all(results) else 1)
