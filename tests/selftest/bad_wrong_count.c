/* SPDX-License-Identifier: MIT
 *
 * Self-test fixture: a wrong assertion count.
 *
 * The case prints three assertions. The self-test declares two, so the
 * controller must report "wrong-assertion-count".
 */
#include "assert.h"

void ktest_main(void) {
    ktest_begin("bad_wrong_count");
    KTEST_ASSERT(true);
    KTEST_ASSERT(true);
    KTEST_ASSERT(true);
    ktest_end("bad_wrong_count");
}
