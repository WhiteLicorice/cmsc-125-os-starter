#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
#
# The QEMU test controller.
#
# It builds one test kernel per boot, boots it under QEMU, reads the
# machine-readable records from the serial log, and prints a verdict. The
# record grammar, the idle contract, and the diagnostic locations are
# documented in README.md.
#
# Usage:
#   tests/run-tests.sh --stage N [--no-build]
#   tests/run-tests.sh --case NAME --source PATH --assertions M
#                      [--boots B] [--fault PATTERN] [--input PATH] [--no-build]
#
# The controller builds first, every time, unless --no-build is given. The
# self-test that detects a stale image uses --no-build.
#
# The verdict comes from tests/parse-log.awk, which reads the structure of the
# whole transcript. The controller only decides when to stop QEMU.

set -u

here="$(cd "$(dirname "$0")" && pwd)"
root="$(cd "$here/.." && pwd)"
build="$root/build"
logs="$build/logs"
tmp="$build/tmp"
# A Unix socket path is limited to 108 bytes, so the console socket lives in a
# short temporary directory, not under build/.
sock_dir="$(mktemp -d "${TMPDIR:-/tmp}/ktest.XXXXXX")"

QEMU="${QEMU:-qemu-system-riscv32}"
# Ubuntu 24.04's QEMU ships no riscv32 OpenSBI image, so the firmware is
# vendored and named explicitly.
BIOS="$root/firmware/opensbi-riscv32-generic-fw_dynamic.bin"
# The public deadline is twenty seconds of elapsed time.
DEADLINE="${KTEST_DEADLINE:-20}"
POLL="${KTEST_POLL:-0.1}"
# After a panic, wait this long for more output before the boot is over.
SETTLE="${KTEST_SETTLE:-1}"

# now_s prints the current time in seconds, with a fraction when the host date
# supports it. The deadline and the settle window compare these values in awk.
now_s() {
    local t
    t="$(date +%s.%N 2>/dev/null)"
    case "$t" in
        ''|*[!0-9.]*|*.*.*) date +%s ;;
        *.*) printf '%s' "$t" ;;
        *) date +%s ;;
    esac
}

# elapsed NOW START LIMIT is true when NOW minus START reaches LIMIT seconds.
elapsed() {
    awk -v now="$1" -v start="$2" -v limit="$3" \
        'BEGIN { exit !(now - start >= limit) }'
}

stage=""
case_name=""
case_src=""
case_asserts=""
case_boots="1"
case_fault=""
case_input=""
do_build=1
total=0
failed=0
qemu_pids=""
client_pids=""

usage() {
    echo "usage: tests/run-tests.sh --stage N [--no-build]" >&2
    echo "       tests/run-tests.sh --case NAME --source PATH --assertions M" >&2
    echo "                              [--boots B] [--fault PATTERN] [--input PATH]" >&2
    echo "                              [--no-build]" >&2
}

cleanup() {
    local status=$?
    local p
    for p in $client_pids; do
        kill "$p" 2>/dev/null || true
        wait "$p" 2>/dev/null || true
    done
    client_pids=""
    for p in $qemu_pids; do
        kill "$p" 2>/dev/null || true
        wait "$p" 2>/dev/null || true
    done
    qemu_pids=""
    rm -rf "$tmp"
    rm -rf "$sock_dir"
    exit "$status"
}

trap cleanup EXIT
trap 'exit 130' INT
trap 'exit 143' TERM

print_verdict() {
    echo "KTEST VERDICT case=$1 result=$2 reason=$3"
}

# classify SERIAL_LOG NAME WANT_ASSERTIONS FAULT_PATTERN WANT_BUILD_ID
# Prints "ok" or one failure reason.
classify() {
    local slog=$1 name=$2 want=$3 fault=$4 want_id=$5
    awk -v want_case="$name" -v want_id="$want_id" -v want="$want" -v fault="$fault" \
        -f "$here/parse-log.awk" < <(tr -d '\r' < "$slog")
}

