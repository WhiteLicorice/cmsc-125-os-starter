/* SPDX-License-Identifier: MIT
 *
 * Self-test fixture: the idle words inside a larger panic message.
 *
 * The panic message contains the accepted idle words but does not equal them.
 * The controller must report "unrelated-panic".
 */
#include "assert.h"
#include "kernel.h"

void ktest_main(void) {
    ktest_begin("bad_idle_substring");
    KTEST_ASSERT(true);
    ktest_end("bad_idle_substring");
    PANIC("unrelated failure mentioning switched to idle process");
}
