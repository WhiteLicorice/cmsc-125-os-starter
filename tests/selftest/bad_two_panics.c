/* SPDX-License-Identifier: MIT
 *
 * Self-test fixture: two panic events on separate lines.
 *
 * The case declares one fault but prints two panic records. A second panic is
 * a contradiction, so the controller must report "multiple-panics". The case
 * does not return, so the reference OS (instructor only) cannot add its own
 * idle panic.
 */
#include "assert.h"

void ktest_main(void) {
    ktest_begin("bad_two_panics");
    KTEST_ASSERT(true);
    printf("PANIC: fake:1: declared probe fault\n");
    printf("PANIC: fake:2: declared probe fault\n");
    for (;;)
        __asm__ __volatile__("wfi");
}
