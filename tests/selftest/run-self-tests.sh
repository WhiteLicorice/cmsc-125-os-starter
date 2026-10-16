#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
#
# The controller self-tests.
#
# Each self-test feeds the controller a deliberately bad case and checks the
# verdict. The persistence and negative-fault self-tests need the file system
# and the allocator, so the starter skips them. The starter has no shell.c.
#
# The starter and the reference OS share this file byte for byte. The reference
# OS is instructor material, and a student does not have it.
#
# Run with: make test-harness

set -u

here="$(cd "$(dirname "$0")" && pwd)"
root="$(cd "$here/../.." && pwd)"
ctrl="$root/tests/run-tests.sh"

# The compiler the caller selected, for the checks that pass a CC command with
# an argument. A bare "clang" is the system compiler, and the system compiler on
# macOS carries no RISC-V target, so the checks must start from the caller's CC.
base_cc="${CC:-clang}"
# The fixture compilers delegate their real compile to this compiler.
export KTEST_BASE_CC="$base_cc"

# A short deadline keeps the deliberate hangs quick.
export KTEST_DEADLINE="${KTEST_DEADLINE:-5}"
export KTEST_POLL="${KTEST_POLL:-0.1}"

# The backups live outside build/, because the controller removes build/tmp on
# every exit path.
harness_tmp="$(mktemp -d)"
kernel_src="$root/kernel.c"
kernel_bak="$harness_tmp/kernel.c"
stale_src="$root/tests/selftest/stale_case.c"
stale_bak="$harness_tmp/stale_case.c"
header_src="$root/tests/selftest/header_case.h"
header_bak="$harness_tmp/header_case.h"
linker_src="$root/kernel.ld"
linker_bak="$harness_tmp/kernel.ld"
space_header_src="$root/tests/selftest/header with spaces.h"
space_header_bak="$harness_tmp/header with spaces.h"
probe_src="$root/tests/selftest/stale_probe.c"
probe_bak="$harness_tmp/stale_probe.c"
dollar_hdr="$root/tests/selftest/dollar\$name.h"
dollar_bak="$harness_tmp/dollar_hdr"

kernel_mutated=0
stale_mutated=0
header_mutated=0
linker_mutated=0
space_header_mutated=0
probe_mutated=0
dollar_mutated=0
ran=0
failures=0

restore() {
    if [ "$kernel_mutated" -eq 1 ] && [ -f "$kernel_bak" ]; then
        cp "$kernel_bak" "$kernel_src"
        kernel_mutated=0
    fi
    if [ "$stale_mutated" -eq 1 ] && [ -f "$stale_bak" ]; then
        cp "$stale_bak" "$stale_src"
        stale_mutated=0
    fi
    if [ "$header_mutated" -eq 1 ] && [ -f "$header_bak" ]; then
        cp "$header_bak" "$header_src"
        header_mutated=0
    fi
    if [ "$linker_mutated" -eq 1 ] && [ -f "$linker_bak" ]; then
        cp "$linker_bak" "$linker_src"
        linker_mutated=0
    fi
    if [ "$space_header_mutated" -eq 1 ] && [ -f "$space_header_bak" ]; then
        cp "$space_header_bak" "$space_header_src"
        space_header_mutated=0
    fi
    if [ "$probe_mutated" -eq 1 ] && [ -f "$probe_bak" ]; then
        cp "$probe_bak" "$probe_src"
        probe_mutated=0
    fi
    if [ "$dollar_mutated" -eq 1 ] && [ -f "$dollar_bak" ]; then
        cp "$dollar_bak" "$dollar_hdr"
        dollar_mutated=0
    fi
    rm -rf "$harness_tmp"
    rm -rf "$root/probe-build"
}
trap restore EXIT
trap 'exit 130' INT
trap 'exit 143' TERM

# verdict_line prints the verdict from the controller output on stdin.
verdict_line() {
    grep -o 'KTEST VERDICT case=[^ ]* result=[^ ]* reason=[^ ]*' | tail -n1
}

