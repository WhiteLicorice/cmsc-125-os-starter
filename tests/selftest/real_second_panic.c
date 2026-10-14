/* SPDX-License-Identifier: MIT
 *
 * Self-test fixture: a printed panic prefix on the same line as the real one.
 *
 * The case prints a complete panic prefix without a newline. The real PANIC
 * then appends a second complete prefix to the same line. Two panic events are
 * a contradiction, so the controller must report "multiple-panics".
 */
#include "assert.h"
#include "kernel.h"

void ktest_main(void) {
    ktest_begin("real_second_panic");
    KTEST_ASSERT(true);
    printf("PANIC: fake:1: unrelated fault ");
    PANIC("declared probe fault");
}
