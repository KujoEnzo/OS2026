#!/usr/bin/env python3
"""Test B in an isolated copy; never alter formal handlers or A functions."""
from pathlib import Path
import argparse
import json
import shutil
import subprocess
import tempfile

parser = argparse.ArgumentParser()
parser.add_argument('--logs', required=True, type=Path)
args = parser.parse_args()
source = Path(__file__).resolve().parents[2]
logs = args.logs.resolve()
logs.mkdir(parents=True, exist_ok=True)
results = {}
with tempfile.TemporaryDirectory(prefix='lab4-b-') as temporary:
    root = Path(temporary) / 'code'
    shutil.copytree(source, root, ignore=shutil.ignore_patterns('bin', 'obj', '__pycache__'))
    proc = root / 'kern/process/proc.c'
    text = proc.read_text()
    marker = '//LAB4 function2 2412784'
    hooks = '''static void *b_alloc(size_t);
static void b_free(void *);
#define kmalloc(n) b_alloc(n)
#define kfree(p) b_free(p)
'''
    assert text.count(marker) == 1
    text = text.replace(marker, marker + '\n' + hooks)
    text += '\n' + (source / 'tools/b_tests/checks.c').read_text()
    proc.write_text(text)
    init = root / 'kern/init/init.c'
    text = init.read_text().replace('int kern_init(void)', 'void b_checks(void);\nint kern_init(void)', 1)
    text = text.replace('proc_init();', 'proc_init(); b_checks();', 1)
    init.write_text(text)
    build = subprocess.run(['make', '-j4'], cwd=root, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    (logs / 'b-build.log').write_bytes(build.stdout)
    results['build'] = build.returncode
    if build.returncode == 0:
        cmd = ['qemu-system-riscv64', '-machine', 'virt', '-nographic', '-bios', 'default',
               '-kernel', 'bin/ucore.img', '-monitor', 'none', '-no-reboot']
        try:
            run = subprocess.run(cmd, cwd=root, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=12)
            output, rc = run.stdout, run.returncode
        except subprocess.TimeoutExpired as e:
            output, rc = e.stdout or b'', 124
        (logs / 'b-qemu.log').write_bytes(output)
        results['qemu_exit'] = rc
        results['checks'] = b'[B PCB/CONTEXT/FAILURE-CLEANUP] PASS' in output
        results['init_main_entered'] = b'this initproc, pid = 1, name = "init"' in output
        results['expected_lab4_exit_panic'] = b'process exit!!' in output
        print(output.decode(errors='replace')[-2500:])
    (logs / 'b-results.json').write_text(json.dumps(results, indent=2) + '\n')
print(results)
raise SystemExit(0 if results.get('checks') and results.get('init_main_entered') else 1)