verdict_of() {
    local out
    out="$(bash "$ctrl" "$@" 2>&1)"
    printf '%s\n' "$out" | verdict_line
}

# verdict_cc CC ARGS... runs the controller with CC set for the controller and
# every process it starts.
#
# The CC assignment must reach the controller's own make calls, not only the
# controller. MAKEFLAGS and MAKEOVERRIDES are cleared for that reason. GNU make
# passes a command-line variable such as CC=... to every sub-make through
# MAKEFLAGS, and a MAKEFLAGS assignment outranks the environment. Without the
# clear, "make CC=... test-harness" would hand the outer CC to the controller
# and skip the fixture. The assignment itself precedes the bash command.
verdict_cc() {
    local cc=$1
    shift
    local out
    out="$(env -u MAKEFLAGS -u MAKEOVERRIDES -u MFLAGS CC="$cc" \
        bash "$ctrl" "$@" 2>&1)"
    printf '%s\n' "$out" | verdict_line
}

# verdict_qemu QEMU ARGS... runs the controller with QEMU set for the controller
# and every process it starts. It carries the same rules as verdict_cc.
verdict_qemu() {
    local qemu=$1
    shift
    local out
    out="$(env -u MAKEFLAGS -u MAKEOVERRIDES -u MFLAGS QEMU="$qemu" \
        KTEST_RESET_IMAGE="$root/build/test-disk.tar" \
        bash "$ctrl" "$@" 2>&1)"
    printf '%s\n' "$out" | verdict_line
}

# report LABEL CASE EXPECTED_VERDICT GOT
# LABEL names the check. CASE names the controller verdict it expects.
report() {
    local label=$1 case=$2 want=$3 got=$4
    if [ "$got" = "KTEST VERDICT case=$case result=$want" ]; then
        printf 'self-test %-24s %s\n' "$label" "$got"
    else
        printf 'self-test %-24s FAILED\n' "$label"
        printf '    expected KTEST VERDICT case=%s result=%s\n' "$case" "$want"
        printf '    got      %s\n' "${got:-<no verdict>}"
        failures=$((failures + 1))
    fi
}

# check LABEL EXPECTED_VERDICT CONTROLLER_ARGS...
check() {
    local label=$1 want=$2 got
    shift 2
    ran=$((ran + 1))
    got="$(verdict_of "$@")"
    report "$label" "$label" "$want" "$got"
}

echo "controller self-tests"
echo "deadline ${KTEST_DEADLINE}s"

check bad_hang "fail reason=hang" \
    --case bad_hang --source tests/selftest/bad_hang.c --assertions 0

check bad_no_completion "fail reason=no-completion" \
    --case bad_no_completion --source tests/selftest/bad_no_completion.c --assertions 1

check bad_wrong_count "fail reason=wrong-assertion-count" \
    --case bad_wrong_count --source tests/selftest/bad_wrong_count.c --assertions 2

check bad_unrelated_panic "fail reason=unrelated-panic" \
    --case bad_unrelated_panic --source tests/selftest/bad_unrelated_panic.c --assertions 1

# The transcript structure.
check bad_forged_end "fail reason=duplicate-record" \
    --case bad_forged_end --source tests/selftest/bad_forged_end.c --assertions 1

check bad_foreign_case "fail reason=wrong-case" \
    --case bad_foreign_case --source tests/selftest/bad_foreign_case.c --assertions 1

check bad_embedded_records "fail reason=out-of-order-record" \
    --case bad_embedded_records --source tests/selftest/bad_embedded_records.c --assertions 1

check bad_record_after_panic "fail reason=out-of-order-record" \
    --case bad_record_after_panic \
    --source tests/selftest/bad_record_after_panic.c --assertions 1 \
    --fault "declared probe fault"

# The idle contract and the negative-fault ordering.
check bad_idle_substring "fail reason=unrelated-panic" \
    --case bad_idle_substring --source tests/selftest/bad_idle_substring.c --assertions 1

