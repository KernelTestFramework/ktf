#include <ktf.h>
#include <lib.h>
#include <list.h>
#include <page.h>
#include <setup.h>
#include <console.h>
#include <multiboot.h>

#include <mm/pmm.h>
#include <drivers/vga.h>

static list_head_t free_frames[MAX_PAGE_ORDER + 1];
static list_head_t busy_frames[MAX_PAGE_ORDER + 1];

static frame_t early_frames[2 * PAGE_SIZE];
static unsigned int free_frame_idx;

static size_t frames_count[MAX_PAGE_ORDER + 1];

#define _RANGE(_name, _base, _flags, _start, _end) {   \
    .name = _name, .base = (_base), .flags = (_flags), \
    .start = _ptr(_start ),                            \
    .end = _ptr(_end )                                 \
}

#define IDENT_RANGE(name, flags, start, end) \
    _RANGE(name, VIRT_IDENT_BASE, flags, start, end)

#define USER_RANGE(name, flags, start, end) \
    _RANGE(name, VIRT_USER_BASE, flags, start, end)

#define KERNEL_RANGE(name, flags, start, end) \
    _RANGE(name, VIRT_KERNEL_BASE, flags, start, end)

#define VIDEO_START (VIRT_KERNEL_BASE + VGA_START_ADDR)
#define VIDEO_END   (VIRT_KERNEL_BASE + VGA_END_ADDR)

addr_range_t addr_ranges[] = {
    IDENT_RANGE( "Low memory",  L1_PROT_RO,      0x0,               MB(1)           ),
    IDENT_RANGE( ".text.init",  L1_PROT_RO,      __start_text_init, __end_text_init ),
    IDENT_RANGE( ".data.init",  L1_PROT,         __start_data_init, __end_data_init ),
    IDENT_RANGE( ".bss.init",   L1_PROT,         __start_bss_init,  __end_bss_init  ),

    IDENT_RANGE( ".rmode",      L1_PROT,         __start_rmode,     __end_rmode     ),

    USER_RANGE( ".text.user",   L1_PROT_USER_RO, __start_text_user, __end_text_user ),
    USER_RANGE( ".data.user",   L1_PROT_USER,    __start_data_user, __end_data_user ),
    USER_RANGE( ".bss.user",    L1_PROT_USER,    __start_bss_user,  __end_bss_user  ),

    KERNEL_RANGE( "Low memory", L1_PROT,         0x0,               MB(1)           ),
    KERNEL_RANGE( ".text",      L1_PROT_RO,      __start_text,      __end_text      ),
    KERNEL_RANGE( ".data",      L1_PROT,         __start_data,      __end_data      ),
    KERNEL_RANGE( ".bss",       L1_PROT,         __start_bss,       __end_bss       ),
    KERNEL_RANGE( ".rodata",    L1_PROT_RO,      __start_rodata,    __end_rodata    ),

    { 0x0 } /* NULL array terminator */
};

void display_memory_map(void) {
    printk("Memory Map:\n");

    for_each_memory_range(r) {
        printk("%11s: VA: [0x%016lx - 0x%016lx] PA: [0x%08lx - 0x%08lx]\n",
               r->name, r->start, r->end, r->start - r->base, r->end - r->base);
    }
}

addr_range_t get_memory_range(paddr_t pa) {
    addr_range_t r;

    if (mbi_get_memory_range(pa, &r) < 0)
        /* FIXME: e820_lower_memory_bound() */
        panic("Unable to get memory range for: 0x%016lx\n", pa);

    return r;
}

paddr_t get_memory_range_start(paddr_t pa) {
    addr_range_t r = get_memory_range(pa);

    return _paddr(r.start);
}

paddr_t get_memory_range_end(paddr_t pa) {
    addr_range_t r = get_memory_range(pa);

    return _paddr(r.end);
}

static void display_frames_count(size_t size) {
    printk("Avail memory frames: (total size: %lu MB)\n", size / MB(1));

    for_each_order(order) {
        size_t count = frames_count[order];

        if (count)
            printk("  %lu KB: %lu\n",  (PAGE_SIZE << order) / KB(1), count);
    }
}

