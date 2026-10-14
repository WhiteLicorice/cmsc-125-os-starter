/* SPDX-License-Identifier: MIT
 *
 * Self-test fixture: records for a different case.
 *
 * The case prints records for another case name and then reaches the idle
 * event. The controller must reject the transcript, because the begin record
 * names the wrong case.
 */
#include "assert.h"

void ktest_main(void) {
    printf("KTEST BEGIN name=other_case\n");
    printf("KTEST ASSERT ok at=fake.c:1 expr=1\n");
    printf("KTEST END name=other_case assertions=1 failures=0\n");
    ktest_idle();
}
