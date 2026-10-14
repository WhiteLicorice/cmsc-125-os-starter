/* SPDX-License-Identifier: MIT
 *
 * Self-test fixture: a case with a case-local header.
 *
 * The case reads CASE_VALUE from header_case.h. The self-test changes the
 * header and runs the old image with --no-build.
 */
#include "assert.h"
#include "header_case.h"

void ktest_main(void) {
    ktest_begin("header_case");
    KTEST_ASSERT(CASE_VALUE);
    ktest_end("header_case");
}