check bad_idle_before_end "fail reason=out-of-order-record" \
    --case bad_idle_before_end --source tests/selftest/bad_idle_before_end.c --assertions 1

check bad_end_then_panic "fail reason=unrelated-panic" \
    --case bad_end_then_panic --source tests/selftest/bad_end_then_panic.c --assertions 1

check bad_hang_after_end "fail reason=no-idle" \
    --case bad_hang_after_end --source tests/selftest/bad_hang_after_end.c --assertions 1

check bad_fault_after_failed_assert "fail reason=assertion-failed" \
    --case bad_fault_after_failed_assert \
    --source tests/selftest/bad_fault_after_failed_assert.c --assertions 1 \
    --fault "declared probe fault"

# A declared fault after console text. The panic starts partway through the
# line, so the controller must still read its message.
check good_prefixed_fault "pass reason=ok" \
    --case good_prefixed_fault --source tests/selftest/good_prefixed_fault.c \
    --assertions 1 --fault "declared probe fault"

# The passing run that builds the image the stale check reuses.
check stale_probe "pass reason=ok" \
    --case stale_probe --source tests/selftest/stale_probe.c --assertions 1

# A stale build artifact: change a kernel source, run without rebuilding.
stale_check() {
    local got
    ran=$((ran + 1))
    cp "$kernel_src" "$kernel_bak"
    kernel_mutated=1
    printf '\n/* harness: stale probe */\n' >> "$kernel_src"
    got="$(verdict_of --case stale_probe --source tests/selftest/stale_probe.c \
        --assertions 1 --no-build)"
    cp "$kernel_bak" "$kernel_src"
    kernel_mutated=0
    report bad_stale_build stale_probe "fail reason=stale-build" "$got"
}
stale_check

# A stale case source: change the case, run the old image without rebuilding.
stale_case_check() {
    local got
    ran=$((ran + 1))
    cp "$stale_src" "$stale_bak"
    stale_mutated=1
    verdict_of --case stale_case --source tests/selftest/stale_case.c \
        --assertions 1 >/dev/null
    sed 's/KTEST_ASSERT(true)/KTEST_ASSERT(false)/' "$stale_bak" > "$stale_src"
    got="$(verdict_of --case stale_case --source tests/selftest/stale_case.c \
        --assertions 1 --no-build)"
    cp "$stale_bak" "$stale_src"
    stale_mutated=0
    report bad_stale_case stale_case "fail reason=stale-build" "$got"
}
stale_case_check

# A stale case-local header: change a header beside the case, run the old image
# without rebuilding. The identity covers every header the compiler reads.
header_check() {
    local got
    ran=$((ran + 1))
    cp "$header_src" "$header_bak"
    header_mutated=1
    verdict_of --case header_case --source tests/selftest/header_case.c \
        --assertions 1 >/dev/null
    sed 's/CASE_VALUE 1/CASE_VALUE 0/' "$header_bak" > "$header_src"
    got="$(verdict_of --case header_case --source tests/selftest/header_case.c \
        --assertions 1 --no-build)"
    cp "$header_bak" "$header_src"
    header_mutated=0
    report bad_stale_header header_case "fail reason=stale-build" "$got"
}
header_check

# A stale linker script: change kernel.ld, run the old image without rebuilding.
# The identity covers the linker script, not only the compiler dependencies.
linker_check() {
    local got
    ran=$((ran + 1))
    cp "$linker_src" "$linker_bak"
    linker_mutated=1
    verdict_of --case stale_probe --source tests/selftest/stale_probe.c \
        --assertions 1 >/dev/null
    printf '\n/* harness: stale linker */\n' >> "$linker_src"
    got="$(verdict_of --case stale_probe --source tests/selftest/stale_probe.c \
        --assertions 1 --no-build)"
    cp "$linker_bak" "$linker_src"
    linker_mutated=0
    report bad_stale_linker stale_probe "fail reason=stale-build" "$got"
}
linker_check

