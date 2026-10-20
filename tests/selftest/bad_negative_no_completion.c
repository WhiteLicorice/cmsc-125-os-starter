/* SPDX-License-Identifier: MIT
 *
 * Self-test fixture: a declared fault without a completion record.
 */
#include "assert.h"
#include "kernel.h"

void handle_syscall(struct trap_frame *f);

void ktest_main(void) {
    struct trap_frame f = {0};

    ktest_begin("bad_negative_no_completion");
    KTEST_ASSERT(true);
    f.a3 = 99;
    handle_syscall(&f);
}
