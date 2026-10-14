/* SPDX-License-Identifier: MIT
 *
 * Self-test fixture: a missing input.
 *
 * The controller supplies no input, because the fixture does not exist. The
 * case waits for a byte that never comes, so the boot ends at the deadline
 * with no completion record. It needs the stage 3 syscall mechanism.
 */
#include "assert.h"
#include "kernel.h"

void handle_syscall(struct trap_frame *f);

void ktest_main(void) {
    ktest_begin("bad_getchar");

    struct trap_frame f;
    memset(&f, 0, sizeof(f));
    f.a3 = SYS_GETCHAR;
    handle_syscall(&f);

    KTEST_ASSERT(f.a0 == 'Z');

    ktest_end("bad_getchar");
}
