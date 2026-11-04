#!/usr/bin/env python3
"""Compile the formal tree and verify that only the two approved regions changed."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess

p=argparse.ArgumentParser()
p.add_argument('--repo',type=Path,required=True)
p.add_argument('--out',type=Path,required=True)
a=p.parse_args()
repo=a.repo.resolve(); out=a.out.resolve(); out.mkdir(parents=True,exist_ok=True)
base='3335e0e6e1663e33cba41c3d5645f6b9be0416f0'
code=repo/'code'
def git(*args): return subprocess.check_output(['git','-C',str(repo),*args])
paths=git('ls-tree','-r','-z','--name-only',base,'code').decode().rstrip('\0').split('\0')
changed=[]
for rel in paths:
    old=git('show',base+':'+rel)
    if (repo/rel).read_bytes()!=old: changed.append(rel)
assert changed==['code/kern/trap/trapentry.S'],changed
source=(code/'kern/trap/trapentry.S').read_text()
old=git('show',base+':code/kern/trap/trapentry.S').decode()
marker='    #lab3 function1 YOUR CODE'
restore=source.split('    .macro RESTORE_ALL\n\n',1)[1].split('\n\n    .endm',1)[0]
entry=source.split('__alltraps:\n    SAVE_ALL\n\n',1)[1].split('\n    # sp should',1)[0]
without=source.replace(restore,marker,1).replace(entry,marker,1)
assert without==old,'Changes outside the two allowed implementation regions'
# No test sources or other untracked production files can enter the build.
actual={f.relative_to(repo).as_posix() for f in code.rglob('*')
        if f.is_file() and 'obj' not in f.parts and 'bin' not in f.parts}
# The required engineering-root report is documentation, never a build input.
actual.discard('code/lab3.md')
assert actual==set(paths), actual^set(paths)

def run(name,cmd,expect):
    with (out/name).open('w') as f:
        f.write('$ '+' '.join(cmd)+'\n'); f.flush()
        r=subprocess.run(cmd,cwd=code,stdout=f,stderr=subprocess.STDOUT)
        f.write('\nEXIT=%d\n'%r.returncode)
    assert r.returncode==expect,(name,r.returncode)

run('clean.log',['make','clean'],0)
run('full-build.log',['make','-k','-j2'],2)
build=(out/'full-build.log').read_text()
assert build.count('error:')==2,build
assert "implicit declaration of function 'interrupt_handler'" in build
assert "implicit declaration of function 'exception_handler'" in build
assert not (code/'bin/kernel').exists() and not (code/'bin/ucore.img').exists()
run('trapentry-build.log',['make','-B','obj/kern/trap/trapentry.o'],0)
run('trapentry-disassembly.log',['riscv64-unknown-elf-objdump','-dr','obj/kern/trap/trapentry.o'],0)
dis=(out/'trapentry-disassembly.log').read_text()
rest=dis.split('<__trapret>:',1)[1]
instructions=[line.split('\t')[-1].strip() for line in rest.splitlines() if '\t' in line]
assert 'sstatus,s0' in rest and 'sepc,s1' in rest
assert 'sp,16(sp)' in rest
assert instructions[-2:] == ['sp,16(sp)','sret'],instructions[-4:]
assert 'R_RISCV_CALL' in dis and 'trap' in dis and 'a0,sp' in dis
run('layout-check.log',['riscv64-unknown-elf-gcc','-mcmodel=medany','-std=gnu99','-Werror','-Wall',
    '-nostdinc','-Ilibs','-Ikern/trap','-c','../report/sources/trapframe-layout-check.c',
    '-o','obj/module1-layout.o'],0)
with (out/'environment.log').open('w') as f:
    for cmd in [['uname','-srmo'],['riscv64-unknown-elf-gcc','--version'],['make','--version'],
                ['qemu-system-riscv64','--version'],['gdb-multiarch','--version']]:
        f.write('$ '+' '.join(cmd)+'\n'); f.flush()
        subprocess.run(cmd,stdout=f,stderr=subprocess.STDOUT,check=True)
summary={'baseline':base,'formal_code_changed':changed,'only_two_allowed_regions_changed':True,
         'other_formal_sources_identical_to_baseline':True,'no_test_sources_in_code':True,
         'trapentry_sha256':hashlib.sha256(source.encode()).hexdigest(),
         'object_build_exit':0,'layout_assertions':43,'layout_exit':0,
         'disassembly_checked':True,'full_build_exit':2,'module2_missing_handlers_only':True,
         'formal_kernel_produced':False}
(out/'integrity.json').write_text(json.dumps(summary,indent=2)+'\n')
print(json.dumps(summary,indent=2))
