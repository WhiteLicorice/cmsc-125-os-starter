/* SPDX-License-Identifier: MIT
 *
 * Self-test fixture: a record after a terminal panic.
 *
 * The case prints a begin record, a declared panic, and a failed assertion
 * after the panic. A panic ends the transcript, so no record may follow it.
 * The controller must reject the transcript. A fixture may be bad on purpose.
 * It still calls production code only.
 */
#include "assert.h"

void ktest_main(void) {
    ktest_begin("bad_record_after_panic");
    printf("PANIC: fake:1: declared probe fault\n");
    KTEST_ASSERT(false);
    for (;;)
        __asm__ __volatile__("wfi");
}
