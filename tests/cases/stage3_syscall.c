/* SPDX-License-Identifier: MIT
 *
 * Stage 3 seed case: applications and system calls.
 *
 * T07 writes the full suite. This seed checks the trap-frame ABI, the system
 * call dispatch, and the user-mode mapping that create_process builds.
 */
#include "assert.h"
#include "kernel.h"

struct process *create_process(const void *image, size_t image_size);
void handle_syscall(struct trap_frame *f);
void putchar(char ch);

static uint8_t image[PAGE_SIZE];

void ktest_main(void) {
    ktest_begin("stage3_syscall");

    KTEST_ASSERT(sizeof(struct trap_frame) == 31 * sizeof(uint32_t));
    KTEST_ASSERT(offsetof(struct trap_frame, a0) == 10 * sizeof(uint32_t));
    KTEST_ASSERT(offsetof(struct trap_frame, a3) == 13 * sizeof(uint32_t));

    struct trap_frame f;
    memset(&f, 0, sizeof(f));
    f.a0 = 'K';
    f.a3 = SYS_PUTCHAR;
    handle_syscall(&f);
    putchar('\n'); /* End the console output, so the next record starts a line. */
    KTEST_ASSERT(f.a0 == 'K');

    struct process *p = create_process(image, PAGE_SIZE);
    KTEST_ASSERT(p != NULL);
    KTEST_ASSERT(p->state == PROC_RUNNABLE);

    uint32_t vpn1 = (USER_BASE >> 22) & 0x3ff;
    uint32_t vpn0 = (USER_BASE >> 12) & 0x3ff;
    KTEST_ASSERT((p->page_table[vpn1] & PAGE_V) != 0);

    uint32_t *table0 = (uint32_t *) ((p->page_table[vpn1] >> 10) * PAGE_SIZE);
    KTEST_ASSERT((table0[vpn0] & (PAGE_V | PAGE_U)) == (PAGE_V | PAGE_U));

    /* Leave no runnable process behind, so the kernel reaches the idle state. */
    p->state = PROC_UNUSED;

    ktest_end("stage3_syscall");
}
