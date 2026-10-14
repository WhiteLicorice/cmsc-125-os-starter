/* SPDX-License-Identifier: MIT
 *
 * Self-test fixture: a declared fault after a failed assertion.
 *
 * The case declares the fault "declared probe fault", but a failed assertion
 * comes first. The controller must report "assertion-failed".
 */
#include "assert.h"
#include "kernel.h"

void ktest_main(void) {
    ktest_begin("bad_fault_after_failed_assert");
    KTEST_ASSERT(false);
    PANIC("declared probe fault");
}
