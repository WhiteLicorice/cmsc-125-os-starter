/* SPDX-License-Identifier: MIT
 *
 * Self-test fixture: an intended negative fault.
 *
 * The allocator must refuse a request larger than the free region. The case
 * declares the expected fault, so the controller must accept the panic and
 * report a pass.
 */
#include "assert.h"
#include "kernel.h"

paddr_t alloc_pages(uint32_t n);

void ktest_main(void) {
    ktest_begin("good_negative");
    KTEST_ASSERT(PAGE_SIZE == 4096);
    alloc_pages(16385);
    /* Never reached: the allocator panics with "out of memory". */
    ktest_end("good_negative");
}
