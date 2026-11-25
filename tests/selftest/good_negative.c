/* SPDX-License-Identifier: MIT
 *
 * Self-test fixture: an intended negative fault.
 *
 * The case records completion before its declared fault. The controller must
 * accept the panic only after it validates that completion record.
 */
#include "assert.h"
#include "kernel.h"

paddr_t alloc_pages(uint32_t n);

void ktest_main(void) {
    ktest_begin("good_negative");
    KTEST_ASSERT(PAGE_SIZE == 4096);
    ktest_end("good_negative");

    /* One more page than the region holds, and a count whose byte size passes
     * 2^32. The second must panic before the page-count arithmetic wraps. */
    alloc_pages(0x100000);
}