# The normal stale check, with no --no-build. Build a passing case, then select
# an older failing source under the same case name. The controller rebuilds, so
# the failing source runs and the case fails its assertion.
normal_stale_check() {
    local got fail_src="$harness_tmp/stale_probe_fail.c"
    ran=$((ran + 1))
    verdict_of --case stale_probe --source tests/selftest/stale_probe.c \
        --assertions 1 >/dev/null
    sed 's/KTEST_ASSERT(true)/KTEST_ASSERT(false)/' \
        "$root/tests/selftest/stale_probe.c" > "$fail_src"
    touch -t 200001010000 "$fail_src"
    got="$(verdict_of --case stale_probe --source "$fail_src" --assertions 1)"
    report bad_stale_normal stale_probe "fail reason=assertion-failed" "$got"
}
normal_stale_check

# A stale case-local header whose name holds a space. The compiler escapes the
# space in its dependency list, so the identity must parse the escaped name.
space_header_check() {
    local got
    ran=$((ran + 1))
    cp "$space_header_src" "$space_header_bak"
    space_header_mutated=1
    verdict_of --case space_header --source tests/selftest/space_header.c \
        --assertions 1 >/dev/null
    sed 's/SPACE_HEADER_VALUE 1/SPACE_HEADER_VALUE 0/' \
        "$space_header_bak" > "$space_header_src"
    got="$(verdict_of --case space_header --source tests/selftest/space_header.c \
        --assertions 1 --no-build)"
    cp "$space_header_bak" "$space_header_src"
    space_header_mutated=0
    report bad_stale_space_header space_header "fail reason=stale-build" "$got"
}
space_header_check

# A real -MF in CC. The compiler writes its rules to the caller's file, so the
# identity must own its output file. The identity stays correct on the build
# path and on the --no-build path.
mf_redirect_check() {
    local got sink="$harness_tmp/mf.d"
    cp "$probe_src" "$probe_bak"
    probe_mutated=1
    ran=$((ran + 1))
    got="$(verdict_cc "$base_cc -MF $sink" --case stale_probe \
        --source tests/selftest/stale_probe.c --assertions 1)"
    report mf_redirect_probe stale_probe "pass reason=ok" "$got"
    ran=$((ran + 1))
    sed 's/KTEST_ASSERT(true)/KTEST_ASSERT(false)/' "$probe_bak" > "$probe_src"
    got="$(verdict_cc "$base_cc -MF $sink" --case stale_probe \
        --source tests/selftest/stale_probe.c --assertions 1 --no-build)"
    report mf_redirect_stale stale_probe "fail reason=stale-build" "$got"
    ran=$((ran + 1))
    got="$(verdict_cc "$base_cc -MF $sink" --case stale_probe \
        --source tests/selftest/stale_probe.c --assertions 1)"
    report mf_redirect_normal stale_probe "fail reason=assertion-failed" "$got"
    cp "$probe_bak" "$probe_src"
    probe_mutated=0
}
mf_redirect_check

# A compiler that exits zero and lists no dependencies. The identity must fail
# on both the build path and the --no-build path. It must never accept an empty
# list.
empty_deps_check() {
    local got wrapper="$harness_tmp/empty-deps-clang"
    cp "$root/tests/selftest/empty-deps-clang" "$wrapper"
    chmod +x "$wrapper"
    ran=$((ran + 1))
    got="$(verdict_cc "$wrapper" --case stale_probe \
        --source tests/selftest/stale_probe.c --assertions 1)"
    report bad_empty_deps stale_probe "fail reason=build-identity" "$got"
    ran=$((ran + 1))
    got="$(verdict_cc "$wrapper" --case stale_probe \
        --source tests/selftest/stale_probe.c --assertions 1 --no-build)"
    report bad_empty_deps_nobuild stale_probe "fail reason=build-identity" "$got"
}
empty_deps_check

