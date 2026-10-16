/* SPDX-License-Identifier: MIT
 *
 * This Stage 1 case checks that a context switch preserves callee-saved registers.
 *
 * Two kernel-mode test processes load s0 to s11 with known values, yield, and
 * read the registers back after they resume. The case asserts that each process
 * reads its own values. A switch_context that fails to save or restore one of
 * the registers hands a process the other process's value.
 *
 * The starter has no process mechanism, so the build fails on the starter. That
 * failure is the expected start state.
 */
#include "assert.h"
#include "kernel.h"

struct process *create_process(const void *image, size_t image_size);
void yield(void);

extern struct process *current_proc;
extern struct process *idle_proc;

#define REGS_BASE_A 0xAA000
#define REGS_BASE_B 0xBB000
#define REGS_N 12

/* The preprocessor expands each argument before it creates a string literal. */
#define STR2(x) #x
#define STR(x) STR2(x)

/* One row per process, one column per callee-saved register. The workers
 * store into it from inline assembly, so keep the symbol. */
static uint32_t regs_seen[2][REGS_N] __attribute__((used));
static int regs_ran;

/* Each worker loads the twelve registers, yields, and stores them back. The
 * other worker runs in between and loads its own values. If switch_context
 * fails to save or restore one register, a process can read the other process's value. */
#define REGS_WORKER(name, base, off)                                     \
    static void name(void) {                                             \
        __asm__ __volatile__(                                            \
            "li s0,  " STR(base) " + 0\n"                                    \
            "li s1,  " STR(base) " + 1\n"                                    \
            "li s2,  " STR(base) " + 2\n"                                    \
            "li s3,  " STR(base) " + 3\n"                                    \
            "li s4,  " STR(base) " + 4\n"                                    \
            "li s5,  " STR(base) " + 5\n"                                    \
            "li s6,  " STR(base) " + 6\n"                                    \
            "li s7,  " STR(base) " + 7\n"                                    \
            "li s8,  " STR(base) " + 8\n"                                    \
            "li s9,  " STR(base) " + 9\n"                                    \
            "li s10, " STR(base) " + 10\n"                                   \
            "li s11, " STR(base) " + 11\n"                                   \
            "call yield\n"                                               \
            "la t0, regs_seen\n"                                         \
            "addi t0, t0, " STR(off) "\n"                                    \
            "sw s0,  0(t0)\n"                                            \
            "sw s1,  4(t0)\n"                                            \
            "sw s2,  8(t0)\n"                                            \
            "sw s3,  12(t0)\n"                                           \
            "sw s4,  16(t0)\n"                                           \
            "sw s5,  20(t0)\n"                                           \
            "sw s6,  24(t0)\n"                                           \
            "sw s7,  28(t0)\n"                                           \
            "sw s8,  32(t0)\n"                                           \
            "sw s9,  36(t0)\n"                                           \
            "sw s10, 40(t0)\n"                                           \
            "sw s11, 44(t0)\n"                                           \
            :                                                            \
            :                                                            \
            : "s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7", "s8",      \
              "s9", "s10", "s11", "ra", "a0", "a1", "a2", "a3", "a4",    \
              "a5", "a6", "a7", "t0", "t1", "t2", "t3", "t4", "t5",      \
              "t6", "memory");                                           \
        current_proc->state = PROC_EXITED;                               \
        yield();                                                         \
    }

REGS_WORKER(proc_a, REGS_BASE_A, 0)
REGS_WORKER(proc_b, REGS_BASE_B, REGS_N * 4)

static void check_row(int row, uint32_t base) {
    for (int i = 0; i < REGS_N; i++)
        KTEST_ASSERT(regs_seen[row][i] == base + (uint32_t) i);
    regs_ran++;
}

void ktest_main(void) {
    ktest_begin("stage1_regs");

    /* create_process builds the entry frame. Run each worker as kernel code. */
    struct process *a = create_process(NULL, 0);
    ((uint32_t *) a->sp)[0] = (uint32_t) proc_a;
    struct process *b = create_process(NULL, 0);
    ((uint32_t *) b->sp)[0] = (uint32_t) proc_b;

    yield(); /* Run the two processes to completion. */

    check_row(0, REGS_BASE_A);
    check_row(1, REGS_BASE_B);
    KTEST_ASSERT(regs_ran == 2);
    KTEST_ASSERT(current_proc == idle_proc);

    ktest_end("stage1_regs");
}
