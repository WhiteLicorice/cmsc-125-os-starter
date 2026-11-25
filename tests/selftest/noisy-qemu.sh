#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
#
# A stand-in for QEMU that warns on standard error and then runs the real
# emulator. A line on that stream says nothing about the guest, so the run's own
# verdict must stand.
echo "qemu-system-riscv32: warning: a stand-in warning, then the real emulator" >&2
exec "${KTEST_BASE_QEMU:-qemu-system-riscv32}" "$@"
