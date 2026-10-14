/* SPDX-License-Identifier: MIT
 *
 * Self-test fixture: console input through the production path.
 *
 * The controller sends the fixture tests/selftest/getchar_probe.input. The
 * case reads one byte through SYS_GETCHAR and checks it. It needs the stage 3
 * syscall mechanism, so only the reference OS (instructor only) runs it.
 */
#include "assert.h"
#include "kernel.h"

void handle_syscall(struct trap_frame *f);

void ktest_main(void) {
    ktest_begin("getchar_probe");

    struct trap_frame f;
    memset(&f, 0, sizeof(f));
    f.a3 = SYS_GETCHAR;
    handle_syscall(&f);

    KTEST_ASSERT(f.a0 == 'Z');

    ktest_end("getchar_probe");
}
