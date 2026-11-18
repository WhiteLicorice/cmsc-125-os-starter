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

    . = ALIGN(16);
    . += 128 * 1024; /* 128KB */
    __stack_top = .;

    . = ALIGN(4096);
    __free_ram = .;
    . += 64 * 1024 * 1024; /* 64MB */
    __free_ram_end = .;
}

/* Boot allocations and the stage 1 zeroing case share this region. The case
 * writes four pages past the first page it reads, so the four stage suites
 * need 64 KiB. Booting the shell needs more. The smallest measured region that
 * both passes every stage and reaches the shell prompt is 108 KiB, so the floor
 * is 112 KiB, the next multiple of 16 KiB. */
ASSERT(__free_ram_end - __free_ram >= 28 * 4096,
       "free region too small: __free_ram_end - __free_ram must be at least 114688 bytes (28 pages); enlarge the reservation after __free_ram in kernel.ld")
