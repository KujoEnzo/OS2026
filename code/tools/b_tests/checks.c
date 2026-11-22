/* Appended only to a temporary proc.c, so static helpers can be tested. */
static bool b_fail_pcb;
static unsigned b_pcb_live;
static void *b_alloc(size_t size) {
    if (b_fail_pcb) return NULL;
    void *p = (kmalloc)(size);
    if (p != NULL) b_pcb_live++;
    return p;
}
static void b_free(void *p) { b_pcb_live--; (kfree)(p); }
static struct Page *b_no_pages(size_t n) { (void)n; return NULL; }
void b_checks(void) {
    assert(current == idleproc && nr_process == 2);
    assert(idleproc->pid == 0 && idleproc->tf == NULL && idleproc->runs == 0);
    assert(initproc->parent == idleproc && initproc->pid == 1);
    assert(initproc->pgdir == boot_pgdir_pa && initproc->mm == NULL);
    assert(initproc->context.ra == (uintptr_t)forkret);
    assert(initproc->context.sp == (uintptr_t)initproc->tf);
    assert(initproc->tf->epc == (uintptr_t)kernel_thread_entry);
    assert(initproc->tf->gpr.s0 == (uintptr_t)init_main);
    assert(initproc->tf->status & SSTATUS_SPP);
    assert(initproc->tf->status & SSTATUS_SPIE);
    assert(!(initproc->tf->status & SSTATUS_SIE));
    assert((initproc->context.sp & 15) == 0);
    assert(initproc->tf->gpr.sp == (uintptr_t)initproc->tf);
    struct proc_struct *p = alloc_proc();
    assert(p && p->pid == -1 && p->state == PROC_UNINIT);
    assert(p->tf == NULL && p->parent == NULL && p->runs == 0);
    assert(p->context.ra == 0 && p->context.s11 == 0 && list_empty(&p->list_link));
    b_free(p);
    unsigned live = b_pcb_live;
    size_t free_before = nr_free_pages();
    b_fail_pcb = 1;
    assert(kernel_thread(init_main, NULL, 0) == -E_NO_MEM);
    b_fail_pcb = 0;
    const struct pmm_manager *old = pmm_manager;
    struct pmm_manager limited = *old;
    limited.alloc_pages = b_no_pages;
    pmm_manager = &limited;
    assert(kernel_thread(init_main, NULL, 0) == -E_NO_MEM);
    pmm_manager = old;
    current->mm = (struct mm_struct *)1;
    assert(kernel_thread(init_main, NULL, 0) == -E_INVAL);
    current->mm = NULL;
    int count = nr_process;
    nr_process = MAX_PROCESS;
    assert(kernel_thread(init_main, NULL, 0) == -E_NO_FREE_PROC);
    nr_process = count;
    assert(kernel_thread(NULL, NULL, 0) == -E_INVAL);
    assert(do_fork(0, 0, NULL) == -E_INVAL);
    assert(nr_process == 2 && b_pcb_live == live && nr_free_pages() == free_before);
    struct trapframe tf = *initproc->tf;
    struct trapframe saved = tf;
    int pid = do_fork(0, 0x10000, &tf);
    assert(pid > 1);
    p = find_proc(pid);
    assert(p && p->state == PROC_RUNNABLE && p->parent == current);
    assert(p->tf->gpr.sp == 0x10000 && p->tf->gpr.a0 == 0);
    assert(memcmp(&tf, &saved, sizeof(tf)) == 0);
    list_del(&p->hash_link); list_del(&p->list_link); nr_process--;
    put_kstack(p); b_free(p);
    assert(nr_process == 2 && b_pcb_live == live && nr_free_pages() == free_before);
    cprintf("[B PCB/CONTEXT/FAILURE-CLEANUP] PASS\n");
}
