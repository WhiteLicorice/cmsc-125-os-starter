/* SPDX-License-Identifier: MIT
 *
 * Stage 2 case: the allocator returns page-aligned, non-overlapping blocks.
 *
 * The case asks for single pages and one multi-page block, then checks that
 * every returned address lies on a page boundary and that each block begins at
 * or after the end of the previous one. It fills the block, takes one more
 * page, and checks that the successor starts past the block's full extent and
 * that the block's bytes survive. A block that is misaligned, one that
 * overlaps its neighbor, or one whose reserved extent is too short fails here.
 *
 * The reference OS (instructor only) runs this case. The starter has no
 * allocator, so the build fails on the starter. That failure is the expected
 * start state.
 */
#include "assert.h"
#include "kernel.h"

/* The kernel source defines these. The book declares no header for them. */
paddr_t alloc_pages(uint32_t n);

extern char __free_ram_end[];

/* A per-page, per-byte pattern, so a later write can be told from the fill. */
static uint8_t block_byte(uint32_t page, uint32_t i) {
    return (uint8_t) (page * 31 + i * 7 + 1);
}

static void fill_block(uint8_t *base, uint32_t pages) {
    for (uint32_t p = 0; p < pages; p++) {
        for (uint32_t i = 0; i < PAGE_SIZE; i++)
            base[p * PAGE_SIZE + i] = block_byte(p, i);
    }
}

static bool block_ok(const uint8_t *base, uint32_t pages) {
    for (uint32_t p = 0; p < pages; p++) {
        for (uint32_t i = 0; i < PAGE_SIZE; i++) {
            if (base[p * PAGE_SIZE + i] != block_byte(p, i))
                return false;
        }
    }
    return true;
}

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

    /* Reserve the block, fill it, and take one more page. The successor must
     * start past the block's four pages, and the block's bytes must survive. */
    fill_block((uint8_t *) d, 4);

    paddr_t e = alloc_pages(1);
    KTEST_ASSERT(is_aligned(e, PAGE_SIZE));
    KTEST_ASSERT(e >= d + 4 * PAGE_SIZE);
    KTEST_ASSERT(block_ok((const uint8_t *) d, 4));

    ktest_end("stage2_align");
}