# A case-local header whose name holds a dollar. The compiler escapes the
# dollar as "$$". A readable decoy with the doubled name sits beside the
# real header. Only the real header changes, so the identity must detect it.
dollar_header_check() {
    local got
    cp "$dollar_hdr" "$dollar_bak"
    dollar_mutated=1
    ran=$((ran + 1))
    verdict_of --case dollar_header --source tests/selftest/dollar_header.c \
        --assertions 1 >/dev/null
    sed 's/DOLLAR_VALUE 1/DOLLAR_VALUE 0/' "$dollar_bak" > "$dollar_hdr"
    got="$(verdict_of --case dollar_header \
        --source tests/selftest/dollar_header.c --assertions 1 --no-build)"
    cp "$dollar_bak" "$dollar_hdr"
    dollar_mutated=0
    report bad_stale_dollar_header dollar_header "fail reason=stale-build" "$got"
}
dollar_header_check

# A BUILD override reaches the kernel recipe without a named goal. The kernel
# recipe, not a parse-time goal filter, must stop an empty identity.
guard_check() {
    local wrapper="$harness_tmp/no-deps-clang" out rc
    cp "$root/tests/selftest/no-deps-clang" "$wrapper"
    chmod +x "$wrapper"
    ran=$((ran + 1))
    out="$(make -C "$root" BUILD=probe-build probe-build/kernel.elf \
        CC="$wrapper" 2>&1)"
    rc=$?
    rm -rf "$root/probe-build"
    if [ "$rc" -ne 0 ] && printf '%s\n' "$out" | grep -q 'build identity is empty'; then
        printf 'self-test %-24s %s\n' bad_build_override "exit $rc, identity error printed"
    else
        printf 'self-test %-24s FAILED\n' bad_build_override
        printf '    expected a non-zero exit with the identity error\n'
        printf '    got      exit %s: %s\n' "$rc" "$out"
        failures=$((failures + 1))
    fi
}
guard_check

# Two panic events on one line. The printed prefix precedes the real panic on
# the same line, so the parser must count both.
check real_second_panic "fail reason=multiple-panics" \
    --case real_second_panic --source tests/selftest/real_second_panic.c \
    --assertions 1 --fault "declared probe fault"

# Two panic events on separate lines.
check bad_two_panics "fail reason=multiple-panics" \
    --case bad_two_panics --source tests/selftest/bad_two_panics.c \
    --assertions 1 --fault "declared probe fault"

# The bare PANIC token without the file and line fields is ordinary output.
check ordinary_panic "pass reason=ok" \
    --case ordinary_panic --source tests/selftest/ordinary_panic.c --assertions 1

# A real PANIC after a #line with a colon in the file name. The file field may
# hold a colon, so the controller must still find the declared fault and pass.
check colon_panic "pass reason=ok" \
    --case colon_panic --source tests/selftest/colon_panic.c \
    --assertions 1 --fault "declared probe fault"

# A compiler command that carries an argument. The identity must run it the way
# make does, so the build and the identity both succeed. A later assertion
# change with --no-build must still fail the identity.
cc_args_check() {
    local got
    cp "$probe_src" "$probe_bak"
    probe_mutated=1
    ran=$((ran + 1))
    got="$(verdict_cc "$base_cc -Qunused-arguments" --case stale_probe \
        --source tests/selftest/stale_probe.c --assertions 1)"
    report cc_args_probe stale_probe "pass reason=ok" "$got"
    ran=$((ran + 1))
    sed 's/KTEST_ASSERT(true)/KTEST_ASSERT(false)/' "$probe_bak" > "$probe_src"
    got="$(verdict_cc "$base_cc -Qunused-arguments" --case stale_probe \
        --source tests/selftest/stale_probe.c --assertions 1 --no-build)"
    cp "$probe_bak" "$probe_src"
    probe_mutated=0
    report cc_args_stale stale_probe "fail reason=stale-build" "$got"
}
cc_args_check

