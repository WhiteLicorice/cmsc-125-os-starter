/* SPDX-License-Identifier: MIT
 *
 * Stage 4 test for a missing-file read that returns -1.
 */
#include "assert.h"
#include "kernel.h"

void handle_syscall(struct trap_frame *f);

void ktest_main(void) {
    static const char name[] = "absent.txt";
    char buf[8] = {0};
    struct trap_frame f = {0};

    ktest_begin("stage4_missing_file");
    f.a3 = SYS_READFILE;
    f.a0 = (uint32_t) name;
    f.a1 = (uint32_t) buf;
    f.a2 = sizeof(buf);
    handle_syscall(&f);
    KTEST_ASSERT(f.a0 == (uint32_t) -1);
    ktest_end("stage4_missing_file");
}
