/* SPDX-License-Identifier: MIT
 *
 * Derived from "Operating System in 1,000 Lines" by Seiya Nuta,
 * https://github.com/nuta/operating-system-in-1000-lines,
 * commit 012efb92b40be6a8a37ae3e1c2a0a15234315bfe.
 * See LICENSE for the full MIT permission notice.
 */
ENTRY(start)

SECTIONS {
    . = 0x1000000;

    .text :{
        KEEP(*(.text.start));
        *(.text .text.*);
    }

    .rodata : ALIGN(4) {
        *(.rodata .rodata.*);
    }

    .data : ALIGN(4) {
        *(.data .data.*);
    }

    .bss : ALIGN(4) {
        *(.bss .bss.* .sbss .sbss.*);

        /*
         * The RISC-V ABI requires 16-byte alignment for the stack.
         */
        . = ALIGN(16);
        . += 64 * 1024; /* 64KB stack space */
        __stack_top = .;

       ASSERT(. < 0x1800000, "too large executable");
    }
}
