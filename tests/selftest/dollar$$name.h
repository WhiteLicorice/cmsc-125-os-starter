/* SPDX-License-Identifier: MIT
 *
 * Self-test fixture: a decoy header whose doubled-dollar name matches the
 * escaped form of "dollar$name.h".
 *
 * The file is never included. It only proves that the identity hashes the file
 * the compiler read, not a file that happens to match the undecoded escape.
 */
#define DOLLAR_VALUE 99
