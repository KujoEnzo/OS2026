#include <buddy_pmm.h>
#include <list.h>
#include <stdio.h>
#include <string.h>

//lab2 buddy system

/* Index metadata by ppn - nbase, but align and XOR ABSOLUTE physical PPNs.
 * Static BSS is part of the kernel image, reserved by the unchanged page_init.
 * The current physical ceiling bounds this table to 32768 entries (256 KiB).
 * No allocator is needed to bootstrap the allocator itself. */
#define BUDDY_PPN_END (PHYSICAL_MEMORY_END / PGSIZE)
#define BUDDY_PAGE_CAPACITY ((PHYSICAL_MEMORY_END - DRAM_BASE) / PGSIZE)
#define BUDDY_MAX_ORDER (63U - __builtin_clzll(BUDDY_PAGE_CAPACITY))

enum buddy_page_state {
    BUDDY_UNMANAGED,
    BUDDY_INTERIOR, /* registered, but neither a free nor an allocated head */
    BUDDY_FREE_HEAD,
    BUDDY_ALLOC_HEAD,
};

struct buddy_page_meta {
    unsigned int requested; /* original request, at allocated heads only */
    unsigned char order;
    unsigned char state;
};

static struct buddy_page_meta buddy_meta[BUDDY_PAGE_CAPACITY];
static list_entry_t buddy_free_lists[BUDDY_MAX_ORDER + 1];
static size_t buddy_nr_free;

/* Validate an externally supplied descriptor before pointer subtraction. */
static bool
buddy_page_index(struct Page *page, size_t *index) {
    if (pages == NULL || nbase < DRAM_BASE / PGSIZE || npage < nbase ||
        npage > BUDDY_PPN_END || npage - nbase > BUDDY_PAGE_CAPACITY ||
        (uintptr_t)page < (uintptr_t)pages) {
        return 0;
    }
    uintptr_t delta = (uintptr_t)page - (uintptr_t)pages;
    if (delta % sizeof(struct Page) != 0) {
        return 0;
    }
    size_t i = delta / sizeof(struct Page);
    if (i >= npage - nbase || i >= BUDDY_PAGE_CAPACITY) {
        return 0;
    }
    *index = i;
    return 1;
}

/* All numerical bounds are checked BEFORE forming a Page pointer. Checking
 * every registered page also excludes holes and externally reserved pages. */
static bool
buddy_block_registered(size_t ppn, unsigned int order) {
    if (order > BUDDY_MAX_ORDER || ppn < nbase || ppn >= npage ||
        ppn >= BUDDY_PPN_END) {
        return 0;
    }
    size_t size = (size_t)1 << order;
    size_t index = ppn - nbase;
    if ((ppn & (size - 1)) != 0 || size > npage - ppn ||
        size > BUDDY_PPN_END - ppn || index >= BUDDY_PAGE_CAPACITY ||
        size > BUDDY_PAGE_CAPACITY - index) {
        return 0;
    }
    for (size_t i = index; i < index + size; i++) {
        if (buddy_meta[i].state == BUDDY_UNMANAGED ||
            PageReserved(&pages[i])) {
            return 0;
        }
    }
    return 1;
}

static void
buddy_link_free(size_t ppn, unsigned int order) {
    assert(buddy_block_registered(ppn, order));
    size_t index = ppn - nbase;
    struct Page *page = &pages[index];
    assert(buddy_meta[index].state == BUDDY_INTERIOR);
    buddy_meta[index].state = BUDDY_FREE_HEAD;
    buddy_meta[index].order = order;
    buddy_meta[index].requested = 0;
    page->property = (size_t)1 << order;
    SetPageProperty(page);
    list_add(&buddy_free_lists[order], &page->page_link);
}

static void
buddy_unlink_free(struct Page *page) {
    size_t index = 0;
    assert(buddy_page_index(page, &index));
    struct buddy_page_meta *meta = &buddy_meta[index];
    assert(meta->state == BUDDY_FREE_HEAD);
    assert(buddy_block_registered(nbase + index, meta->order));
    assert(PageProperty(page) && page->property == ((size_t)1 << meta->order));
    list_del_init(&page->page_link);
    ClearPageProperty(page);
    page->property = 0;
    meta->state = BUDDY_INTERIOR;
    meta->order = 0;
    meta->requested = 0;
}

