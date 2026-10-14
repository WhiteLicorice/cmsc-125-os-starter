/* SPDX-License-Identifier: MIT
 *
 * Self-test fixture: a case with a case-local header whose name holds a dollar.
 *
 * The compiler escapes the dollar as "$$" in its dependency list. A second
 * readable file named "dollar$$name.h" sits beside the real header. The
 * self-test changes only the real header and runs the old image with
 * --no-build. The controller must report "stale-build", so the identity must
 * hash the file the compiler read, not the doubled-dollar decoy.
 */
#include "assert.h"
#include "dollar$name.h"

void ktest_main(void) {
    ktest_begin("dollar_header");
    KTEST_ASSERT(DOLLAR_VALUE);
    ktest_end("dollar_header");
}
