/* SPDX-License-Identifier: MIT
 *
 * Self-test fixture: the idle panic before the completion record.
 *
 * The case prints an idle panic, then the assertion and the completion record.
 * A panic ends the transcript, so the records after it are out of order. The
 * controller must reject the transcript and report "out-of-order-record". The
 * case does not return, so the reference OS cannot add its own idle panic.
 */
#include "assert.h"
#include "kernel.h"

void ktest_main(void) {
    ktest_begin("bad_idle_before_end");
    printf("PANIC: fake.c:1: switched to idle process\n");
    KTEST_ASSERT(true);
    ktest_end("bad_idle_before_end");
    for (;;)
        __asm__ __volatile__("wfi");
}