static struct Page *
buddy_free_partner(size_t ppn, unsigned int order) {
    assert(order <= BUDDY_MAX_ORDER);
    size_t partner_ppn = ppn ^ ((size_t)1 << order);
    if (!buddy_block_registered(partner_ppn, order)) {
        return NULL;
    }
    size_t index = partner_ppn - nbase;
    struct buddy_page_meta *meta = &buddy_meta[index];
    /* Only now is conversion of partner_ppn to a descriptor allowed. */
    struct Page *partner = &pages[index];
    if (meta->state != BUDDY_FREE_HEAD || meta->order != order ||
        !PageProperty(partner) || partner->property != ((size_t)1 << order)) {
        return NULL;
    }
    return partner;
}

/* The incoming block is not linked yet. Coalesce only true XOR partners. */
static void
buddy_release_block(size_t ppn, unsigned int order) {
    while (order < BUDDY_MAX_ORDER) {
        struct Page *partner = buddy_free_partner(ppn, order);
        if (partner == NULL) {
            break;
        }
        size_t partner_ppn = page2ppn(partner);
        buddy_unlink_free(partner);
        if (partner_ppn < ppn) {
            ppn = partner_ppn;
        }
        order++;
    }
    buddy_link_free(ppn, order);
}

static void
buddy_init(void) {
    static_assert(BUDDY_PAGE_CAPACITY > 0);
    static_assert(BUDDY_PAGE_CAPACITY <= (unsigned int)-1);
    static_assert(BUDDY_MAX_ORDER < sizeof(size_t) * 8);
    static_assert(BUDDY_MAX_ORDER <= (unsigned char)-1);
    memset(buddy_meta, 0, sizeof(buddy_meta));
    for (unsigned int order = 0; order <= BUDDY_MAX_ORDER; order++) {
        list_init(&buddy_free_lists[order]);
    }
    buddy_nr_free = 0;
}

static void
buddy_init_memmap(struct Page *base, size_t n) {
    assert(n > 0);
    size_t index = 0;
    assert(buddy_page_index(base, &index));
    assert(n <= npage - nbase - index);
    assert(n <= BUDDY_PAGE_CAPACITY - index);
    /* Preflight the complete interval; repeated/overlapping registration is
     * invalid even if an allocated page happens to look non-free. */
    for (size_t i = index; i < index + n; i++) {
        assert(buddy_meta[i].state == BUDDY_UNMANAGED);
        assert(PageReserved(&pages[i]));
    }
    for (size_t i = index; i < index + n; i++) {
        buddy_meta[i].state = BUDDY_INTERIOR;
        pages[i].flags = 0;
        pages[i].property = 0;
        set_page_ref(&pages[i], 0);
        list_init(&pages[i].page_link);
    }
    size_t ppn = nbase + index, remaining = n;
    while (remaining > 0) {
        unsigned int order = 0;
        while (order < BUDDY_MAX_ORDER) {
            size_t next_size = (size_t)1 << (order + 1);
            if (next_size > remaining || (ppn & (next_size - 1)) != 0) {
                break;
            }
            order++;
        }
        size_t size = (size_t)1 << order;
        buddy_release_block(ppn, order);
        ppn += size;
        remaining -= size;
    }
    buddy_nr_free += n;
}

static struct Page *
buddy_alloc_pages(size_t n) {
    assert(n > 0);
    /* Bound n before rounding: no n + size - 1 or unbounded left shifts. */
    if (n > BUDDY_PAGE_CAPACITY) {
        return NULL;
    }
    unsigned int wanted = 0;
    size_t size = 1;
    while (size < n) {
        if (wanted == BUDDY_MAX_ORDER) {
            return NULL;
        }
        wanted++;
        size <<= 1;
    }
    if (size > buddy_nr_free) {
        return NULL;
    }
    unsigned int order = wanted;
    while (order <= BUDDY_MAX_ORDER && list_empty(&buddy_free_lists[order])) {
        order++;
    }
    if (order > BUDDY_MAX_ORDER) {
        return NULL;
    }
    struct Page *base = le2page(list_next(&buddy_free_lists[order]), page_link);
    size_t ppn = page2ppn(base), index = ppn - nbase;
    buddy_unlink_free(base);
    while (order > wanted) {
        order--;
        buddy_link_free(ppn + ((size_t)1 << order), order);
    }
    for (size_t i = index; i < index + size; i++) {
        assert(buddy_meta[i].state == BUDDY_INTERIOR);
        ClearPageProperty(&pages[i]);
        pages[i].property = 0;
        set_page_ref(&pages[i], 0);
        list_init(&pages[i].page_link);
    }
    buddy_meta[index].state = BUDDY_ALLOC_HEAD;
    buddy_meta[index].order = wanted;
    buddy_meta[index].requested = n;
    buddy_nr_free -= size;
    return base;
}

