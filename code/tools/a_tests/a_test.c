#include <assert.h>
#include <error.h>
#include <intr.h>
#include <pmm.h>
#include <proc.h>
#include <riscv.h>
#include <sched.h>
#include <sbi.h>
#include <stdio.h>
#include <string.h>

void test_lab4_pmm_basic(void);

/* The official PMM test leaves empty tables; this fixture owns root[0]. */
static void reclaim_low_tables(void) {
    if (boot_pgdir_va[0] & PTE_V) {
        pde_t *middle = KADDR(PDE_ADDR(boot_pgdir_va[0]));
        for (int i = 0; i < NPDEENTRY; i++) {
            if (middle[i] & PTE_V) free_page(pde2page(middle[i]));
        }
        free_page(pde2page(boot_pgdir_va[0]));
        boot_pgdir_va[0] = 0;
        flush_tlb();
    }
}

static const struct pmm_manager *real_manager;
static int allocation_budget;
static struct Page *limited_alloc(size_t count) {
    if (allocation_budget-- <= 0) return NULL;
    return real_manager->alloc_pages(count);
}

static void test_mapping(void) {
    size_t before = nr_free_pages();
    test_lab4_pmm_basic();
    reclaim_low_tables();
    assert(nr_free_pages() == before);

    struct Page *first = alloc_page(), *second = alloc_page();
    assert(first != NULL && second != NULL);
    *(volatile int *)page2kva(first) = 17;
    *(volatile int *)page2kva(second) = 29;
    assert(page_insert(boot_pgdir_va, first, 0x4000, PTE_R | PTE_W) == 0);
    assert(*(volatile int *)0x4000 == 17);
    size_t allocated = nr_free_pages();
    assert(page_insert(boot_pgdir_va, first, 0x4000, PTE_R) == 0);
    assert(page_ref(first) == 1 && nr_free_pages() == allocated);
    pte_t *entry = get_pte(boot_pgdir_va, 0x4000, 0);
    assert((*entry & (PTE_R | PTE_W | PTE_U)) == PTE_R);
    assert(page_insert(boot_pgdir_va, second, 0x4000, PTE_R | PTE_W) == 0);
    assert(page_ref(first) == 0 && page_ref(second) == 1);
    assert(*(volatile int *)0x4000 == 29); /* Stale TLB would still read 17. */
    *(volatile int *)0x4000 = 31;
    assert(*(volatile int *)page2kva(second) == 31);
    page_remove(boot_pgdir_va, 0x4000);
    assert(*entry == 0 && page_ref(second) == 0);
    size_t removed = nr_free_pages();
    page_remove(boot_pgdir_va, 0x4000);
    page_remove(boot_pgdir_va, 1UL << 30);
    assert(nr_free_pages() == removed);
    reclaim_low_tables();
    assert(nr_free_pages() == before);
    cprintf("[A PMM] mapping, permissions, references, TLB, idempotence PASS\n");

    struct Page *data = alloc_page();
    real_manager = pmm_manager;
    struct pmm_manager limited = *real_manager;
    limited.alloc_pages = limited_alloc;
    for (int budget = 0; budget <= 1; budget++) {
        allocation_budget = budget;
        pmm_manager = &limited;
        assert(page_insert(boot_pgdir_va, data, 0x4000, PTE_R) == -E_NO_MEM);
        pmm_manager = real_manager;
        assert(page_ref(data) == 0 && get_page(boot_pgdir_va, 0x4000, NULL) == NULL);
        reclaim_low_tables();
        assert(nr_free_pages() == before - 1);
    }
    free_page(data);
    assert(nr_free_pages() == before);
    cprintf("[A PMM] first/second page-table allocation failure PASS\n");
}

#ifdef A_TEST_SCHEDULE
static struct proc_struct idle, worker[3];
static unsigned char stacks[3][KSTACKSIZE] __attribute__((aligned(16)));
static int events[8], event_count;
static uintptr_t alternate_pgdir;
int a_test_registers(int thread_id);

