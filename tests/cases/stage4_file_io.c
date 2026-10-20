/* SPDX-License-Identifier: MIT
 *
 * Stage 4 test for a file write and read through the system-call handler.
 */
#include "assert.h"
#include "kernel.h"

void handle_syscall(struct trap_frame *f);

void ktest_main(void) {
    static const char name[] = "hello.txt";
    static const char written[] = "One boot round trip.\n";
    char readback[64] = {0};
    struct trap_frame f = {0};

    ktest_begin("stage4_file_io");
    f.a3 = SYS_WRITEFILE;
    f.a0 = (uint32_t) name;
    f.a1 = (uint32_t) written;
    f.a2 = sizeof(written) - 1;
    handle_syscall(&f);
    KTEST_ASSERT(f.a0 == sizeof(written) - 1);

    f = (struct trap_frame) {0};
    f.a3 = SYS_READFILE;
    f.a0 = (uint32_t) name;
    f.a1 = (uint32_t) readback;
    f.a2 = sizeof(readback) - 1;
    handle_syscall(&f);
    KTEST_ASSERT(f.a0 == sizeof(written) - 1);
    KTEST_ASSERT(strcmp(readback, written) == 0);
    ktest_end("stage4_file_io");
}
