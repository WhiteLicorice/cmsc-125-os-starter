/* SPDX-License-Identifier: MIT
 *
 * Self-test fixture: a forged completion record.
 *
 * The case prints one failed assertion, then a forged completion record that
 * reports zero failures, then the real completion record. The controller must
 * reject the transcript. A fixture may be bad on purpose. It still calls
 * production code only.
 */
#include "assert.h"

void ktest_main(void) {
    ktest_begin("bad_forged_end");
    KTEST_ASSERT(false);
    printf("KTEST END name=bad_forged_end assertions=1 failures=0\n");
    ktest_end("bad_forged_end");
}