static void worker_entry(void) {
    int id = current->pid;
    assert((read_csr(sstatus) & SSTATUS_SIE) == 0);
    assert(read_csr(satp) == ((8UL << 60) | (current->pgdir >> 12)));
    if (id == 1) assert(current->pgdir == alternate_pgdir);
    for (int i = 0; i < 3; i++) {
        events[event_count++] = id;
        current->need_resched = 1;
        assert(a_test_registers(id) == 0);
        assert(current->pid == id && !current->need_resched);
    }
    current->state = PROC_SLEEPING;
    schedule();
    panic("sleeping worker resumed");
}

static void return_once(void) {
    assert(current == &worker[0]);
    assert((read_csr(sstatus) & SSTATUS_SIE) == 0);
    current->state = PROC_SLEEPING;
    schedule();
    panic("return-once worker resumed");
}

static void finish_from_idle(void) {
    assert(current == &worker[0] && idle.need_resched == 0);
    cprintf("[A SCHEDULE] cpu_idle dispatch PASS\n");
    cprintf("[A SCHEDULE] ALL PASS\n");
    sbi_shutdown();
    panic("shutdown returned");
}

static void set_entry(struct proc_struct *proc, void (*entry)(void)) {
    memset(&proc->context, 0, sizeof(proc->context));
    proc->context.ra = (uintptr_t)entry;
    proc->context.sp = (uintptr_t)&stacks[proc->pid - 1][KSTACKSIZE];
    proc->state = PROC_RUNNABLE;
}

static void test_schedule(void) {
    intr_disable();
    memset(&idle, 0, sizeof(idle));
    idle.state = PROC_RUNNABLE;
    idle.pgdir = boot_pgdir_pa;
    idle.kstack = (uintptr_t)bootstack;
    idleproc = current = &idle;
    list_init(&proc_list);
    idle.need_resched = 1;
    proc_run(current);
    assert(idle.need_resched == 1);
    schedule();
    assert(current == &idle && idle.runs == 1 && !idle.need_resched);
    assert(!(read_csr(sstatus) & SSTATUS_SIE));
    for (int i = 0; i < 3; i++) {
        worker[i].pid = i + 1;
        worker[i].state = PROC_SLEEPING;
        worker[i].pgdir = boot_pgdir_pa;
        list_add_before(&proc_list, &worker[i].list_link);
    }
    schedule();
    assert(current == &idle && idle.runs == 2);
    struct Page *root = alloc_page();
    assert(root != NULL);
    memcpy(page2kva(root), boot_pgdir_va, PGSIZE);
    alternate_pgdir = page2pa(root);
    worker[0].pgdir = alternate_pgdir;
    set_entry(&worker[0], worker_entry);
    set_entry(&worker[1], worker_entry);
    schedule();
    assert(current == &idle && event_count == 6);
    for (int i = 0; i < 6; i++) assert(events[i] == 1 + i % 2);
    assert(worker[2].runs == 0);
    assert(read_csr(satp) == ((8UL << 60) | (boot_pgdir_pa >> 12)));
    assert(!(read_csr(sstatus) & SSTATUS_SIE));
    cprintf("[A SCHEDULE] rotation, wrap, sleeping, fallback, ra/sp/s0-s11, Sv39 PASS\n");
    /* Only current is runnable: the scan must include it after wrapping. */
    current = &worker[2];
    worker[2].state = PROC_RUNNABLE;
    int old_runs = worker[2].runs;
    intr_enable();
    schedule();
    assert(current == &worker[2] && worker[2].runs == old_runs + 1);
    assert(read_csr(sstatus) & SSTATUS_SIE);
    worker[2].state = PROC_SLEEPING;
    current = &idle;
    set_entry(&worker[0], return_once);
    proc_run(&worker[0]);
    assert(current == &idle && (read_csr(sstatus) & SSTATUS_SIE));
    intr_disable();
    cprintf("[A SCHEDULE] interrupt off/on restoration and self selection PASS\n");
    worker[0].pgdir = boot_pgdir_pa;
    free_page(root);
    set_entry(&worker[0], finish_from_idle);
    idle.need_resched = 1;
    cpu_idle();
}
#endif

void a_test_main(void) {
    test_mapping();
#ifdef A_TEST_SCHEDULE
    test_schedule();
#else
    cprintf("[A PMM] ALL PASS\n");
    sbi_shutdown();
#endif
    panic("test returned");
}