# A dependency scan that fails while the compilation succeeds. The controller
# must stop with the diagnostic and never compare a partial identity.
deps_scan_check() {
    local got wrapper="$harness_tmp/no-deps-clang"
    ran=$((ran + 1))
    cp "$root/tests/selftest/no-deps-clang" "$wrapper"
    chmod +x "$wrapper"
    got="$(verdict_cc "$wrapper" --case stale_probe \
        --source tests/selftest/stale_probe.c --assertions 1)"
    report bad_deps_scan stale_probe "fail reason=build-identity" "$got"
}
deps_scan_check

# This check runs a fixture with a compiler command that includes an argument. The
# fixture must split KTEST_BASE_CC the way the controller does: plain words
# with globbing off. Its own arguments kept apart. A quoted command would
# name one program, so the compile would never start.
fixture_cc_args_check() {
    local fixture="$harness_tmp/no-deps-clang" src="$harness_tmp/fixture-args.c"
    local obj="$harness_tmp/fixture-args.o" out rc
    cp "$root/tests/selftest/no-deps-clang" "$fixture"
    chmod +x "$fixture"
    printf 'int fixture_args_probe;\n' > "$src"
    ran=$((ran + 1))
    out="$(KTEST_BASE_CC="$base_cc -Qunused-arguments" \
        "$fixture" -c "$src" -o "$obj" 2>&1)"
    rc=$?
    if [ "$rc" -eq 0 ] && [ -f "$obj" ]; then
        printf 'self-test %-24s %s\n' fixture_cc_args "exit 0, compiled through KTEST_BASE_CC"
    else
        printf 'self-test %-24s FAILED\n' fixture_cc_args
        printf '    expected the fixture to compile through KTEST_BASE_CC\n'
        printf '    got      exit %s: %s\n' "$rc" "$out"
        failures=$((failures + 1))
    fi
}
fixture_cc_args_check

if [ -f "$root/shell.c" ]; then
    check good_negative "pass reason=ok" \
        --case good_negative --source tests/selftest/good_negative.c \
        --assertions 1 --fault "out of memory"

    # The reference idle panic follows console text on the same line.
    check idle_after_text "pass reason=ok" \
        --case idle_after_text --source tests/selftest/idle_after_text.c --assertions 1

    check good_persistence "pass reason=ok" \
        --case good_persistence --source tests/selftest/good_persistence.c \
        --assertions 2 --boots 2

    # Console input through the production path.
    check getchar_probe "pass reason=ok" \
        --case getchar_probe --source tests/selftest/getchar_probe.c \
        --assertions 1 --input tests/selftest/getchar_probe.input

    check bad_getchar "fail reason=no-completion" \
        --case bad_getchar --source tests/selftest/bad_getchar.c \
        --assertions 1 --input tests/selftest/missing.input

    # The disk does not survive the reboot, so the persistence case must fail.
    persistence_reset_check() {
        local got wrapper="$harness_tmp/reset-disk-qemu.py"
        ran=$((ran + 1))
        cp "$root/tests/selftest/reset-disk-qemu.py" "$wrapper"
        chmod +x "$wrapper"
        got="$(verdict_qemu "$wrapper" --case good_persistence \
            --source tests/selftest/good_persistence.c --assertions 2 --boots 2)"
        report bad_persistence_reset good_persistence "fail reason=assertion-failed" "$got"
    }
    persistence_reset_check
else
    echo "skip    good_negative      (needs the stage 1 allocator)"
    echo "skip    idle_after_text    (needs the reference idle panic)"
    echo "skip    good_persistence   (needs the stage 4 file system)"
    echo "skip    getchar_probe      (needs the stage 3 syscall)"
    echo "skip    bad_getchar        (needs the stage 3 syscall)"
    echo "skip    bad_persistence_reset (needs the stage 4 file system)"
fi

echo
if [ "$failures" -eq 0 ]; then
    echo "self-tests: pass ($ran checks)"
    exit 0
fi
echo "self-tests: fail ($failures of $ran checks)"
exit 1
