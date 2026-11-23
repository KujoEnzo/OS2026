#!/usr/bin/env python3
"""Reproduce build and compatibility grading from a tracked Git snapshot."""
from pathlib import Path
import io
import subprocess
import tarfile
import tempfile
repo = Path(__file__).resolve().parents[3]
logs = repo / 'report/logs/b'
archive = subprocess.check_output(['git', 'archive', 'HEAD', 'code'], cwd=repo)
with tempfile.TemporaryDirectory(prefix='lab4-clean-') as tmp:
    with tarfile.open(fileobj=io.BytesIO(archive)) as tar:
        tar.extractall(tmp, filter='data')
    root = Path(tmp) / 'code'
    for name, cmd in [('clean-checkout-build', ['make', '-j4']),
                      ('clean-checkout-grade', ['make', 'grade', 'GDB=gdb-multiarch',
                       'QEMU=qemu-system-riscv64 -kernel bin/ucore.img'])]:
        with (logs / (name + '.log')).open('w') as log:
            log.write('COMMAND: ' + repr(cmd) + '\n'); log.flush()
            r = subprocess.run(cmd, cwd=root, stdout=log, stderr=subprocess.STDOUT, timeout=70)
        print(name, r.returncode)
        if r.returncode: raise SystemExit(r.returncode)
    print((logs / 'clean-checkout-grade.log').read_text()[-350:])
