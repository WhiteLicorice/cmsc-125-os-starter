/* SPDX-License-Identifier: MIT
 *
 * Self-test fixture: a case with a header whose name holds a space.
 *
 * The case reads SPACE_HEADER_VALUE from "header with spaces.h". The self-test
 * changes the header and runs the old image with --no-build.
 */
#include "assert.h"
#include "header with spaces.h"

void ktest_main(void) {
    ktest_begin("space_header");
    KTEST_ASSERT(SPACE_HEADER_VALUE);
    ktest_end("space_header");
}
