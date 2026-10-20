/* SPDX-License-Identifier: MIT
 *
 * Stage 4 test for a file change that survives a second boot.
 */
#include "assert.h"
#include "kernel.h"

struct file *fs_lookup(const char *filename);
void handle_syscall(struct trap_frame *f);

#define NAME "hello.txt"
#define UPDATED "Saved before reboot.\n"

void ktest_main(void) {
    char readback[sizeof(UPDATED)] = {0};
    struct trap_frame f = {0};

    ktest_begin("stage4_persistence");
    struct file *file = fs_lookup(NAME);
    KTEST_ASSERT(file != NULL);

    if (file != NULL && KTEST_BOOT == 1) {
        f.a3 = SYS_WRITEFILE;
        f.a0 = (uint32_t) NAME;
        f.a1 = (uint32_t) UPDATED;
        f.a2 = sizeof(UPDATED);
        handle_syscall(&f);
        uint32_t written = f.a0;

        f.a3 = SYS_READFILE;
        f.a0 = (uint32_t) NAME;
        f.a1 = (uint32_t) readback;
        f.a2 = sizeof(readback);
        handle_syscall(&f);
        KTEST_ASSERT(written == sizeof(UPDATED));
        KTEST_ASSERT(f.a0 == sizeof(UPDATED) && strcmp(readback, UPDATED) == 0);
    } else if (file != NULL) {
        f.a3 = SYS_READFILE;
        f.a0 = (uint32_t) NAME;
        f.a1 = (uint32_t) readback;
        f.a2 = sizeof(readback);
        handle_syscall(&f);
        KTEST_ASSERT(f.a0 == sizeof(UPDATED));
        KTEST_ASSERT(strcmp(readback, UPDATED) == 0);
    } else {
        KTEST_ASSERT(false);
        KTEST_ASSERT(false);
    }
    ktest_end("stage4_persistence");
}
