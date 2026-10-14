/* SPDX-License-Identifier: MIT
 *
 * Self-test fixture: the panic token without the file and line fields.
 *
 * The case prints the bare text "PANIC: " in ordinary console output, then
 * passes. No panic occurred, so the controller must accept the case.
 */
#include "assert.h"

void ktest_main(void) {
    ktest_begin("ordinary_panic");
    printf("the token PANIC: is ordinary output\n");
    KTEST_ASSERT(true);
    ktest_end("ordinary_panic");
}
