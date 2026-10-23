#ifndef __KERN_MM_BUDDY_PMM_CHECK_H__
#define __KERN_MM_BUDDY_PMM_CHECK_H__

/* Private to buddy_pmm.c. Tests use descriptors only, never physical payloads.
 * This BSS workspace is reserved with the kernel; no dynamic allocation and
 * no large snapshots on the two-page kernel stack. */
#define BUDDY_CHECK_PAGES 64
#define BUDDY_CHECK_LIVE 32

struct buddy_check_allocation {
    struct Page *base;
    size_t requested, actual;
    bool live;
};

static struct {
    struct Page controlled[BUDDY_CHECK_PAGES];
    struct buddy_page_meta original_meta[BUDDY_CHECK_PAGES];
    list_entry_t original_lists[BUDDY_MAX_ORDER + 1];
    struct Page failure_pages[BUDDY_CHECK_PAGES];
    struct buddy_page_meta failure_meta[BUDDY_CHECK_PAGES];
    list_entry_t failure_lists[BUDDY_MAX_ORDER + 1];
    struct buddy_check_allocation allocations[BUDDY_CHECK_LIVE];
    bool managed[BUDDY_CHECK_PAGES];
} buddy_test;

/* Independent conservation and interval oracle, in addition to the
 * allocator's structural validator. Expectations use the requested fixture
 * sizes, never the allocator's order metadata. */
static void
buddy_test_state(void) {
    size_t managed = 0, used = 0;
    for (size_t i = 0; i < BUDDY_CHECK_PAGES; i++) {
        managed += buddy_test.managed[i] != 0;
        if (!buddy_test.managed[i]) {
            assert(PageReserved(&pages[i]));
        }
    }
    for (size_t i = 0; i < BUDDY_CHECK_LIVE; i++) {
        struct buddy_check_allocation *a = &buddy_test.allocations[i];
        if (!a->live) {
            continue;
        }
        size_t index = 0;
        assert(buddy_page_index(a->base, &index));
        assert(a->actual <= BUDDY_CHECK_PAGES - index);
        assert((page2ppn(a->base) & (a->actual - 1)) == 0);
        assert(buddy_meta[index].state == BUDDY_ALLOC_HEAD);
        assert(buddy_meta[index].requested == a->requested);
        assert(((size_t)1 << buddy_meta[index].order) == a->actual);
        for (size_t j = index; j < index + a->actual; j++) {
            assert(buddy_test.managed[j]);
            assert(!PageReserved(&pages[j]) && !PageProperty(&pages[j]));
            assert(pages[j].property == 0 && page_ref(&pages[j]) == 0);
        }
        for (size_t j = i + 1; j < BUDDY_CHECK_LIVE; j++) {
            struct buddy_check_allocation *b = &buddy_test.allocations[j];
            if (b->live) {
                size_t a_ppn = page2ppn(a->base), b_ppn = page2ppn(b->base);
                assert(a_ppn + a->actual <= b_ppn || b_ppn + b->actual <= a_ppn);
            }
        }
        used += a->actual;
    }
    assert(buddy_pmm_manager.nr_free_pages() + used == managed);
    buddy_validate();
}

static void
buddy_test_reset(void) {
    for (size_t i = 0; i < BUDDY_CHECK_LIVE; i++) {
        assert(!buddy_test.allocations[i].live);
    }
    /* Do NOT call buddy_init: metadata outside this prefix belongs to the
     * real allocator and must remain untouched. npage bounds all accesses. */
    memset(buddy_meta, 0, sizeof(buddy_test.original_meta));
    memset(buddy_test.controlled, 0, sizeof(buddy_test.controlled));
    memset(buddy_test.managed, 0, sizeof(buddy_test.managed));
    for (size_t i = 0; i < BUDDY_CHECK_PAGES; i++) {
        SetPageReserved(&pages[i]);
        list_init(&pages[i].page_link);
    }
    for (unsigned int order = 0; order <= BUDDY_MAX_ORDER; order++) {
        list_init(&buddy_free_lists[order]);
    }
    buddy_nr_free = 0;
    buddy_test_state();
}

