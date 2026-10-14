/* SPDX-License-Identifier: MIT
 *
 * The assertion header. A test case includes this file and calls
 * KTEST_ASSERT. The record grammar is documented in README.md.
 */
#pragma once

#include "common.h"

/* The Makefile defines this. The value is a hash of the build inputs. */
#ifndef KTEST_BUILD_ID
#define KTEST_BUILD_ID "unknown"
#endif

/* The controller defines this. It numbers the boots of one image. A case that
 * checks persistence reads it to tell the first boot from the second. */
#ifndef KTEST_BOOT
#define KTEST_BOOT 1
#endif

void ktest_build(void);
void ktest_begin(const char *name);
void ktest_assert(bool ok, const char *file, int line, const char *expr);
void ktest_end(const char *name);
void ktest_idle(void);

/* A test case provides this entry point. The kernel calls it in test mode. */
void ktest_main(void);

#define KTEST_ASSERT(expr) \
    ktest_assert((expr) ? true : false, __FILE__, __LINE__, #expr)
