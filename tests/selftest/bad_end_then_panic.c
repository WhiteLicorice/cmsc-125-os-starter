/* SPDX-License-Identifier: MIT
 *
 * Self-test fixture: an unrelated panic after the completion record.
 *
 * The case completes, then panics with a message that is not the idle panic.
 * The controller must report "unrelated-panic".
 */
#include "assert.h"
#include "kernel.h"

void ktest_main(void) {
    ktest_begin("bad_end_then_panic");
    KTEST_ASSERT(true);
    ktest_end("bad_end_then_panic");
    PANIC("unrelated failure");
}
