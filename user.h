/* SPDX-License-Identifier: MIT
 *
 * Derived from "Operating System in 1,000 Lines" by Seiya Nuta,
 * https://github.com/nuta/operating-system-in-1000-lines,
 * commit 012efb92b40be6a8a37ae3e1c2a0a15234315bfe.
 * See LICENSE for the full MIT permission notice.
 */
#pragma once
#include "common.h"

struct sysret {
    int a0;
    int a1;
    int a2;
};

void putchar(char ch);
int getchar(void);
int sem_wait(int id);
int sem_post(int id);
__attribute__((noreturn)) void exit(void);
