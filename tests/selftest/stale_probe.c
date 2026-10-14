/* SPDX-License-Identifier: MIT
 *
 * Self-test fixture: a passing case with a known build identity.
 *
 * The self-test first runs this case normally, then changes a source and runs
 * it again with --no-build. The controller must report "stale-build".
 */
#include "assert.h"

void ktest_main(void) {
    ktest_begin("stale_probe");
    KTEST_ASSERT(true);
    ktest_end("stale_probe");
}