static void
buddy_test_register(size_t index, size_t n) {
    assert(index < BUDDY_CHECK_PAGES && n <= BUDDY_CHECK_PAGES - index);
    buddy_pmm_manager.init_memmap(&pages[index], n);
    for (size_t i = index; i < index + n; i++) {
        assert(!buddy_test.managed[i]);
        buddy_test.managed[i] = 1;
    }
    buddy_test_state();
}

static size_t
buddy_test_take(size_t requested, size_t actual) {
    size_t slot = 0, before = buddy_pmm_manager.nr_free_pages();
    while (slot < BUDDY_CHECK_LIVE && buddy_test.allocations[slot].live) {
        slot++;
    }
    assert(slot < BUDDY_CHECK_LIVE && before >= actual);
    struct Page *base = buddy_pmm_manager.alloc_pages(requested);
    assert(base != NULL);
    assert(buddy_pmm_manager.nr_free_pages() == before - actual);
    buddy_test.allocations[slot].base = base;
    buddy_test.allocations[slot].requested = requested;
    buddy_test.allocations[slot].actual = actual;
    buddy_test.allocations[slot].live = 1;
    buddy_test_state();
    return slot;
}

static void
buddy_test_give(size_t slot) {
    assert(slot < BUDDY_CHECK_LIVE && buddy_test.allocations[slot].live);
    struct buddy_check_allocation *a = &buddy_test.allocations[slot];
    size_t before = buddy_pmm_manager.nr_free_pages();
    buddy_pmm_manager.free_pages(a->base, a->requested);
    assert(buddy_pmm_manager.nr_free_pages() == before + a->actual);
    a->live = 0;
    buddy_test_state();
}

static void
buddy_test_failure(size_t request) {
    size_t before = buddy_pmm_manager.nr_free_pages();
    memcpy(buddy_test.failure_pages, pages, sizeof(buddy_test.failure_pages));
    memcpy(buddy_test.failure_meta, buddy_meta, sizeof(buddy_test.failure_meta));
    memcpy(buddy_test.failure_lists, buddy_free_lists, sizeof(buddy_free_lists));
    assert(buddy_pmm_manager.alloc_pages(request) == NULL);
    assert(buddy_pmm_manager.nr_free_pages() == before);
    assert(memcmp(pages, buddy_test.failure_pages, sizeof(buddy_test.failure_pages)) == 0);
    assert(memcmp(buddy_meta, buddy_test.failure_meta, sizeof(buddy_test.failure_meta)) == 0);
    assert(memcmp(buddy_free_lists, buddy_test.failure_lists, sizeof(buddy_free_lists)) == 0);
    buddy_test_state();
}

static void
buddy_test_mixed(size_t root) {
    const size_t requested[] = {1, 2, 3, 5, 8};
    const size_t occupied[] = {1, 2, 4, 8, 8};
    size_t slots[5];
    for (unsigned int reverse = 0; reverse < 2; reverse++) {
        buddy_test_reset();
        buddy_test_register(root, 32);
        for (size_t i = 0; i < 5; i++) {
            slots[i] = buddy_test_take(requested[i], occupied[i]);
        }
        assert(buddy_pmm_manager.nr_free_pages() == 9);
        for (size_t i = 0; i < 5; i++) {
            buddy_test_give(slots[reverse ? 4 - i : i]);
        }
        assert(buddy_pmm_manager.nr_free_pages() == 32);
        size_t whole = buddy_test_take(32, 32);
        buddy_test_give(whole);
    }
    cprintf("buddy test: mixed orders/rounding/disjoint ranges, forward/reverse recovery PASS\n");
}

static void
buddy_test_split_and_fragment(size_t root) {
    size_t slots[BUDDY_CHECK_LIVE];
    buddy_test_reset();
    buddy_test_register(root, 32);
    for (size_t i = 0; i < 32; i++) {
        slots[i] = buddy_test_take(1, 1);
    }
    assert(buddy_pmm_manager.nr_free_pages() == 0);
    buddy_test_failure(1);
    /* Select by actual PPN, never by incidental return-address ordering. */
    for (size_t i = 0; i < 32; i++) {
        if ((page2ppn(buddy_test.allocations[slots[i]].base) & 1) == 0) {
            buddy_test_give(slots[i]);
        }
    }
    assert(buddy_pmm_manager.nr_free_pages() == 16);
    buddy_test_failure(2); /* enough pages, but every buddy is still allocated */
    for (size_t i = 32; i > 0; i--) {
        if (buddy_test.allocations[slots[i - 1]].live) {
            buddy_test_give(slots[i - 1]);
        }
    }
    assert(buddy_pmm_manager.nr_free_pages() == 32);
    size_t whole = buddy_test_take(32, 32);
    buddy_test_give(whole);
    cprintf("buddy test: repeated splits/exhaustion, fragmented failure, recursive merge PASS\n");
}

