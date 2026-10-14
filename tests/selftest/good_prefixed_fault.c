/* SPDX-License-Identifier: MIT
 *
 * Self-test fixture: a declared fault after console text.
 *
 * The case prints console text without a newline and then panics with the
 * declared fault. The panic starts partway through the line. The controller
 * must still read the message and accept the case.
 */
#include "assert.h"
#include "kernel.h"

void ktest_main(void) {
    ktest_begin("good_prefixed_fault");
    KTEST_ASSERT(true);
    printf("ordinary console text ");
    PANIC("declared probe fault");
}
