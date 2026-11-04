#!/usr/bin/env python3
"""Reproduce Module 1 tests in fresh temporary copies, never patch formal code.

Usage: python3 report/tests/module1/run.py --repo . --out /tmp/module1-logs [--red]
--red tests HEAD's current empty implementation during the initial RED stage.
Without --red, run the formal implementation, one stack-restore mutant, then
rerun the correct implementation. All QEMU/GDB children are bounded and reaped.
"""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import socket
import subprocess
import tempfile
import time

HERE = Path(__file__).resolve().parent
P = argparse.ArgumentParser()
P.add_argument('--repo',type=Path,required=True)
P.add_argument('--out',type=Path,required=True)
P.add_argument('--red',action='store_true')
args = P.parse_args()
repo = args.repo.resolve()
out = args.out.resolve()
out.mkdir(parents=True,exist_ok=True)
before = {p.relative_to(repo/'code').as_posix():hashlib.sha256(p.read_bytes()).hexdigest()
          for p in (repo/'code').rglob('*') if p.is_file() and 'obj' not in p.parts and 'bin' not in p.parts}

def command(cmd,cwd,log,timeout=25):
    with log.open('w') as f:
        f.write('$ '+' '.join(map(str,cmd))+'\n'); f.flush()
        try:
            r = subprocess.run(cmd,cwd=cwd,stdout=f,stderr=subprocess.STDOUT,timeout=timeout)
            code = r.returncode
        except subprocess.TimeoutExpired:
            f.write('\nTIMEOUT\n')
            code = 124
        f.write('\nEXIT=%d\n' % code)
    return code

def static_assertions(source):
    restore = source.split('.macro RESTORE_ALL',1)[1].split('.endm',1)[0]
    entry = source.split('__alltraps:',1)[1].split('.globl __trapret',1)[0]
    required = ['csrw sstatus','csrw sepc','LOAD x2, 2*REGBYTES(sp)']
    errors = [s for s in required if s not in restore]
    if 'call trap' not in entry and 'jal trap' not in entry: errors.append('call trap')
    return errors

initial = (repo/'code/kern/trap/trapentry.S').read_text()
errors = static_assertions(initial)
(out/'static-gate.log').write_text('Missing required contracts: '+repr(errors)+'\n')
if not args.red: assert not errors, errors
else: assert errors, 'RED stage unexpectedly has an implementation'

