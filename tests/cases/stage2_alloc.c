/* SPDX-License-Identifier: MIT
 *
 * Stage 2 seed case: page allocation and page tables.
 *
 * The full suite is added later. This seed checks allocation alignment and zero
 * fill, and the two-level page table that map_page builds.
 */
#include "assert.h"
#include "kernel.h"

paddr_t alloc_pages(uint32_t n);
void map_page(uint32_t *table1, uint32_t vaddr, paddr_t paddr, uint32_t flags);

extern char __free_ram_end[];

void ktest_main(void) {
    ktest_begin("stage2_alloc");

    paddr_t p1 = alloc_pages(1);
    paddr_t p2 = alloc_pages(1);
    paddr_t p4 = alloc_pages(4);

    KTEST_ASSERT(is_aligned(p1, PAGE_SIZE));
    KTEST_ASSERT(is_aligned(p2, PAGE_SIZE));
    KTEST_ASSERT(is_aligned(p4, PAGE_SIZE));
    KTEST_ASSERT(p1 != p2);
    KTEST_ASSERT((paddr_t) __free_ram_end - p1 >= 4 * PAGE_SIZE);

    uint8_t *m = (uint8_t *) p1;
    bool zeroed = true;
    for (uint32_t i = 0; i < PAGE_SIZE; i++) {
        if (m[i] != 0) {
            zeroed = false;
            break;
        }
    }
    KTEST_ASSERT(zeroed);

    uint32_t *table1 = (uint32_t *) alloc_pages(1);
    uint32_t vaddr = 0x80000000;
    uint32_t flags = PAGE_R | PAGE_W;
    map_page(table1, vaddr, p1, flags);

    uint32_t vpn1 = (vaddr >> 22) & 0x3ff;
    uint32_t vpn0 = (vaddr >> 12) & 0x3ff;
    KTEST_ASSERT((table1[vpn1] & PAGE_V) != 0);

    uint32_t *table0 = (uint32_t *) ((table1[vpn1] >> 10) * PAGE_SIZE);
    KTEST_ASSERT(table0[vpn0] == (((p1 / PAGE_SIZE) << 10) | flags | PAGE_V));

    ktest_end("stage2_alloc");
}
