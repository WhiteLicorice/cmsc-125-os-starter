/* SPDX-License-Identifier: MIT
 *
 * The record writer. A test kernel links this file under -DKTEST.
 * The record grammar is documented in README.md.
 *
 * Every record starts on a new line. The controller accepts a record only at
 * the start of a line, so case console output cannot merge into a record.
 *
 * The starter and the reference OS share this file byte for byte.
 */
#include "assert.h"

static unsigned ktest_assertions;
static unsigned ktest_failures;

void ktest_build(void) {
    printf("\nKTEST BUILD id=%s\n", KTEST_BUILD_ID);
}

void ktest_begin(const char *name) {
    ktest_assertions = 0;
    ktest_failures = 0;
    printf("\nKTEST BEGIN name=%s\n", name);
}

void ktest_assert(bool ok, const char *file, int line, const char *expr) {
    ktest_assertions++;
    if (ok) {
        printf("\nKTEST ASSERT ok at=%s:%d expr=%s\n", file, line, expr);
    } else {
        ktest_failures++;
        printf("\nKTEST ASSERT fail at=%s:%d expr=%s\n", file, line, expr);
    }
}

void ktest_end(const char *name) {
    printf("\nKTEST END name=%s assertions=%d failures=%d\n", name,
           (int) ktest_assertions, (int) ktest_failures);
}

void ktest_idle(void) {
    printf("\nKTEST IDLE\n");
    for (;;)
        __asm__ __volatile__("wfi");
}