def one(label,mutant=False):
    tmp = Path(tempfile.mkdtemp(prefix='os2026-module1-'+label+'-'))
    copy = tmp/'code'
    shutil.copytree(repo/'code',copy,ignore=shutil.ignore_patterns('obj','bin'))
    for name in ['module1_test.c','module1_test.h','module1_harness.S']:
        shutil.copyfile(HERE/name,copy/'kern/init'/name)
    trap = copy/'kern/trap/trap.c'
    text = trap.read_text()
    anchor = '//lab3 function2 YOUR CODE'
    assert text.count(anchor)==1
    text = text.replace(anchor,'''/* TEST COPY ONLY: no real module-two implementation. */
#include <module1_test.h>
void interrupt_handler(struct trapframe *tf) { module1_fail("unexpected interrupt", tf->cause, CAUSE_BREAKPOINT); }
void exception_handler(struct trapframe *tf) { module1_handler(tf); }
''')
    # Only this test copy needs the test header directory for trap.c.
    text = text.replace('#include <module1_test.h>','#include "../init/module1_test.h"')
    trap.write_text(text)
    init = copy/'kern/init/init.c'
    text = init.read_text()
    text = '#include "module1_test.h"\n'+text
    anchor = '    clock_init();'
    assert text.count(anchor)==1
    init.write_text(text.replace(anchor,'    module1_run();  // TEST COPY ONLY, before timer setup\n'+anchor))
    if mutant:
        entry = copy/'kern/trap/trapentry.S'
        text = entry.read_text()
        anchor = 'LOAD x2, 2*REGBYTES(sp)'
        assert text.count(anchor)==1
        entry.write_text(text.replace(anchor,'addi sp, sp, 36*REGBYTES'))
    edits = {}
    for p in copy.rglob('*'):
        if p.is_file():
            rel = p.relative_to(copy).as_posix()
            digest = hashlib.sha256(p.read_bytes()).hexdigest()
            if before.get(rel) != digest: edits[rel]=digest
    (out/(label+'-copy-edits.json')).write_text(json.dumps(edits,indent=2)+'\n')
    assert set(edits) == {'kern/init/init.c','kern/trap/trap.c','kern/init/module1_test.c',
                         'kern/init/module1_test.h','kern/init/module1_harness.S'} | ({'kern/trap/trapentry.S'} if mutant else set())
    build = command(['make','-j2'],copy,out/(label+'-build.log'))
    if build != 0: raise RuntimeError(label+' test-copy build failed, inspect log')
    command(['riscv64-unknown-elf-objdump','-d','--disassemble=__trapret','bin/kernel'],
            copy,out/(label+'-linked-disassembly.log'))
    with socket.socket() as s:
        s.bind(('127.0.0.1',0)); port=s.getsockname()[1]
    qcmd=['qemu-system-riscv64','-machine','virt','-nographic','-bios','default',
          '-kernel','bin/ucore.img','-monitor','none','-no-reboot',
          '-S','-gdb','tcp:127.0.0.1:%d'%port]
    with (out/(label+'-qemu.log')).open('w') as qlog:
        qlog.write('$ '+' '.join(qcmd)+'\n'); qlog.flush()
        q = subprocess.Popen(qcmd,cwd=copy,stdout=qlog,stderr=subprocess.STDOUT)
        try:
            # Only a short readiness retry; never consume the GDB protocol socket.
            time.sleep(0.2)
            gdb = command(['gdb-multiarch','-q','-nx','-batch',
                           '-ex','set $module1_port=%d'%port,'-x',str(HERE/'gdb_check.py')],
                          copy,out/(label+'-gdb.log'),timeout=20)
        finally:
            q.terminate()
            try: q.wait(timeout=3)
            except subprocess.TimeoutExpired: q.kill(); q.wait()
            qlog.write('\nQEMU_STOPPED_BY_TEST_DRIVER exit=%d\n'%q.returncode)
    result={'label':label,'build_exit':build,'gdb_exit':gdb,'scratch_copy':str(copy),'mutant':mutant}
    print(json.dumps(result),flush=True)
    return result

results=[]
if args.red:
    r=one('red'); results.append(r)
    text=(out/'red-gdb.log').read_text()
    missing_call = 'stop at trap' in text or 'No symbol "trap"' in text
    assert r['gdb_exit']==1 and missing_call and 'MODULE1_GDB_FAIL' in text, text[-2000:]
else:
    for label,mutant in [('positive',False),('mutant-sp',True),('recheck',False)]:
        r=one(label,mutant); results.append(r)
        text=(out/(label+'-gdb.log')).read_text()
        serial=(out/(label+'-qemu.log')).read_text()
        if mutant:
            assert r['gdb_exit']==1 and 'case2 restored x2' in text and 'MODULE1_GDB_FAIL' in text,text[-2000:]
        else:
            assert r['gdb_exit']==0 and 'MODULE1_GDB_PASS' in text,text[-2000:]
            assert 'MODULE1_DYNAMIC_PASS cases=3 events=3 posts=3' in serial,serial[-2000:]
after = {p.relative_to(repo/'code').as_posix():hashlib.sha256(p.read_bytes()).hexdigest()
         for p in (repo/'code').rglob('*') if p.is_file() and 'obj' not in p.parts and 'bin' not in p.parts}
assert before==after, 'Formal source was mutated by the test runner'
(out/'results.json').write_text(json.dumps({'runs':results,'formal_source_unchanged':True,
    'trapentry_sha256':before['kern/trap/trapentry.S']},indent=2)+'\n')
print('RED_CONFIRMED' if args.red else 'POSITIVE_MUTANT_RECHECK_CONFIRMED')
