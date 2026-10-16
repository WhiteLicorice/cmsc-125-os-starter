/* SPDX-License-Identifier: MIT
 *
 * Self-test fixture: a case-local header for the build-identity check.
 *
 * The self-test builds the case, changes this header, and runs the old image
 * with --no-build. The controller must report "stale-build" because the identity covers every
 * header the compiler reads.
 */
#define CASE_VALUE 1
