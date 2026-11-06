#!/usr/bin/env python3
"""Exercise the actual trap.c C handlers with host stubs (not CPU emulation)."""
import pathlib
import subprocess
import tempfile

ROOT = pathlib.Path(__file__).resolve().parents[3]

with tempfile.TemporaryDirectory(prefix="lab3-handlers-") as directory:
    tmp = pathlib.Path(directory)
    inc = tmp / "include"
    inc.mkdir()
    (inc / "defs.h").write_text(
        "#include <stdint.h>\n#include <stddef.h>\n#include <stdbool.h>\n")
    for name in ("console.h", "kdebug.h", "memlayout.h", "mmu.h"):
        (inc / name).write_text("")
    # Keep the production trapframe definition and real cause numbers.
    (inc / "trap.h").write_text((ROOT / "code/kern/trap/trap.h").read_text())
    real = (ROOT / "code/libs/riscv.h").read_text().splitlines()
    constants = [s for s in real if s.startswith(("#define IRQ_", "#define CAUSE_", "#define SSTATUS_SPP"))]
    (inc / "riscv.h").write_text("\n".join(constants) + """
#define SIP_SSIP 2
#define write_csr(reg, value) ((void)(value))
#define clear_csr(reg, value) (cleared |= (value))
extern unsigned long cleared;
""")
    (inc / "clock.h").write_text("#include <stddef.h>\nextern volatile size_t ticks;\nvoid clock_set_next_event(void);\n")
    (inc / "sbi.h").write_text("void sbi_shutdown(void);\n")
    # Use host stdio/assert headers; declare the two kernel-specific interfaces.
    source = r'''
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include <stdint.h>
volatile size_t ticks;
unsigned long cleared;
static unsigned armed, shutdowns, panics;
static char output[32768];
static size_t used;
static jmp_buf escape;
int cprintf(const char *format, ...) {
    va_list ap; va_start(ap, format);
    int n = vsnprintf(output + used, sizeof(output) - used, format, ap);
    va_end(ap); assert(n >= 0 && (size_t)n < sizeof(output) - used);
    used += n; return n;
}
void panic(const char *message, ...) { (void)message; panics++; longjmp(escape, 2); }
void clock_set_next_event(void) { armed++; }
void sbi_shutdown(void) { shutdowns++; longjmp(escape, 1); }
#include "PRODUCTION_TRAP"
static void reset_output(void) { used = 0; output[0] = 0; }
int main(void) {
    struct trapframe tf = {0}, saved;
    tf.epc = 0x12345678; tf.cause = ((uintptr_t)1 << 63) | IRQ_S_TIMER;
    saved = tf;
    for (unsigned i = 0; i < 99; i++) interrupt_handler(&tf);
    assert(ticks == 99 && armed == 99 && used == 0);
    interrupt_handler(&tf);
    assert(ticks == 100 && armed == 100 && strstr(output, "100 ticks"));
    assert(memcmp(&tf, &saved, sizeof(tf)) == 0);
    if (setjmp(escape) == 0) {
        while (ticks < 1000) interrupt_handler(&tf);
        assert(0 && "shutdown was not called");
    }
    assert(ticks == 1000 && armed == 1000 && shutdowns == 1 && panics == 0);
    unsigned lines = 0;
    for (char *s = output; (s = strstr(s, "100 ticks")); s += 9) lines++;
    assert(lines == 10);
    puts("PASS timer rearm/count/frame preservation/10 reports/shutdown");

    tf.cause = ((uintptr_t)1 << 63) | IRQ_S_SOFT;
    interrupt_handler(&tf);
    assert(cleared == SIP_SSIP && strstr(output, "Supervisor software interrupt"));
    for (unsigned i = 0; i < 12; i++) {
        if (i != IRQ_S_TIMER) { tf.cause = ((uintptr_t)1 << 63) | i; interrupt_handler(&tf); }
    }
    reset_output(); tf.cause = 15; interrupt_handler(&tf);
    assert(strstr(output, "Unknown interrupt: cause 15") && strstr(output, "trapframe at"));
    puts("PASS interrupt classification/software acknowledgement/fallback");

    uint16_t instructions[] = {0x9002, 0x0000, 0x0073, 0x0010, 0x007b, 0};
    const unsigned offsets[] = {0, 1, 2, 4};
    const unsigned lengths[] = {2, 2, 4, 4};
    for (unsigned i = 0; i < 4; i++) {
        memset(&tf, 0x5a, sizeof(tf));
        tf.status |= SSTATUS_SPP;
        tf.cause = i % 2 ? CAUSE_ILLEGAL_INSTRUCTION : CAUSE_BREAKPOINT;
        tf.epc = (uintptr_t)&instructions[offsets[i]];
        saved = tf; exception_handler(&tf); saved.epc += lengths[i];
        assert(memcmp(&tf, &saved, sizeof(tf)) == 0);
    }
    reset_output(); tf.cause = 20; saved = tf; exception_handler(&tf);
    assert(strstr(output, "trapframe at") && memcmp(&tf, &saved, sizeof(tf)) == 0);
    uint16_t long_insn = 0x1f; tf.cause = CAUSE_ILLEGAL_INSTRUCTION;
    tf.epc = (uintptr_t)&long_insn;
    if (setjmp(escape) == 0) { exception_handler(&tf); assert(0); }
    assert(panics == 1);
    puts("PASS 16/32-bit EPC advance/frame preservation/exception fallback/long instruction rejection");
    return 0;
}
'''
    source = source.replace("PRODUCTION_TRAP", (ROOT / "code/kern/trap/trap.c").as_posix())
    (tmp / "test.c").write_text(source)
    exe = tmp / "test"
    subprocess.run(["gcc", "-std=gnu99", "-Wall", "-Wextra", "-Werror",
                    "-Wno-unused-parameter", "-I", str(inc), str(tmp / "test.c"),
                    "-o", str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