# run_boot NAME BOOT IMAGE INPUT
run_boot() {
    local name=$1 boot=$2 image=$3 input=$4
    local slog="$logs/$name.boot$boot.serial.log"
    local qlog="$logs/$name.boot$boot.qemu.log"
    local pid="" client="" sock="" p rest size last_size start t_now last_change

    : > "$slog"

    local -a args=( -machine virt -bios "$BIOS" -display none -monitor none
                    --no-reboot -d guest_errors -D "$qlog" )
    if [ -n "$image" ]; then
        args+=( -drive "id=drive0,file=$image,format=raw,if=none"
                -device "virtio-blk-device,drive=drive0,bus=virtio-mmio-bus.0" )
    fi
    args+=( -kernel "$build/ktest/$name.boot$boot.elf" )

    if [ -n "$input" ] && [ "$input" != "-" ]; then
        # A socket chardev carries both directions. The client writes the
        # guest output to the serial log and sends the input fixture.
        mkdir -p "$tmp"
        sock="$sock_dir/$name.boot$boot.sock"
        rm -f "$sock"
        args+=( -chardev "socket,id=console,path=$sock,server=on,wait=on"
                -serial chardev:console )
    else
        args+=( -serial "file:$slog" )
    fi

    "$QEMU" "${args[@]}" >/dev/null 2>&1 &
    pid=$!
    qemu_pids="$qemu_pids $pid"

    if [ -n "$sock" ]; then
        local ipath="$input"
        case "$ipath" in
            /*) ;;
            *) ipath="$root/$input" ;;
        esac
        python3 "$here/serial-client.py" --socket "$sock" --log "$slog" \
            --input "$ipath" --marker "KTEST BEGIN" >/dev/null 2>&1 &
        client=$!
        client_pids="$client_pids $client"
    fi

    # Wait for a terminal record, a settled panic, or the deadline.
    start="$(now_s)"
    last_change="$start"
    last_size=-1
    while :; do
        kill -0 "$pid" 2>/dev/null || break
        t_now="$(now_s)"
        if elapsed "$t_now" "$start" "$DEADLINE"; then break; fi
        if grep -q '^KTEST IDLE' "$slog" 2>/dev/null; then break; fi
        size="$(wc -c < "$slog" 2>/dev/null || printf '0')"
        size="${size// /}"
        if [ "$size" != "$last_size" ]; then last_size="$size"; last_change="$t_now"; fi
        # A panic prefix, the same rule as tests/parse-log.awk: "PANIC: " then
        # a file field then ":<digits>: ". This only detects presence, so .*
        # stands in for the file field, which may hold a colon.
        if grep -qE 'PANIC: .*:[0-9]+: ' "$slog" 2>/dev/null; then
            if elapsed "$t_now" "$last_change" "$SETTLE"; then break; fi
        fi
        sleep "$POLL"
    done

    kill "$pid" 2>/dev/null || true
    wait "$pid" 2>/dev/null || true
    rest=""
    for p in $qemu_pids; do
        [ "$p" = "$pid" ] || rest="$rest $p"
    done
    qemu_pids="$rest"

    if [ -n "$client" ]; then
        kill "$client" 2>/dev/null || true
        wait "$client" 2>/dev/null || true
        rest=""
        for p in $client_pids; do
            [ "$p" = "$client" ] || rest="$rest $p"
        done
        client_pids="$rest"
    fi
}

# run_case NAME SOURCE WANT_ASSERTIONS BOOTS FAULT INPUT
run_case() {
    local name=$1 src=$2 want=$3 boots=$4 fault=$5 input=$6
    local image="" want_id="" flags="" cc="" result="pass" reason="ok" b r

    total=$((total + 1))

    # A fresh copy of the pristine test image, shared by every boot of a
    # persistence case. The interactive image is a different file.
    if [ -f "$root/build/test-disk.tar" ]; then
        mkdir -p "$tmp"
        image="$tmp/$name.tar"
        cp "$root/build/test-disk.tar" "$image"
    fi

    b=1
    while [ "$b" -le "$boots" ]; do
        # Resolve the compiler and the flags, then compute the identity. A
        # failed or empty identity stops the case before any build or boot, so
        # the controller never compares a partial identity.
        if ! flags="$(make -C "$root" -s --no-print-directory print-test-flags \
                BOOT="$b" 2> "$logs/$name.boot$b.identity.log")"; then
            result="fail"
            reason="build-identity"
            echo "case $name: boot $b build-identity"
            sed 's/^/    /' "$logs/$name.boot$b.identity.log"
            b=$((b + 1))
            continue
        fi
        cc="$(make -C "$root" -s --no-print-directory print-test-cc 2>/dev/null)"
        # The scan appends its diagnostic to the flags query's identity log, so
        # the log holds the reason the identity is empty.
        want_id="$(cd "$root" && sh tests/build-id.sh "$src" "$flags" "$cc" \
            2>> "$logs/$name.boot$b.identity.log")" || want_id=""
        if [ -z "$want_id" ]; then
            result="fail"
            reason="build-identity"
            echo "case $name: boot $b build-identity"
            sed 's/^/    /' "$logs/$name.boot$b.identity.log"
            b=$((b + 1))
            continue
        fi

        if [ "$do_build" -eq 1 ]; then
            if ! make -C "$root" -s --no-print-directory \
                    "build/ktest/$name.boot$b.elf" \
                    CASE="$name" SRC="$src" BOOT="$b" \
                    > "$logs/$name.boot$b.build.log" 2>&1; then
                result="fail"
                reason="build-failed"
                echo "case $name: boot $b build-failed"
                tail -n 20 "$logs/$name.boot$b.build.log" | sed 's/^/    /'
                b=$((b + 1))
                continue
            fi
        fi

        run_boot "$name" "$b" "$image" "$input"
        r="$(classify "$logs/$name.boot$b.serial.log" "$name" "$want" "$fault" "$want_id")"
        if [ "$r" != "ok" ]; then
            result="fail"
            reason="$r"
            echo "case $name: boot $b failed: $r"
        fi
        b=$((b + 1))
    done

    print_verdict "$name" "$result" "$reason"
    if [ "$result" = "fail" ]; then
        failed=$((failed + 1))
    fi
    return 0
}

while [ $# -gt 0 ]; do
    case "$1" in
        --stage)
            stage="${2:-}"
            shift
            if [ $# -gt 0 ]; then shift; fi
            ;;
        --case)
            case_name="${2:-}"
            shift
            if [ $# -gt 0 ]; then shift; fi
            ;;
        --source)
            case_src="${2:-}"
            shift
            if [ $# -gt 0 ]; then shift; fi
            ;;
        --assertions)
            case_asserts="${2:-}"
            shift
            if [ $# -gt 0 ]; then shift; fi
            ;;
        --boots)
            case_boots="${2:-}"
            shift
            if [ $# -gt 0 ]; then shift; fi
            ;;
        --fault)
            case_fault="${2:-}"
            shift
            if [ $# -gt 0 ]; then shift; fi
            ;;
        --input)
            case_input="${2:-}"
            shift
            if [ $# -gt 0 ]; then shift; fi
            ;;
        --no-build)
            do_build=0
            shift
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        *)
            echo "error: unknown argument: $1" >&2
            usage
            exit 2
            ;;
    esac
done

mkdir -p "$logs"

if [ -f "$root/shell.c" ]; then
    if ! make -C "$root" -s --no-print-directory build/test-disk.tar >/dev/null 2>&1; then
        echo "error: cannot build the test disk image in $root" >&2
        exit 2
    fi
fi

if [ -n "$stage" ]; then
    case "$stage" in
        1|2|3|4) ;;
        *)
            echo "error: STAGE must be 1, 2, 3, or 4 (got '$stage')" >&2
            exit 2
            ;;
    esac
    echo "stage: $stage"
    while IFS="$(printf '\t')" read -r name st src want boots fault input; do
        [ -n "${name:-}" ] || continue
        case "$name" in \#*) continue ;; esac
        if [ "$st" -le "$stage" ]; then
            run_case "$name" "$src" "$want" "${boots:-1}" "${fault:--}" "${input:--}"
        fi
    done < "$here/manifest.tsv"
elif [ -n "$case_name" ]; then
    if [ -z "$case_src" ] || [ -z "$case_asserts" ]; then
        echo "error: --case also needs --source and --assertions" >&2
        exit 2
    fi
    run_case "$case_name" "$case_src" "$case_asserts" "${case_boots:-1}" "${case_fault:--}" "${case_input:--}"
else
    usage
    exit 2
fi

if [ "$failed" -eq 0 ]; then
    echo "result: pass ($total cases)"
    exit 0
fi
echo "result: fail ($failed of $total cases)"
exit 1
