/* SPDX-License-Identifier: MIT
 *
 * Self-test fixture: a decoy header whose doubled-dollar name matches the
 * escaped form of "dollar$name.h".
 *
 * This decoy is never included. The compiler reports the real header with an
 * escaped dollar. The identity must hash the real header path.
 */
#define DOLLAR_VALUE 99
