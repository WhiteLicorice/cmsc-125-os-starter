/* SPDX-License-Identifier: MIT
 *
 * Self-test fixture: no completion record.
 *
 * The case begins and prints one assertion, then stops. The controller must
 * report "no-completion".
 */
#include "assert.h"

void ktest_main(void) {
    ktest_begin("bad_no_completion");
    KTEST_ASSERT(true);
    for (;;)
        __asm__ __volatile__("wfi");
}
