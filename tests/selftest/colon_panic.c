/* SPDX-License-Identifier: MIT
 *
 * Self-test fixture: a real PANIC after a #line with a colon in the file name.
 *
 * The line directive sets the file to "C:drive.c". The production PANIC macro
 * prints "PANIC: C:drive.c:10: declared probe fault". The file field may hold a
 * colon, so the controller must find the declared fault and pass the case.
 */
#include "assert.h"
#include "kernel.h"

#line 10 "C:drive.c"
void ktest_main(void){ktest_begin("colon_panic");KTEST_ASSERT(1);ktest_end("colon_panic");PANIC("declared probe fault");}