static inline void display_frame(const frame_t *frame) {
    printk("Frame: mfn: %lx, order: %u, refcnt: %u, uc: %u, free: %u\n",
           frame->mfn, frame->order, frame->refcount, frame->uncachable, frame->free);
}

static void add_frame(paddr_t *pa, unsigned int order) {
    frame_t *free_frame = &early_frames[free_frame_idx++];

    if (free_frame_idx > ARRAY_SIZE(early_frames))
        panic("Not enough initial frames for PMM allocation!\n");

    free_frame->order = order;
    free_frame->mfn = paddr_to_mfn(*pa);
    free_frame->free = true;

    *pa += (PAGE_SIZE << order);

    list_add(&free_frame->list, &free_frames[order]);
}

static size_t process_memory_range(unsigned index) {
    paddr_t start, end, cur;
    addr_range_t range;
    size_t size;

    if (mbi_get_avail_memory_range(index, &range) < 0)
        return 0;

    cur = start = _paddr(range.start);
    end = _paddr(range.end);
    size = end - start;

    while(cur % PAGE_SIZE_2M) {
        add_frame(&cur, PAGE_ORDER_4K);
        frames_count[PAGE_ORDER_4K]++;
    }

    while((cur % PAGE_SIZE_1G) && cur + (PAGE_SIZE_2M - 1) <= end) {
        add_frame(&cur, PAGE_ORDER_2M);
        frames_count[PAGE_ORDER_2M]++;
    }

    while(cur + (PAGE_SIZE_1G - 1) <= end) {
        add_frame(&cur, PAGE_ORDER_1G);
        frames_count[PAGE_ORDER_1G]++;
    }

    while(cur + (PAGE_SIZE_2M - 1) <= end) {
        add_frame(&cur, PAGE_ORDER_2M);
        frames_count[PAGE_ORDER_2M]++;
    }

    BUG_ON(cur > end);

    return size;
}

void init_pmm(void) {
    size_t total_size = 0;
    unsigned num;

    printk("Initialize Physical Memory Manager\n");

    BUG_ON(ARRAY_SIZE(free_frames) != ARRAY_SIZE(busy_frames));
    for_each_order(order) {
        list_init(&free_frames[order]);
        list_init(&busy_frames[order]);
    }

    num = mbi_get_avail_memory_ranges_num();

    /* Skip low memory range */
    for (int i = 1; i < num; i++)
        total_size += process_memory_range(i);

    display_frames_count(total_size);

    if (opt_debug)
    {
        frame_t *frame;

        printk("List of frames:\n");
        for_each_order(order) {
            if (list_is_empty(&free_frames[order]))
                continue;

            printk("Order: %u\n", order);
            list_for_each_entry(frame, &free_frames[order], list)
                display_frame(frame);
        }
    }
}

mfn_t get_free_frames(unsigned int order) {
    frame_t *frame;

    if (order > MAX_PAGE_ORDER)
        return MFN_INVALID;

    if (list_is_empty(&free_frames[order])) {
        /* FIXME: Add page split */
        return MFN_INVALID;
    }

    frame = list_first_entry(&free_frames[order], frame_t, list);
    BUG_ON(!frame->free);
    frame->free = false;

    BUG_ON(frame->refcount > 0);
    frame->refcount++;

    list_unlink(&frame->list);
    list_add(&frame->list, &busy_frames[order]);

    return frame->mfn;
}

void put_frame(mfn_t mfn, unsigned int order) {
    frame_t *frame;
    frame_t *found = NULL;

    if (mfn == MFN_INVALID)
        return;

    list_for_each_entry(frame, &busy_frames[order], list) {
        if (frame->mfn == mfn) {
            found = frame;
            break;
        }
    }

    BUG_ON(!found);

    BUG_ON(frame->refcount == 0);
    frame->refcount--;

    if (found->refcount > 0)
        return;

    BUG_ON(frame->free);
    frame->free = true;

    list_unlink(&frame->list);
    /* FIXME: Maintain order wrt mfn value */
    list_add(&frame->list, &free_frames[order]);
    /* FIXME: Add frame merge */
}

void map_used_memory(void) {
    frame_t *frame;

    for_each_order(order) {
        list_for_each_entry(frame, &busy_frames[order], list)
            kmap(frame->mfn, order, L1_PROT);
    }

}
