/* SPDX-License-Identifier: MIT
 *
 * This fixture includes the accepted idle words in a longer panic message.
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
