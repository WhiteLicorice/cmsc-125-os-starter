/* SPDX-License-Identifier: MIT
 *
 * Self-test fixture: console text that precedes the idle panic.
 *
 * The case prints console text without a newline and returns. The reference
 * kernel then reaches the idle process and panics on the same line. The
 * controller must read the panic message and accept the case.
 */
#include "assert.h"

void ktest_main(void) {
    ktest_begin("idle_after_text");
    KTEST_ASSERT(true);
    ktest_end("idle_after_text");
    printf("trailing console text ");
}
