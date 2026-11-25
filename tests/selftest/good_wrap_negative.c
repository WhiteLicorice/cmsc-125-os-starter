/* SPDX-License-Identifier: MIT
 *
 * Self-test fixture: an intended negative fault from a wrapped byte size.
 *
 * The case records completion before its declared fault. The count is large
 * enough that its byte size reaches 2^32 and wraps to zero in 32-bit unsigned
 * arithmetic, so the allocator must stop at the page count before it
 * multiplies.
 */
#include "assert.h"
#include "kernel.h"

paddr_t alloc_pages(uint32_t n);

void ktest_main(void) {
    ktest_begin("good_wrap_negative");
    KTEST_ASSERT(PAGE_SIZE == 4096);
    ktest_end("good_wrap_negative");

    /* 0x100000 pages of 4 KiB each is 2^32 bytes, which is zero in 32 bits. */
    alloc_pages(0x100000);
}
