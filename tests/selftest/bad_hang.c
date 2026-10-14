/* SPDX-License-Identifier: MIT
 *
 * Self-test fixture: a deliberate hang.
 *
 * The case never prints a record, so the controller must report "hang". A
 * fixture may be bad on purpose. It still calls production code only.
 */
#include "assert.h"

void ktest_main(void) {
    for (;;)
        __asm__ __volatile__("wfi");
}