static void
buddy_free_pages(struct Page *base, size_t n) {
    assert(n > 0);
    size_t index = 0;
    assert(buddy_page_index(base, &index));
    struct buddy_page_meta *meta = &buddy_meta[index];
    assert(meta->state == BUDDY_ALLOC_HEAD && meta->requested == n);
    unsigned int order = meta->order;
    size_t ppn = nbase + index;
    assert(buddy_block_registered(ppn, order));
    size_t size = (size_t)1 << order;
    for (size_t i = index; i < index + size; i++) {
        assert(!PageProperty(&pages[i]));
        assert(i == index || buddy_meta[i].state == BUDDY_INTERIOR);
    }
    for (size_t i = index; i < index + size; i++) {
        buddy_meta[i].state = BUDDY_INTERIOR;
        buddy_meta[i].order = 0;
        buddy_meta[i].requested = 0;
        pages[i].flags = 0;
        pages[i].property = 0;
        set_page_ref(&pages[i], 0);
        list_init(&pages[i].page_link);
    }
    buddy_release_block(ppn, order);
    buddy_nr_free += size;
}

static size_t
buddy_nr_free_pages(void) {
    return buddy_nr_free;
}

static void
buddy_validate(void) {
    size_t head_counts[BUDDY_MAX_ORDER + 1] = {0};
    size_t total = 0;
    assert(npage >= nbase && npage <= BUDDY_PPN_END);
    assert(npage - nbase <= BUDDY_PAGE_CAPACITY);
    for (size_t i = 0; i < npage - nbase;) {
        struct buddy_page_meta *meta = &buddy_meta[i];
        if (meta->state == BUDDY_UNMANAGED) {
            i++;
            continue;
        }
        assert(meta->state == BUDDY_FREE_HEAD || meta->state == BUDDY_ALLOC_HEAD);
        assert(buddy_block_registered(nbase + i, meta->order));
        size_t size = (size_t)1 << meta->order;
        for (size_t j = i + 1; j < i + size; j++) {
            assert(buddy_meta[j].state == BUDDY_INTERIOR);
            assert(buddy_meta[j].requested == 0 && buddy_meta[j].order == 0);
            assert(!PageProperty(&pages[j]) && pages[j].property == 0);
        }
        if (meta->state == BUDDY_FREE_HEAD) {
            assert(PageProperty(&pages[i]) && pages[i].property == size);
            assert(meta->requested == 0);
            if (meta->order < BUDDY_MAX_ORDER) {
                assert(buddy_free_partner(nbase + i, meta->order) == NULL);
            }
            head_counts[meta->order]++;
            total += size;
        } else {
            assert(!PageProperty(&pages[i]) && pages[i].property == 0);
            assert(meta->requested > 0 && meta->requested <= size);
            assert(meta->order == 0 || meta->requested > size / 2);
        }
        i += size;
    }
    assert(total == buddy_nr_free);
    for (unsigned int order = 0; order <= BUDDY_MAX_ORDER; order++) {
        list_entry_t *head = &buddy_free_lists[order];
        list_entry_t *le = head;
        size_t count = 0;
        while ((le = list_next(le)) != head) {
            assert(++count <= BUDDY_PAGE_CAPACITY);
            assert(list_prev(list_next(le)) == le && list_next(list_prev(le)) == le);
            size_t index = 0;
            assert(buddy_page_index(le2page(le, page_link), &index));
            assert(buddy_meta[index].state == BUDDY_FREE_HEAD);
            assert(buddy_meta[index].order == order);
        }
        assert(list_prev(list_next(head)) == head && list_next(list_prev(head)) == head);
        assert(count == head_counts[order]);
    }
}

#include "buddy_pmm_check.h"

static void
buddy_check(void) {
    buddy_validate();
    buddy_run_isolated_tests();
    cprintf("buddy_check() succeeded!\n");
}

const struct pmm_manager buddy_pmm_manager = {
    .name = "buddy_pmm_manager",
    .init = buddy_init,
    .init_memmap = buddy_init_memmap,
    .alloc_pages = buddy_alloc_pages,
    .free_pages = buddy_free_pages,
    .nr_free_pages = buddy_nr_free_pages,
    .check = buddy_check,
};
