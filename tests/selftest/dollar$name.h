/* SPDX-License-Identifier: MIT
 *
 * Self-test fixture: a case-local header whose name holds a dollar.
 *
 * The compiler writes the name with the dollar escaped as "$$" in its
 * dependency list. The identity must decode the escape. See dollar_header.c.
 */
#define DOLLAR_VALUE 1
