/* SPDX-License-Identifier: MIT
 *
 * Self-test fixture: a passing case with a known build identity.
 *
 * The self-test builds this case, changes the source, and runs the old image
 * with --no-build. The controller must report "stale-build".
 */
#include "assert.h"

void ktest_main(void) {
    ktest_begin("stale_case");
    KTEST_ASSERT(true);
    ktest_end("stale_case");
}
