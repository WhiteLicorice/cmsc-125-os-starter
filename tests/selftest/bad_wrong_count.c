/* SPDX-License-Identifier: MIT
 *
 * This fixture prints three assertions. The self-test expects two. The
 * controller reports "wrong-assertion-count".
 */
#include "assert.h"

void ktest_main(void) {
    ktest_begin("bad_wrong_count");
    KTEST_ASSERT(true);
    KTEST_ASSERT(true);
    KTEST_ASSERT(true);
    ktest_end("bad_wrong_count");
}
