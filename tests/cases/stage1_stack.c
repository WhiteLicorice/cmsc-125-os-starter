/* SPDX-License-Identifier: MIT
 *
 * Stage 1 case: a process keeps stack-local state across several yields.
 *
 * The case starts two kernel-mode test processes. Each process fills a local
 * array, yields, and checks the array after each yield. The tag for the array
 * comes from the process identity, and the check recomputes the tag, so a
 * process that reads another process's stack sees a mismatch straight away.
 *
 * The case first asserts that the two processes hold distinct stacks. A
 * create_process that shares one stack between processes fails that assertion.
 * The check stops before it runs the processes, because two processes on one
 * stack corrupt each other's saved context frames and the kernel then faults.
 *
 * The starter has no process mechanism, so the build fails on the starter.
 * That failure is the expected start state.
 */
#include "assert.h"
#include "kernel.h"

struct process *create_process(const void *image, size_t image_size);
void yield(void);

extern struct process *current_proc;
extern struct process *idle_proc;

#define STACK_N 8
#define ROUNDS 2

static int stack_checks;
static int stack_bad;

/* The array is volatile, so the compiler keeps it in memory on the process's
 * own stack. The tag comes from the process identity at fill time and again at
 * check time, so a shared stack holds one process's values under another
 * process's expectation. */
static void stack_worker(void) {
    volatile uint32_t local[STACK_N];
    uint32_t tag = (uint32_t) current_proc->pid * 0x10000;

    for (uint32_t i = 0; i < STACK_N; i++)
        local[i] = tag + i;

    for (int round = 0; round < ROUNDS; round++) {
        yield();

        uint32_t now = (uint32_t) current_proc->pid * 0x10000;
        for (uint32_t i = 0; i < STACK_N; i++) {
            if (local[i] != now + i)
                stack_bad++;
        }
        stack_checks++;
    }

    current_proc->state = PROC_EXITED;
    yield();
}

static struct process *start_stack_worker(void) {
    struct process *proc = create_process(NULL, 0);
    ((uint32_t *) proc->sp)[0] = (uint32_t) stack_worker;
    return proc;
}

void ktest_main(void) {
    ktest_begin("stage1_stack");

    struct process *a = start_stack_worker();
    struct process *b = start_stack_worker();

    KTEST_ASSERT(a != b);
    KTEST_ASSERT(a->sp != b->sp);

    if (a->sp != b->sp) {
        yield(); /* Run the two processes to completion. */
    } else {
        /* The stacks alias, so a run would corrupt the context frames. Keep
         * the kernel in the idle state and report the missing checks below. */
        a->state = PROC_UNUSED;
        b->state = PROC_UNUSED;
    }

    KTEST_ASSERT(stack_checks == ROUNDS * 2);
    KTEST_ASSERT(stack_bad == 0);
    KTEST_ASSERT(current_proc == idle_proc);

    ktest_end("stage1_stack");
}
