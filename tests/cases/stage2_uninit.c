/* SPDX-License-Identifier: MIT
 *
 * Stage 2 case: every allocated page reads as zero, even a recycled one.
 *
 * The case first reads a fresh page. It then dirties the page the allocator
 * will hand out next, asks for that page, and reads it back. A correct
 * allocator zeroes the page before it returns, so the dirty bytes are gone. An
 * allocator that skips the zeroing returns the dirty bytes and fails here.
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

    /* The recycled page is zero, not the dirt left on it. */
    KTEST_ASSERT(page_is_zero((const uint8_t *) q));

    /* A multi-page block is zero across all of its pages. */
    paddr_t r = alloc_pages(3);
    KTEST_ASSERT(page_is_zero((const uint8_t *) r));
    KTEST_ASSERT(page_is_zero((const uint8_t *) (r + PAGE_SIZE)));
    KTEST_ASSERT(page_is_zero((const uint8_t *) (r + 2 * PAGE_SIZE)));

    ktest_end("stage2_uninit");
}
