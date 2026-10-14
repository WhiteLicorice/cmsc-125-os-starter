/* SPDX-License-Identifier: MIT
 *
 * Self-test fixture: an unrelated panic.
 *
 * The case declares no fault, so the controller must report
 * "unrelated-panic".
 */
#include "assert.h"
#include "kernel.h"

void ktest_main(void) {
    ktest_begin("bad_unrelated_panic");
    KTEST_ASSERT(true);
    PANIC("deliberate unrelated panic");
}
