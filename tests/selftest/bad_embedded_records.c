/* SPDX-License-Identifier: MIT
 *
 * Self-test fixture: records embedded in console text.
 *
 * The case prints untrusted text that contains the record words, but never at
 * the start of a line, and then reaches the idle event. The controller must
 * treat the text as ordinary console output, so the case has no begin record.
 * The idle record then appears with no begin or end record, so the case fails
 * with "out-of-order-record".
 */
#include "assert.h"

void ktest_main(void) {
    printf("untrusted text KTEST BEGIN name=wrong\n");
    printf("untrusted text KTEST ASSERT fail at=fake.c:1 expr=0\n");
    printf("untrusted text KTEST END name=wrong assertions=1 failures=0\n");
    ktest_idle();
}
