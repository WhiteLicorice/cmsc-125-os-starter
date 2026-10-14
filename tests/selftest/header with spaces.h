/* SPDX-License-Identifier: MIT
 *
 * Self-test fixture: a case-local header whose name holds a space.
 *
 * The self-test builds the case, changes this header, and runs the old image
 * with --no-build. The compiler escapes the space in its dependency list, so
 * the controller must parse the escaped name and report "stale-build".
 */
#define SPACE_HEADER_VALUE 1
