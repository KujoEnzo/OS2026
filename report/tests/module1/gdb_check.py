"""Executed by GDB's Python interpreter, never imported into the kernel."""
import gdb
import traceback

MASK64 = (1 << 64) - 1
STATUS_MASK = 0x122

def value(expr):
    return int(gdb.parse_and_eval(expr)) & MASK64

def reg(name):
    return value('$' + name)

def equal(name, got, expected):
    if got != expected:
        raise AssertionError('%s: got=0x%016x expected=0x%016x' % (name, got, expected))

def pc_at(symbol):
    equal('stop at '+symbol, reg('pc'), value('(uintptr_t)&'+symbol))

def words(address, count):
    raw = bytes(gdb.selected_inferior().read_memory(address, count * 8))
    return [int.from_bytes(raw[i*8:i*8+8], 'little') for i in range(count)]

try:
    gdb.execute('set pagination off')
    gdb.execute('set confirm off')
    gdb.execute('set architecture riscv:rv64')
    gdb.execute('set remotetimeout 3')
    gdb.execute('file bin/kernel')
    gdb.execute('target remote 127.0.0.1:%d' % value('$module1_port'))
    begin = gdb.Breakpoint('*module1_run')
    failure = gdb.Breakpoint('*module1_fail')
    gdb.execute('continue')
    pc_at('module1_run')
    begin.delete()
    trap_bp = gdb.Breakpoint('*trap')
    ret_bp = gdb.Breakpoint('*__trapret')
    start = value('(uintptr_t)&__trapret')
    instructions = gdb.selected_frame().architecture().disassemble(start, start+256)
    sret = next(i['addr'] for i in instructions if i['asm'].strip() == 'sret')
    sret_bp = gdb.Breakpoint('*0x%x' % sret)
    done = gdb.Breakpoint('*module1_done')
    print('GDB sret instruction address=0x%016x' % sret)
    for case in range(3):
        gdb.execute('continue')
        pc_at('trap')
        equal('case counter', value('module1_case'), case)
        frame = value('module1_frame')
        equal('case%d trap a0' % case, reg('a0'), frame)
        equal('case%d trap sp' % case, reg('sp'), frame)
        equal('ABI alignment', reg('sp') % 16, 0)
        saved = words(frame, 36)
        for x in range(32):
            equal('case%d saved x%d' % (case,x), saved[x], value('module1_expected[%d]' % x))
        equal('saved epc', saved[33], value('module1_fault'))
        equal('saved cause', saved[35], 3)
        equal('saved status', saved[32] & STATUS_MASK, 0x100 | (0x20 if case == 1 else 0))
        equal('entry sscratch', reg('sscratch'), 0)
        print('PASS case%d trap(a0=sp=F=0x%016x), ABI16, saved x0..x31, status, epc, cause' % (case,frame))
        gdb.execute('continue')
        pc_at('__trapret')
        equal('case%d frame base after C' % case, reg('sp'), frame)
        equal('processed frame epc', words(frame+264,1)[0], value('module1_epc'))
        print('PASS case%d C return sp=F, processed epc=0x%016x' % (case,value('module1_epc')))
        gdb.execute('continue')
        equal('stop at sret', reg('pc'), sret)
        for x in range(32):
            equal('case%d restored x%d' % (case,x), reg('x%d' % x), value('module1_returned[%d]' % x))
        equal('processed sepc', reg('sepc'), value('module1_epc'))
        equal('processed status', reg('sstatus') & STATUS_MASK, value('module1_status') & STATUS_MASK)
        equal('restored sscratch', reg('sscratch'), 0)
        print('PASS case%d before sret: x0..x31 exact; ra=%016x sp=%016x a0=%016x a5=%016x t6=%016x' %
              (case,reg('ra'),reg('sp'),reg('a0'),reg('a5'),reg('t6')))
        before = value('module1_status')
        target = value('module1_epc')
        # This GDB/QEMU combination software-steps SRET past its actual target.
        # Stop before the first instruction at the expected return PC instead.
        target_bp = gdb.Breakpoint('*0x%x' % target, temporary=True)
        gdb.execute('continue')
        equal('case%d post-sret pc' % case, reg('pc'), target)
        equal('no extra trap while returning', value('module1_events'), case+1)
        equal('case%d post-sret status' % case, reg('sstatus') & STATUS_MASK, 0x20 | (2 if before & 0x20 else 0))
        equal('case%d post-sret privilege S' % case, reg('priv'), 1)
        equal('case%d post-sret sscratch' % case, reg('sscratch'), 0)
        print('PASS case%d after sret: PC=%016x SIE/SPIE/SPP=%03x priv=S' %
              (case,reg('pc'),reg('sstatus') & STATUS_MASK))
    gdb.execute('continue')
    pc_at('module1_done')
    equal('events', value('module1_events'), 3)
    equal('posts', value('module1_posts'), 3)
    equal('failures', value('module1_failures'), 0)
    print('MODULE1_GDB_PASS cases=3 frame/ABI/GPR/sp/processed-epc/status/privilege verified')
    gdb.execute('detach')
    gdb.execute('quit 0')
except Exception as error:
    print('MODULE1_GDB_FAIL: '+str(error))
    traceback.print_exc()
    gdb.execute('quit 1')
