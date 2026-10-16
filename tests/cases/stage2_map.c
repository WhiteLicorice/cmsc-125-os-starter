/* SPDX-License-Identifier: MIT
 *
 * This Stage 2 case checks that map_page builds an Sv32 mapping that translates correctly.
 *
 * The case builds one level-1 table, maps three virtual addresses to three
 * physical pages, and walks the table the way the MMU does: from the level-1
 * entry to the level-0 table, then to the leaf entry. Two addresses share a
 * level-1 entry. The third uses a different level-1 entry. A map_page that installs the
 * wrong physical page, the wrong leaf index, or the wrong level fails here.
 *
 * The starter has no page table, so the build fails on the starter. That
 * failure is the expected start state.
 */
#include "assert.h"
#include "kernel.h"

/* The kernel source defines these. The book declares no header for them. */
paddr_t alloc_pages(uint32_t n);
void map_page(uint32_t *table1, uint32_t vaddr, paddr_t paddr, uint32_t flags);

/* Walk the two Sv32 levels, as the MMU does, and return the physical page the
 * leaf entry encodes. An absent level-1 entry, or an absent leaf, means the
 * address does not translate. */
static paddr_t sv32_translate(uint32_t *table1, uint32_t vaddr) {
    uint32_t vpn1 = (vaddr >> 22) & 0x3ff;
    uint32_t pte1 = table1[vpn1];
    if ((pte1 & PAGE_V) == 0)
        return (paddr_t) -1;

    uint32_t *table0 = (uint32_t *) ((pte1 >> 10) * PAGE_SIZE);
    uint32_t vpn0 = (vaddr >> 12) & 0x3ff;
    uint32_t pte0 = table0[vpn0];
    if ((pte0 & PAGE_V) == 0)
        return (paddr_t) -1;

    return (paddr_t) ((pte0 >> 10) * PAGE_SIZE);
}

void ktest_main(void) {
    ktest_begin("stage2_map");

    uint32_t *table1 = (uint32_t *) alloc_pages(1);
    paddr_t pa = alloc_pages(1);
    paddr_t pb = alloc_pages(1);
    paddr_t pc = alloc_pages(1);

    /* va1 and va2 sit in one 4 MiB region and share a level-1 entry. va3 sits
     * in the next region and uses a different one. */
    uint32_t va1 = 0x80000000;
    uint32_t va2 = 0x80001000;
    uint32_t va3 = 0x80400000;

    map_page(table1, va1, pa, PAGE_R | PAGE_W);
    map_page(table1, va2, pb, PAGE_R | PAGE_W);
    map_page(table1, va3, pc, PAGE_R | PAGE_W);

    /* Each virtual address translates to the physical page it was given. */
    KTEST_ASSERT(sv32_translate(table1, va1) == pa);
    KTEST_ASSERT(sv32_translate(table1, va2) == pb);
    KTEST_ASSERT(sv32_translate(table1, va3) == pc);

    /* The three mappings point to three different physical pages. */
    KTEST_ASSERT(sv32_translate(table1, va1) != sv32_translate(table1, va2));
    KTEST_ASSERT(sv32_translate(table1, va1) != sv32_translate(table1, va3));

    /* Addresses va1 and va2 share a level-1 entry. Address va3 uses a different
 * level-1 entry. */
    uint32_t vpn1_1 = (va1 >> 22) & 0x3ff;
    uint32_t vpn1_2 = (va2 >> 22) & 0x3ff;
    uint32_t vpn1_3 = (va3 >> 22) & 0x3ff;
    KTEST_ASSERT(vpn1_1 == vpn1_2);
    KTEST_ASSERT(vpn1_1 != vpn1_3);
    KTEST_ASSERT(table1[vpn1_1] == table1[vpn1_2]);
    KTEST_ASSERT(table1[vpn1_1] != table1[vpn1_3]);

    /* The two addresses in one region reach distinct leaves. */
    uint32_t *table0 = (uint32_t *) ((table1[vpn1_1] >> 10) * PAGE_SIZE);
    uint32_t vpn0_1 = (va1 >> 12) & 0x3ff;
    uint32_t vpn0_2 = (va2 >> 12) & 0x3ff;
    KTEST_ASSERT(vpn0_1 != vpn0_2);
    KTEST_ASSERT((table0[vpn0_1] >> 10) != (table0[vpn0_2] >> 10));

    /* An address that was never mapped does not translate. */
    KTEST_ASSERT(sv32_translate(table1, 0x40000000) == (paddr_t) -1);

    ktest_end("stage2_map");
}
