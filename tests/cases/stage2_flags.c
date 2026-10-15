/* SPDX-License-Identifier: MIT
 *
 * Stage 2 case: a leaf entry carries exactly the requested permission bits.
 *
 * The case maps three pages with different flag sets, then reads the entries
 * back. It compares all ten low bits of each entry against the lab's encoding,
 * not a five-bit subset, so a stray accessed, dirty, or reserved bit fails as
 * well as a missing or unrequested permission. A leaf must hold the requested
 * R, W, X, and U bits plus V, and nothing else. A level-1 entry holds V alone.
 *
 * The case then maps one page again with reduced permissions and a new frame.
 * A kernel that writes the leaf with `|=` keeps the old write bit and the old
 * frame, and fails the replacement check.
 *
 * The reference OS (instructor only) runs this case. The starter has no page
 * table, so the build fails on the starter. That failure is the expected start
 * state.
 */
#include "assert.h"
#include "kernel.h"

/* The kernel source defines these. The book declares no header for them. */
paddr_t alloc_pages(uint32_t n);
void map_page(uint32_t *table1, uint32_t vaddr, paddr_t paddr, uint32_t flags);

/* The ten low bits of an entry. They hold V, R, W, X, and U, and also the
 * G, A, D, and RSW bits. This lab's encoding leaves the last four clear, so
 * the case compares all ten. */
#define LOW_BITS_MASK 0x3ffu
#define PTE_PPN(pte) ((pte) >> 10)

/* The leaf entry for vaddr, or NULL when the level-1 entry is absent. */
static uint32_t *leaf_entry(uint32_t *table1, uint32_t vaddr) {
    uint32_t pte1 = table1[(vaddr >> 22) & 0x3ff];
    if ((pte1 & PAGE_V) == 0)
        return NULL;
    uint32_t *table0 = (uint32_t *) ((pte1 >> 10) * PAGE_SIZE);
    return &table0[(vaddr >> 12) & 0x3ff];
}

void ktest_main(void) {
    ktest_begin("stage2_flags");

    uint32_t *table1 = (uint32_t *) alloc_pages(1);
    paddr_t pa = alloc_pages(1);
    paddr_t pb = alloc_pages(1);
    paddr_t pc = alloc_pages(1);

    uint32_t va1 = 0x80000000; /* R | W */
    uint32_t va2 = 0x80001000; /* R | X | U */
    uint32_t va3 = 0x80400000; /* R only, different level-1 entry */
    map_page(table1, va1, pa, PAGE_R | PAGE_W);
    map_page(table1, va2, pb, PAGE_R | PAGE_X | PAGE_U);
    map_page(table1, va3, pc, PAGE_R);

    uint32_t vpn1_1 = (va1 >> 22) & 0x3ff;
    uint32_t vpn1_3 = (va3 >> 22) & 0x3ff;

    /* A level-1 entry carries V alone, across all ten low bits. */
    KTEST_ASSERT((table1[vpn1_1] & LOW_BITS_MASK) == PAGE_V);
    KTEST_ASSERT((table1[vpn1_3] & LOW_BITS_MASK) == PAGE_V);

    /* The level-1 entries name distinct level-0 tables. */
    KTEST_ASSERT(PTE_PPN(table1[vpn1_1]) != PTE_PPN(table1[vpn1_3]));

    /* A leaf carries exactly the requested bits, plus V. */
    uint32_t *leaf1 = leaf_entry(table1, va1);
    uint32_t *leaf2 = leaf_entry(table1, va2);
    uint32_t *leaf3 = leaf_entry(table1, va3);
    KTEST_ASSERT((*leaf1 & LOW_BITS_MASK) == (PAGE_V | PAGE_R | PAGE_W));
    KTEST_ASSERT((*leaf2 & LOW_BITS_MASK) == (PAGE_V | PAGE_R | PAGE_X | PAGE_U));
    KTEST_ASSERT((*leaf3 & LOW_BITS_MASK) == (PAGE_V | PAGE_R));

    /* The leaf names the requested physical page. */
    KTEST_ASSERT(PTE_PPN(*leaf1) == pa / PAGE_SIZE);
    KTEST_ASSERT(PTE_PPN(*leaf2) == pb / PAGE_SIZE);
    KTEST_ASSERT(PTE_PPN(*leaf3) == pc / PAGE_SIZE);

    /* Mapping the same page again replaces the old leaf. A kernel that ORs the
     * new entry into the old one keeps the old write bit and the old frame. */
    paddr_t pd = alloc_pages(1);
    uint32_t va4 = 0x80800000;
    map_page(table1, va4, pc, PAGE_R | PAGE_W);
    uint32_t *leaf4 = leaf_entry(table1, va4);
    KTEST_ASSERT((*leaf4 & LOW_BITS_MASK) == (PAGE_V | PAGE_R | PAGE_W));
    KTEST_ASSERT(PTE_PPN(*leaf4) == pc / PAGE_SIZE);

    map_page(table1, va4, pd, PAGE_R);
    KTEST_ASSERT((*leaf4 & LOW_BITS_MASK) == (PAGE_V | PAGE_R));
    KTEST_ASSERT(PTE_PPN(*leaf4) == pd / PAGE_SIZE);

    ktest_end("stage2_flags");
}
