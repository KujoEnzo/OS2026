#!/usr/bin/env python3
import subprocess
import time
import sys
from pathlib import Path
root = Path(__file__).resolve().parents[2]
logs = root.parent / 'report/logs/b'
check_init = '--init' in sys.argv
with (logs / 'debug-serial.log').open('w') as log:
    p = subprocess.Popen(['qemu-system-riscv64', '-machine', 'virt', '-nographic', '-bios',
        'default', '-kernel', 'bin/ucore.img', '-monitor', 'none', '-S', '-gdb', 'tcp:127.0.0.1:1236'], cwd=root, stdout=log, stderr=subprocess.STDOUT)
    try:
        time.sleep(1)
        commands = ['target remote 127.0.0.1:1236']
        if check_init:
            commands += ['break init_main', 'continue', 'p current->pid', 'p current == initproc',
                'p current->parent == idleproc', 'p/x current->pgdir', 'info registers pc sp satp', 'bt',
                'python assert int(gdb.parse_and_eval("current == initproc")) == 1',
                'python assert int(gdb.parse_and_eval("current->pid")) == 1',
                'python print("B_GDB_INIT_ENTRY_PASS")']
        else:
            commands += ['break check_boot_pgdir', 'continue', 'hbreak *0x80200000', 'continue',
                'info registers pc satp scause stval', 'x/6i $pc']
        args = ['gdb-multiarch', '-q', '-nx', '-batch', 'bin/kernel']
        for command in commands: args += ['-ex', command]
        r = subprocess.run(args, cwd=root, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=10)
        (logs / ('init-gdb.log' if check_init else 'debug-gdb.log')).write_bytes(r.stdout)
        print(r.stdout.decode())
    finally:
        p.terminate(); p.wait()
