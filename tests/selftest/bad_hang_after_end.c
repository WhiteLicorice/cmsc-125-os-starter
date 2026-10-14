/* SPDX-License-Identifier: MIT
 *
 * Self-test fixture: a hang after the completion record.
 *
 * The case completes, then stops with no idle marker and no panic. The
 * controller must report "no-idle".
 */
#include "assert.h"

void ktest_main(void) {
    ktest_begin("bad_hang_after_end");
    KTEST_ASSERT(true);
    ktest_end("bad_hang_after_end");
    for (;;)
        __asm__ __volatile__("wfi");
}
