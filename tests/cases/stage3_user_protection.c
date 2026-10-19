/* SPDX-License-Identifier: MIT
 *
 * This application writes to a supervisor-only page in U-mode. The manifest
 * declares the expected store page fault.
 */
#include "assert.h"
#include "kernel.h"

struct process *create_process(const void *image, size_t image_size);
void yield(void);

static const uint32_t image[] = {
    0x80200537, /* lui a0, 0x80200 */
    0x00052023, /* sw zero, 0(a0) */
};

void ktest_main(void) {
    ktest_begin("stage3_user_protection");
    create_process(image, sizeof(image));
    yield();
    ktest_end("stage3_user_protection");
}
