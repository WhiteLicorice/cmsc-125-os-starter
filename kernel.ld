/* SPDX-License-Identifier: MIT
 *
 * Derived from "Operating System in 1,000 Lines" by Seiya Nuta,
 * https://github.com/nuta/operating-system-in-1000-lines,
 * commit 012efb92b40be6a8a37ae3e1c2a0a15234315bfe.
 * See LICENSE for the full MIT permission notice.
 */
ENTRY(boot)

SECTIONS {
    . = 0x80200000;
    __kernel_base = .;

    .text :{
        KEEP(*(.text.boot));
        *(.text .text.*);
    }

    .rodata : ALIGN(4) {
        *(.rodata .rodata.*);
    }

    .data : ALIGN(4) {
        *(.data .data.*);
    }

    .bss : ALIGN(4) {
        __bss = .;
        *(.bss .bss.* .sbss .sbss.*);
        __bss_end = .;
    }

    . = ALIGN(4);
    . += 128 * 1024; /* 128KB */
    __stack_top = .;

    . = ALIGN(4096);
    __free_ram = .;
    . += 64 * 1024 * 1024; /* 64MB */
    __free_ram_end = .;
}
