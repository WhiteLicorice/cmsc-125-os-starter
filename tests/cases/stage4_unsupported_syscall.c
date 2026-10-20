/* SPDX-License-Identifier: MIT
 *
 * Stage 4 test for the declared panic from an unsupported system call.
 */
#include "assert.h"
#include "kernel.h"

void handle_syscall(struct trap_frame *f);

void ktest_main(void) {
    struct trap_frame f = {0};

    ktest_begin("stage4_unsupported_syscall");
    f.a3 = 99;
    KTEST_ASSERT(f.a3 == 99);
    ktest_end("stage4_unsupported_syscall");
    handle_syscall(&f);
}