static void
buddy_test_nonpartners(size_t root) {
    buddy_test_reset();
    /* For a 32-aligned root, root+1 and root+2 are adjacent but not XOR buddies. */
    buddy_test_register(root + 1, 1);
    buddy_test_register(root + 2, 1);
    assert(buddy_pmm_manager.nr_free_pages() == 2);
    buddy_test_failure(2);
    size_t a = buddy_test_take(1, 1), b = buddy_test_take(1, 1);
    buddy_test_give(a);
    buddy_test_give(b);
    buddy_test_failure(2);
    buddy_test_register(root, 1);
    buddy_test_register(root + 3, 1); /* now all four form real recursive buddies */
    size_t whole = buddy_test_take(4, 4);
    buddy_test_give(whole);
    cprintf("buddy test: adjacent nonpartners rejected, multi-registration true buddies PASS\n");
}

static void
buddy_test_initial_regions(size_t root) {
    size_t slots[26];
    buddy_test_reset();
    buddy_test_failure(1);
    buddy_test_failure((size_t)-1);
    buddy_test_register(root + 1, 23);
    buddy_test_register(root + 28, 3);
    assert(buddy_pmm_manager.nr_free_pages() == 26);
    for (size_t i = 0; i < 26; i++) {
        slots[i] = buddy_test_take(1, 1);
    }
    assert(buddy_pmm_manager.nr_free_pages() == 0);
    buddy_test_failure(1);
    for (size_t i = 26; i > 0; i--) {
        buddy_test_give(slots[i - 1]);
    }
    assert(buddy_pmm_manager.nr_free_pages() == 26);
    size_t large = buddy_test_take(8, 8);
    buddy_test_give(large);
    buddy_test_failure(32);
    buddy_test_failure(BUDDY_PAGE_CAPACITY + 1);
    cprintf("buddy test: unaligned/nonpower regions exact coverage, failure snapshots PASS\n");
}

static void
buddy_run_isolated_tests(void) {
    struct Page *original_pages = pages;
    size_t original_npage = npage, original_free = buddy_nr_free;
    assert(nbase <= BUDDY_PPN_END - BUDDY_CHECK_PAGES);
    memcpy(buddy_test.original_meta, buddy_meta, sizeof(buddy_test.original_meta));
    memcpy(buddy_test.original_lists, buddy_free_lists, sizeof(buddy_free_lists));

    /* Boot-time only: no concurrent allocator users. All helper bounds now
     * exclude the real descriptors and metadata outside this saved prefix. */
    pages = buddy_test.controlled;
    npage = nbase + BUDDY_CHECK_PAGES;
    size_t root = ROUNDUP(nbase, 32) - nbase;
    assert(root + 32 <= BUDDY_CHECK_PAGES);
    buddy_test_mixed(root);
    buddy_test_split_and_fragment(root);
    buddy_test_nonpartners(root);
    buddy_test_initial_regions(root);
    for (size_t i = 0; i < BUDDY_CHECK_LIVE; i++) {
        assert(!buddy_test.allocations[i].live);
    }

    memcpy(buddy_meta, buddy_test.original_meta, sizeof(buddy_test.original_meta));
    memcpy(buddy_free_lists, buddy_test.original_lists, sizeof(buddy_free_lists));
    buddy_nr_free = original_free;
    pages = original_pages;
    npage = original_npage;
    buddy_validate();
    assert(buddy_pmm_manager.nr_free_pages() == original_free);
    cprintf("buddy test: original allocator state restored PASS\n");
}

#endif /* !__KERN_MM_BUDDY_PMM_CHECK_H__ */
