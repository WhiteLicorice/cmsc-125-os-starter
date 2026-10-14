/* SPDX-License-Identifier: MIT
 *
 * Stage 1 seed case: cooperative processes.
 *
 * The case calls the production mechanisms of stage 1 and reports the result
 * through the record grammar. On the starter the mechanisms are absent, so the
 * build fails. The full suite is added later.
 */
#include "assert.h"
#include "kernel.h"

/* The kernel source defines these. The book declares no header for them. */
paddr_t alloc_pages(uint32_t n);
void yield(void);

extern struct process *current_proc;
extern struct process *idle_proc;
extern char __free_ram_end[];

void ktest_main(void) {
    ktest_begin("stage1_process");

    KTEST_ASSERT(idle_proc != NULL);
    KTEST_ASSERT(idle_proc->pid == 0);
    KTEST_ASSERT(current_proc == idle_proc);

    paddr_t a = alloc_pages(1);
    KTEST_ASSERT(is_aligned(a, PAGE_SIZE));
    KTEST_ASSERT(a + PAGE_SIZE <= (paddr_t) __free_ram_end);

    uint8_t *p = (uint8_t *) a;
    bool zeroed = true;
    for (uint32_t i = 0; i < PAGE_SIZE; i++) {
        if (p[i] != 0) {
            zeroed = false;
            break;
        }
    }
    KTEST_ASSERT(zeroed);

    paddr_t b = alloc_pages(1);
    KTEST_ASSERT(b != a);

    /* The idle process is the only process, so a yield returns to the caller. */
    yield();
    KTEST_ASSERT(current_proc == idle_proc);

    ktest_end("stage1_process");
}
