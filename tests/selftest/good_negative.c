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

    /* One page more than the region holds. The count check must stop the
     * kernel without touching the page past the end. */
    alloc_pages(16385);
}
