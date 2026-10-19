/* SPDX-License-Identifier: MIT
 *
 * This application executes an illegal instruction in U-mode. The manifest
 * declares the expected exception cause.
 */
#include "assert.h"
#include "kernel.h"

struct process *create_process(const void *image, size_t image_size);
void yield(void);

static const uint32_t image[] = {0xffffffff};

void ktest_main(void) {
    ktest_begin("stage3_user_illegal");
    create_process(image, sizeof(image));
    yield();
    ktest_end("stage3_user_illegal");
}
