set pagination off
set confirm off
python
import gdb
try:
    def value(expression):
        return int(gdb.parse_and_eval(expression)) & ((1 << 64) - 1)
    port = value('$module2_port')
    gdb.execute('target remote 127.0.0.1:%d' % port)
    gdb.Breakpoint('exception_handler', temporary=True)
    gdb.execute('continue')
    frame = value('$a0')
    old_epc = value('((struct trapframe *)%d)->epc' % frame)
    old_sp = value('((struct trapframe *)%d)->gpr.sp' % frame)
    insn = value('*(unsigned short *)%d' % old_epc)
    length = 2 if insn & 3 != 3 else 4
    assert value('((struct trapframe *)%d)->cause' % frame) == 3
    gdb.Breakpoint('__trapret', temporary=True)
    gdb.execute('continue')
    new_epc = value('((struct trapframe *)%d)->epc' % frame)
    assert new_epc == old_epc + length, (old_epc, new_epc, length)
    assert value('$sp') == frame
    gdb.Breakpoint('*0x%x' % new_epc, temporary=True)
    gdb.execute('continue')
    assert value('$pc') == new_epc
    assert value('$sp') == old_sp
    print('MODULE2_GDB_PASS frame=0x%x epc=0x%x->0x%x sp=0x%x' %
          (frame, old_epc, new_epc, old_sp))
except Exception as error:
    print('MODULE2_GDB_FAIL: %s' % error)
    gdb.execute('quit 1')
end
quit 0
