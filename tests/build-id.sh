#!/bin/sh
# SPDX-License-Identifier: MIT
#
# Print the build identity: a SHA-256 hash of the build inputs, cut to twelve
# hexadecimal digits.
#
# Usage: tests/build-id.sh [CASE_SOURCE] [FLAGS] [CC]
#
# The identity covers the compiler dependency list, the linker script
# kernel.ld, the compiler flags, and the selected case path. The dependency
# list names every file the compiler reads, including a header beside the
# selected case and a header whose name holds a space. The Makefile embeds the
# result in the test kernel. The controller recomputes the identity from the
# same inputs, so a stale image never passes.
#
# A failed dependency scan, or a named dependency that cannot be read, is an
# identity failure. The caller must stop. It must never compare an empty or
# partial identity.
#
# CC is a command and its arguments as plain words, for example
# "clang -Qunused-arguments". The command is word-split and globbing is off.
# Quote characters inside CC are not interpreted.
#
# The starter and the reference OS share this file byte for byte. The reference
# OS is instructor material. A student does not have it.

set -eu

cd "$(dirname "$0")/.."

case_src="${1:-}"
flags="${2:-}"
cc="${3:-${CC:-clang}}"

# The sources the test kernel compiles: the kernel sources, the record writer,
# and the selected case.
required="kernel.c common.c tests/ktest.c"
srcs="$required"
if [ -n "$case_src" ]; then
    srcs="$srcs $case_src"
fi

# The compiler writes one rule per source and lists every header the source
# reads. The Makefile and the controller pass the same compiler and flags, so
# both sides see one dependency list.
#
# The scan writes its rules to files this script owns. The appended -MF comes
# after the caller's flags, so it overrides an -MF the caller put in CC. One
# invocation per source is required, because clang keeps only the last input of
# a multi-source -MF file.
#
# Run the compiler the way make does: an unquoted command, word-split, with
# globbing off so a character in the value is not expanded. Keep the
# diagnostic for the failure path, so a successful scan stays quiet.
depfile="$(mktemp "${TMPDIR:-/tmp}/build-id.deps.XXXXXX")"
dep_one="$(mktemp "${TMPDIR:-/tmp}/build-id.one.XXXXXX")"
dep_err="$(mktemp "${TMPDIR:-/tmp}/build-id.err.XXXXXX")"
trap 'rm -f "$depfile" "$dep_one" "$dep_err"' EXIT
: > "$depfile"
for src in $srcs; do
    : > "$dep_one"
    # shellcheck disable=SC2086
    if ! ( set -f; $cc $flags -I. -M -MF "$dep_one" "$src" 2>"$dep_err" ); then
        cat "$dep_err" >&2
        echo "error: the compiler cannot list the build dependencies (CC='$cc')" >&2
        exit 1
    fi
    cat "$dep_one" >> "$depfile"
done
deps="$(cat "$depfile")"

# Parse make rules. A rule has a target and one or more dependencies. It may
# continue on the next line after a trailing backslash. The compiler escapes a
# space as "\ " and a dollar as "$$". Decode the dollar escape before the
# readability check and the hash. Drop the rule target. Keep each dependency
# path whole. Sort the paths for a stable order. The compiler omits the linker
# script from its dependency list. Add `kernel.ld` explicitly.
dep_files="$(
    {
        printf '%s\n' "$deps" | awk '
            function put(name) { if (name !~ /:$/) print name }
            function emit(s,   out, n, i, c) {
                out = ""
                n = length(s)
                i = 1
                while (i <= n) {
                    c = substr(s, i, 1)
                    if (c == "\\") {
                        i++
                        if (i <= n) out = out substr(s, i, 1)
                        i++
                    } else if (c == "$") {
                        out = out "$"
                        if (substr(s, i + 1, 1) == "$") i += 2
                        else i++
                    } else if (c == " " || c == "\t") {
                        if (out != "") { put(out); out = "" }
                        i++
                    } else {
                        out = out c
                        i++
                    }
                }
                if (out != "") put(out)
            }
            {
                line = line " " $0
                if (line ~ /\\$/) { sub(/\\$/, "", line); next }
                emit(line)
                line = ""
            }
            END { if (line != "") emit(line) }
        '
        printf 'kernel.ld\n'
    } | LC_ALL=C sort -u
)"

# Every dependency in the list must be readable. The caller receives an
# identity only after the scan reads the full list.
while IFS= read -r f; do
    [ -n "$f" ] || continue
    if [ ! -r "$f" ]; then
        echo "error: cannot read the build dependency '$f'" >&2
        exit 1
    fi
done <<EOF
$dep_files
EOF

# The list must include every primary source as the compiler spells it. An
# empty or partial list means the identity is incomplete. A missing entry means
# the scan did not reach that source, so the hash would miss a kernel input.
if [ -z "$dep_files" ]; then
    echo "error: the compiler listed no build dependencies (CC='$cc')" >&2
    exit 1
fi
for want in $required; do
    if ! printf '%s\n' "$dep_files" | grep -Fxq -e "$want"; then
        echo "error: the compiler dependency list does not name '$want'" >&2
        exit 1
    fi
done
if [ -n "$case_src" ]; then
    if ! printf '%s\n' "$dep_files" | grep -Fxq -e "$case_src"; then
        echo "error: the compiler dependency list does not name the case '$case_src'" >&2
        exit 1
    fi
fi

hash_input() {
    printf 'flags: %s\n' "$flags"
    printf 'case: %s\n' "$case_src"
    while IFS= read -r f; do
        [ -n "$f" ] || continue
        printf 'file: %s\n' "$f"
        cat "$f"
    done <<EOF
$dep_files
EOF
}

if command -v sha256sum >/dev/null 2>&1; then
    hash_input | sha256sum | cut -c1-12
elif command -v shasum >/dev/null 2>&1; then
    hash_input | shasum -a 256 | cut -c1-12
else
    echo "error: no sha256 tool found" >&2
    exit 1
fi
