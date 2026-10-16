/* SPDX-License-Identifier: MIT
 *
 * This Stage 2 negative case checks that a load through an unmapped page
 * faults with a known cause.
 *
 * The case starts one kernel-mode process. Its page table maps the kernel but
 * leaves USER_BASE unmapped. A yield selects the process's page table before
 * the load. The unmapped load raises a page fault. The trap handler then panics
 * with the cause and faulting address.
 *
 * The manifest declares that PANIC pattern. The case passes only when that
 * pattern appears, alone, after the begin record. A run with no fault, or with
 * an unrelated panic, fails. The mutant set shows both.
 *
 * The starter has no page table, so the build fails on the starter. That
 * failure is the expected start state.
 */
#include "assert.h"
#include "kernel.h"

/* The kernel source defines these. The book declares no header for them. */
struct process *create_process(const void *image, size_t image_size);
void yield(void);

extern struct process *current_proc;
extern struct process *idle_proc;

/* USER_BASE. A process with no image leaves this address unmapped. */
#define UNMAPPED_ADDR 0x1000000

/* Run as the started process. The load faults before it can return. */
static void fault_reader(void) {
    volatile uint32_t *p = (volatile uint32_t *) UNMAPPED_ADDR;
    (void) *p;
    current_proc->state = PROC_EXITED;
    yield();
}

/* Run a kernel function as a process entry point. create_process builds the
 * switch frame with the user entry point as the return address. Replace that
 * address, so the process runs kernel code on its own kernel stack. */
static void start_kernel_process(void (*entry)(void)) {
    struct process *proc = create_process(NULL, 0);
    ((uint32_t *) proc->sp)[0] = (uint32_t) entry;
}

void ktest_main(void) {
    ktest_begin("stage2_fault");

    KTEST_ASSERT(current_proc == idle_proc);
    KTEST_ASSERT(idle_proc->page_table != NULL);

    start_kernel_process(fault_reader);
    yield(); /* Switch to the reader's page table, then fault. */

    ktest_end("stage2_fault");
}
