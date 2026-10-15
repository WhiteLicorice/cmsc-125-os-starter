/* SPDX-License-Identifier: MIT
 *
 * Stage 2 case: the allocator returns page-aligned, non-overlapping blocks.
 *
 * The case asks for single pages and one multi-page block, then checks that
 * every returned address lies on a page boundary and that each block begins at
 * or after the end of the previous one. A block that is misaligned, or one that
 * overlaps its neighbor, fails an assertion here.
 *
 * The starter has no allocator, so the build fails on the starter. That
 * failure is the expected start state.
 */
#include "assert.h"
#include "kernel.h"

/* The kernel source defines these. The book declares no header for them. */
paddr_t alloc_pages(uint32_t n);

extern char __free_ram_end[];

void ktest_main(void) {
    ktest_begin("stage2_align");

    paddr_t a = alloc_pages(1);
    paddr_t b = alloc_pages(1);
    paddr_t c = alloc_pages(1);

    KTEST_ASSERT(is_aligned(a, PAGE_SIZE));
    KTEST_ASSERT(is_aligned(b, PAGE_SIZE));
    KTEST_ASSERT(is_aligned(c, PAGE_SIZE));

    /* Single-page blocks advance by a whole page and never overlap. */
    KTEST_ASSERT(b >= a + PAGE_SIZE);
    KTEST_ASSERT(c >= b + PAGE_SIZE);

    /* A multi-page block is aligned, follows its neighbor, and fits in RAM. */
    paddr_t d = alloc_pages(4);
    KTEST_ASSERT(is_aligned(d, PAGE_SIZE));
    KTEST_ASSERT(d >= c + PAGE_SIZE);
    KTEST_ASSERT((paddr_t) __free_ram_end - d >= 4 * PAGE_SIZE);

    ktest_end("stage2_align");
}
