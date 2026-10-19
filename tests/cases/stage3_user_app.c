/* SPDX-License-Identifier: MIT
 *
 * Run a small application from U-mode. It prints U and exits.
 */
#include "assert.h"
#include "kernel.h"

struct process *create_process(const void *image, size_t image_size);
void yield(void);

extern struct process *current_proc;
extern struct process *idle_proc;

#define ADDI(rd, rs1, imm) \
    ((((uint32_t) (imm) & 0xfff) << 20) | ((rs1) << 15) | ((rd) << 7) | 0x13)
#define ECALL 0x00000073

static const uint32_t image[] = {
    ADDI(10, 0, 'U'),
    ADDI(13, 0, SYS_PUTCHAR),
    ECALL,
    ADDI(13, 0, SYS_EXIT),
    ECALL,
};

void ktest_main(void) {
    ktest_begin("stage3_user_app");

    struct process *p = create_process(image, sizeof(image));
    KTEST_ASSERT(p->state == PROC_RUNNABLE);
    uint32_t vpn1 = (USER_BASE >> 22) & 0x3ff;
    uint32_t vpn0 = (USER_BASE >> 12) & 0x3ff;
    uint32_t *table0 = (uint32_t *) ((p->page_table[vpn1] >> 10) * PAGE_SIZE);
    KTEST_ASSERT((table0[vpn0] & (PAGE_V | PAGE_U | PAGE_X)) ==
                 (PAGE_V | PAGE_U | PAGE_X));
    yield();
    KTEST_ASSERT(p->state == PROC_EXITED);
    KTEST_ASSERT(current_proc == idle_proc);

    ktest_end("stage3_user_app");
}
