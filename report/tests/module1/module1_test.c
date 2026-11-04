/* TEST COPY ONLY. Never compiled into the formal code/ tree. */
#include <defs.h>
#include <riscv.h>
#include <stdio.h>
#include <module1_test.h>

extern char module1_stack_bottom[], module1_stack_top[], module1_alt_top[];
extern char module1_fault0[], module1_after0[], module1_fault1[], module1_after1[];
extern char module1_fault2[], module1_after2[];
extern uintptr_t module1_observed[36];

volatile uintptr_t module1_case, module1_events, module1_posts, module1_failures;
uintptr_t module1_expected[32], module1_returned[32];
uintptr_t module1_frame, module1_fault, module1_epc, module1_status;
static const uintptr_t guard = 0x789abcdef0123456ULL;

void __attribute__((noreturn,noinline))
module1_fail(const char *what, uintptr_t got, uintptr_t expected) {
    module1_failures++;
    cprintf("MODULE1_FAIL case=%lu %s got=0x%016lx expected=0x%016lx\n",
            module1_case, what, got, expected);
    for (;;) asm volatile("" ::: "memory");
}

static void check(const char *name, uintptr_t got, uintptr_t expected) {
    if (got != expected) module1_fail(name, got, expected);
}

void module1_prepare(unsigned long id) {
    module1_case = id;
    for (int i = 0; i < 32; i++) module1_expected[i] = 0x1122334400000100ULL + i;
    module1_expected[0] = 0;
    module1_expected[2] = (uintptr_t)module1_stack_top;
    asm volatile("mv %0, gp" : "=r"(module1_expected[3]));
    asm volatile("mv %0, tp" : "=r"(module1_expected[4]));
    module1_frame = (uintptr_t)module1_stack_top - sizeof(struct trapframe);
    module1_fault = (uintptr_t)(id == 0 ? module1_fault0 : id == 1 ? module1_fault1 : module1_fault2);
    module1_epc = (uintptr_t)(id == 0 ? module1_after0 : id == 1 ? module1_after1 : module1_after2);
    ((uintptr_t *)module1_stack_bottom)[0] = guard;
    ((uintptr_t *)module1_stack_bottom)[1] = ~guard;
    ((uintptr_t *)module1_stack_top)[0] = guard;
    ((uintptr_t *)module1_stack_top)[1] = ~guard;
}

void module1_handler(struct trapframe *tf) {
    check("tf", (uintptr_t)tf, module1_frame);
    check("tf alignment", (uintptr_t)tf & 15, 0);
    check("event count before trap", module1_events, module1_case);
    check("cause", tf->cause, CAUSE_BREAKPOINT);
    check("entry epc", tf->epc, module1_fault);
    check("entry SPP", tf->status & SSTATUS_SPP, SSTATUS_SPP);
    check("entry SIE", tf->status & SSTATUS_SIE, 0);
    check("entry SPIE", tf->status & SSTATUS_SPIE, module1_case == 1 ? SSTATUS_SPIE : 0);
    for (int i = 0; i < 32; i++) {
        check("saved GPR", ((uintptr_t *)&tf->gpr)[i], module1_expected[i]);
    }
    tf->epc = module1_epc;
    if (module1_case == 2) {
        tf->gpr.sp = (uintptr_t)module1_alt_top;
        tf->gpr.a5 = 0xfedcba9876543210ULL;
        tf->status |= SSTATUS_SPIE;
    }
    for (int i = 0; i < 32; i++) module1_returned[i] = ((uintptr_t *)&tf->gpr)[i];
    module1_status = tf->status;
    module1_events++;
    /* Deliberately use every caller-saved argument/temporary register. */
    asm volatile(
        "li t0, 0x55\nli t1, 0x56\nli t2, 0x57\n"
        "li t3, 0x58\nli t4, 0x59\nli t5, 0x5a\nli t6, 0x5b\n"
        "li a0, 0x60\nli a1, 0x61\nli a2, 0x62\nli a3, 0x63\n"
        "li a4, 0x64\nli a5, 0x65\nli a6, 0x66\nli a7, 0x67\n"
        ::: "t0","t1","t2","t3","t4","t5","t6", "a0","a1","a2","a3","a4","a5","a6","a7", "memory");
}

void module1_post(void) {
    for (int i = 0; i < 32; i++) check("returned GPR", module1_observed[i], module1_returned[i]);
    uintptr_t mask = SSTATUS_SIE | SSTATUS_SPIE | SSTATUS_SPP;
    uintptr_t after = SSTATUS_SPIE | ((module1_status & SSTATUS_SPIE) ? SSTATUS_SIE : 0);
    check("returned status", module1_observed[32] & mask, after);
    check("returned sepc", module1_observed[33], module1_epc);
    /* The sampler borrows/clears sscratch; GDB checks the real return value. */
    check("snapshot scratch reset", module1_observed[34], 0);
    check("stack guard bottom", ((uintptr_t *)module1_stack_bottom)[0], guard);
    check("stack guard bottom+8", ((uintptr_t *)module1_stack_bottom)[1], ~guard);
    check("stack guard top", ((uintptr_t *)module1_stack_top)[0], guard);
    check("stack guard top+8", ((uintptr_t *)module1_stack_top)[1], ~guard);
    check("event count after trap", module1_events, module1_case + 1);
    module1_posts++;
    cprintf("MODULE1_CASE_PASS case=%lu gpr=32 sp=0x%016lx epc=0x%016lx status=0x%lx\n",
            module1_case, module1_observed[2], module1_observed[33], module1_observed[32] & mask);
}

void module1_finish(void) {
    check("total events", module1_events, 3);
    check("total posts", module1_posts, 3);
    check("failures", module1_failures, 0);
    cprintf("MODULE1_DYNAMIC_PASS cases=3 events=3 posts=3\n");
}
