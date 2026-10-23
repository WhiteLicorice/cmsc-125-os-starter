/* SPDX-License-Identifier: MIT
 *
 * Stage 4 seed case: disk and files.
 *
 * This seed checks the file table that fs_init loads from the disk image, and
 * one round trip through the block device. The controller gives the case a
 * fresh copy of the pristine image; all four stages' cases ship in the fork.
 */
#include "assert.h"
#include "kernel.h"

struct file *fs_lookup(const char *filename);
void fs_flush(void);

void ktest_main(void) {
    ktest_begin("stage4_fs");

    struct file *f = fs_lookup("hello.txt");
    KTEST_ASSERT(f != NULL);
    KTEST_ASSERT(f != NULL && strcmp(f->name, "hello.txt") == 0);
    KTEST_ASSERT(f != NULL && f->size > 0);
    KTEST_ASSERT(f != NULL && f->data[0] == 'H');
    KTEST_ASSERT(fs_lookup("absent.txt") == NULL);

    fs_flush();
    KTEST_ASSERT(fs_lookup("hello.txt") != NULL);

    ktest_end("stage4_fs");
}
