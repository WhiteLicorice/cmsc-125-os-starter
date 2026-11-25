#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
#
# A stand-in for QEMU that fails before the guest runs. The controller must
# report that failure as its own reason and show this message to the student.
echo "qemu-system-riscv32: -drive: could not open the image: Operation not supported" >&2
exit 1
