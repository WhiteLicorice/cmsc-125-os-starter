/* SPDX-License-Identifier: MIT
 *
 * This Stage 1 case checks that the cooperative scheduler picks the next runnable process.
 *
 * Two kernel-mode test processes record one step and yield. The scheduler
 * runs them in a fixed order, so the case asserts the order of their steps.
 * A yield that picks the wrong next process changes the order.
 *
 * The case builds once the kernel has a process mechanism: `create_process`,
 * `switch_context`, and `yield`. Until then the build fails, which is the
 * expected start state.
 */
#include "assert.h"
#include "kernel.h"

/* The kernel source defines these. The book declares no header for them. */
struct process *create_process(const void *image, size_t image_size);
void yield(void);

extern struct process *current_proc;
extern struct process *idle_proc;

#define STEPS 32

static char trace[STEPS];
static int nsteps;

static void record(char step) {
    if (nsteps < STEPS)
        trace[nsteps++] = step;
}

/* Run a kernel function as a process entry point. create_process builds the
 * switch_context frame with the user entry point as the return address. Replace
 * that address, so the process runs kernel code on its own kernel stack. */
static void start_kernel_process(void (*entry)(void)) {
    struct process *proc = create_process(NULL, 0);
    ((uint32_t *) proc->sp)[0] = (uint32_t) entry;
}

static void proc_a(void) {
    record('A');
    yield();
    record('A');
    yield();
    record('A');
    current_proc->state = PROC_EXITED;
    yield();
}

static void proc_b(void) {
    record('B');
    yield();
    record('B');
    yield();
    record('B');
    current_proc->state = PROC_EXITED;
    yield();
}

void ktest_main(void) {
    ktest_begin("stage1_switch");

    start_kernel_process(proc_a);
    start_kernel_process(proc_b);

    yield(); /* Run the two processes to completion. */

    KTEST_ASSERT(nsteps == 6);
    KTEST_ASSERT(trace[0] == 'A');
    KTEST_ASSERT(trace[1] == 'B');
    KTEST_ASSERT(trace[2] == 'A');
    KTEST_ASSERT(trace[3] == 'B');
    KTEST_ASSERT(trace[4] == 'A');
    KTEST_ASSERT(trace[5] == 'B');
    KTEST_ASSERT(current_proc == idle_proc);

    ktest_end("stage1_switch");
}
