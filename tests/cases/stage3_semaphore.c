/* SPDX-License-Identifier: MIT
 *
 * Block a consumer on an empty semaphore. A producer fills it, and the
 * consumer resumes. The records prove the block, the wake, and the counts.
 */
#include "assert.h"
#include "kernel.h"

struct process *create_process(const void *image, size_t image_size);
void yield(void);

extern int sem_count[];
extern struct process *current_proc;
extern struct process *idle_proc;

#define ADDI(rd, rs1, imm) \
    ((((uint32_t) (imm) & 0xfff) << 20) | ((rs1) << 15) | ((rd) << 7) | 0x13)
#define ECALL 0x00000073

static const uint32_t consumer_image[] = {
    ADDI(10, 0, SEM_FULL),
    ADDI(13, 0, SYS_SEM_WAIT),
    ECALL,
    ADDI(10, 0, 'C'),
    ADDI(13, 0, SYS_PUTCHAR),
    ECALL,
    ADDI(10, 0, SEM_EMPTY),
    ADDI(13, 0, SYS_SEM_POST),
    ECALL,
    ADDI(13, 0, SYS_EXIT),
    ECALL,
};

static const uint32_t producer_image[] = {
    ADDI(10, 0, SEM_EMPTY),
    ADDI(13, 0, SYS_SEM_WAIT),
    ECALL,
    ADDI(10, 0, 'P'),
    ADDI(13, 0, SYS_PUTCHAR),
    ECALL,
    ADDI(10, 0, SEM_FULL),
    ADDI(13, 0, SYS_SEM_POST),
    ECALL,
    ADDI(13, 0, SYS_EXIT),
    ECALL,
};

void ktest_main(void) {
    ktest_begin("stage3_semaphore");

    struct process *consumer = create_process(consumer_image, sizeof(consumer_image));
    yield();

    KTEST_ASSERT(consumer->state == PROC_BLOCKED);
    KTEST_ASSERT(sem_count[SEM_EMPTY] == 1 && sem_count[SEM_FULL] == 0);

    struct process *producer = create_process(producer_image, sizeof(producer_image));
    yield();

    KTEST_ASSERT(consumer->state == PROC_EXITED);
    KTEST_ASSERT(producer->state == PROC_EXITED && current_proc == idle_proc);
    KTEST_ASSERT(sem_count[0] + sem_count[1] == 1);

    ktest_end("stage3_semaphore");
}
