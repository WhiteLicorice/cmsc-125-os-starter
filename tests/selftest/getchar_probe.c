/* SPDX-License-Identifier: MIT
 *
 * Self-test fixture: console input through the production path.
 *
 * The controller sends the fixture tests/selftest/getchar_probe.input. The
 * case reads one byte through SYS_GETCHAR and checks it. It builds once the
 * kernel has the stage 3 system-call mechanism, `handle_syscall`, so the gate
 * runs it only in a tree with shell.c.
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
