/* SPDX-License-Identifier: MIT
 *
 * Stage 2 case: every allocated page reads as zero, even a recycled one.
 *
 * The case first reads a fresh page. It then dirties the page the allocator
 * will hand out next, asks for that page, and reads it back. It dirties every
 * page a three-page block will occupy, asks for the block, and reads all three
 * pages back. A correct allocator zeroes each returned page before it returns,
 * so the dirty bytes are gone. An allocator that skips the zeroing, or zeroes
 * only part of a multi-page block, returns the dirt and fails here.
 *
 * The reference OS (instructor only) runs this case. The starter has no
 * allocator, so the build fails on the starter. That failure is the expected
 * start state.
 */
#include "assert.h"
#include "kernel.h"

/* The kernel source defines these. The book declares no header for them. */
paddr_t alloc_pages(uint32_t n);

static bool page_is_zero(const uint8_t *p) {
    for (uint32_t i = 0; i < PAGE_SIZE; i++) {
        if (p[i] != 0)
            return false;
    }
    return true;
}

void ktest_main(void) {
    ktest_begin("stage2_uninit");

    paddr_t p = alloc_pages(1);
    KTEST_ASSERT(is_aligned(p, PAGE_SIZE));

    /* The fresh page reads as zero. */
    KTEST_ASSERT(page_is_zero((const uint8_t *) p));

    /* Dirty the next page before the allocator reaches it, then ask for it.
     * The allocator advances one page at a time, so the next request returns
     * exactly this page. */
    uint8_t *dirty = (uint8_t *) (p + PAGE_SIZE);
    for (uint32_t i = 0; i < PAGE_SIZE; i++)
        dirty[i] = 0xA5;

    paddr_t q = alloc_pages(1);
    KTEST_ASSERT(q == p + PAGE_SIZE);

    /* The allocator returns the recycled page with every byte set to zero. */
    KTEST_ASSERT(page_is_zero((const uint8_t *) q));

    /* Dirty every page the three-page block will occupy, so a zero fill that
     * stops after the first page leaves the later pages dirty. */
    for (uint32_t i = 0; i < PAGE_SIZE; i++) {
        ((uint8_t *) (p + 2 * PAGE_SIZE))[i] = 0x5A;
        ((uint8_t *) (p + 3 * PAGE_SIZE))[i] = 0x5A;
        ((uint8_t *) (p + 4 * PAGE_SIZE))[i] = 0x5A;
    }

    /* A multi-page block is zero across all of its pages. */
    paddr_t r = alloc_pages(3);
    KTEST_ASSERT(r == p + 2 * PAGE_SIZE);
    KTEST_ASSERT(page_is_zero((const uint8_t *) r));
    KTEST_ASSERT(page_is_zero((const uint8_t *) (r + PAGE_SIZE)));
    KTEST_ASSERT(page_is_zero((const uint8_t *) (r + 2 * PAGE_SIZE)));

    ktest_end("stage2_uninit");
}
