/* SPDX-License-Identifier: MIT
 *
 * Stage 4 test for both full file slots across a reboot.
 */
#include "assert.h"
#include "kernel.h"

struct file *fs_lookup(const char *filename);
void handle_syscall(struct trap_frame *f);

#define FIRST_NAME "hello.txt"
#define SECOND_NAME "created.txt"

static char first_data[1024];
static char second_data[1024];

static bool has_bytes(struct file *file, char value) {
    if (file == NULL || file->size != 1024)
        return false;
    for (size_t i = 0; i < sizeof(file->data); i++) {
        if (file->data[i] != value)
            return false;
    }
    return true;
}

static uint32_t write_file(const char *name, const char *data, size_t size) {
    struct trap_frame f = {0};
    f.a3 = SYS_WRITEFILE;
    f.a0 = (uint32_t) name;
    f.a1 = (uint32_t) data;
    f.a2 = size;
    handle_syscall(&f);
    return f.a0;
}

void ktest_main(void) {
    memset(first_data, 'A', sizeof(first_data));
    memset(second_data, 'B', sizeof(second_data));
    ktest_begin("stage4_create_file");

    struct file *first = fs_lookup(FIRST_NAME);
    struct file *second = fs_lookup(SECOND_NAME);
    if (KTEST_BOOT == 1) {
        KTEST_ASSERT(first != NULL && second == NULL);
        uint32_t first_written = write_file(FIRST_NAME, first_data, sizeof(first_data));
        uint32_t second_written = write_file(SECOND_NAME, second_data, sizeof(second_data));
        KTEST_ASSERT(first_written == sizeof(first_data));
        KTEST_ASSERT(second_written == sizeof(second_data));
        first = fs_lookup(FIRST_NAME);
        second = fs_lookup(SECOND_NAME);
        KTEST_ASSERT(first != NULL && second != NULL);
        KTEST_ASSERT(first != NULL && first->size == sizeof(first_data) &&
                      has_bytes(first, 'A'));
        KTEST_ASSERT(second != NULL && second->size == sizeof(second_data) &&
                      has_bytes(second, 'B'));
    } else {
        KTEST_ASSERT(first != NULL);
        KTEST_ASSERT(second != NULL);
        KTEST_ASSERT(first != NULL && first->size == sizeof(first_data));
        KTEST_ASSERT(second != NULL && second->size == sizeof(second_data));
        KTEST_ASSERT(has_bytes(first, 'A'));
        KTEST_ASSERT(has_bytes(second, 'B'));
    }
    ktest_end("stage4_create_file");
}
