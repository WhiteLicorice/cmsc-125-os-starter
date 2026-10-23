/* SPDX-License-Identifier: MIT
 *
 * This Stage 1 negative case checks that an illegal instruction in kernel mode
 * reaches the trap handler, which stops the kernel with the cause printed.
 *
 * The case starts one kernel-mode process and yields to it. The process
 * executes a word that RV32 does not define. The trap entry saves the frame and
 * calls the handler, and the handler panics with `unexpected trap scause=...`.
 *
 * The manifest declares that PANIC pattern. The case passes only when the panic
 * appears after a valid completion record.
 *
 * The case builds once the kernel has a trap entry, a handler that panics on an
 * unexpected trap, and the process mechanism: `create_process` and `yield`.
 * Until then the build fails, which is the expected start state.
 */
#include "assert.h"
#include "kernel.h"

/* The kernel source defines these. The book declares no header for them. */
struct process *create_process(const void *image, size_t image_size);
void yield(void);

extern struct process *current_proc;
extern struct process *idle_proc;

/* All ones. An encoding of all ones that spans ILEN bits is illegal, and the
 * specification gives that encoding the length ILEN. A base-only RV32
 * implementation has ILEN 32, and this target decodes no instruction longer
 * than 32 bits. A 32-bit word of all ones fills every one of those bits, so the
 * specification makes the word an illegal instruction of 32 bits. The rule
 * turns on instruction length, not on register width, and the hart raises an
 * illegal-instruction exception rather than fetch the word. `ld` is illegal
 * here too, because it is an RV64I load and the target is RV32, but its
 * encoding is 32 bits, so its length is not the reason. The assembler refuses
 * `ld` under `--target=riscv32`, so the case emits the raw word instead. */
#define ILLEGAL_WORD ".word 0xffffffff"

/* Run as the started process. The trap ends the boot before it can return. */
static void illegal_runner(void) {
    __asm__ __volatile__(ILLEGAL_WORD);
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
    ktest_begin("stage1_trap");

    KTEST_ASSERT(current_proc == idle_proc);

    start_kernel_process(illegal_runner);
    ktest_end("stage1_trap");
    yield(); /* Switch to the runner. Its illegal word traps. */
}
