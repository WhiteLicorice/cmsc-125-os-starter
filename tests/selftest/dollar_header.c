/* SPDX-License-Identifier: MIT
 *
 * Self-test fixture: a case with a case-local header whose name holds a dollar.
 *
 * The compiler escapes the dollar as "$$" in its dependency list. A second
 * readable file named "dollar$$name.h" sits beside the real header. The
 * self-test changes only the real header and runs the old image with
 * --no-build. The identity must use the dependency path the compiler reports.
 * This check changes the real header and expects "stale-build". It leaves the
 * doubled-dollar decoy unchanged.
 */
#include "assert.h"
#include "dollar$name.h"

void ktest_main(void) {
    ktest_begin("dollar_header");
    KTEST_ASSERT(DOLLAR_VALUE);
    ktest_end("dollar_header");
}
