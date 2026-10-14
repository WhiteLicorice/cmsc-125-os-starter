#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
#
# Self-test wrapper around QEMU. It restores the pristine test image before
# every boot, so a persistable write cannot survive a reboot. The persistence
# self-test must fail under this wrapper.
#
# The harness copies this file to a temporary path and makes it executable. It
# sets KTEST_RESET_IMAGE to the pristine image.
#
# The starter and the reference OS share this file byte for byte.

import os
import shutil
import sys


def main() -> int:
    args = sys.argv[1:]
    pristine = os.environ.get("KTEST_RESET_IMAGE", "")
    if pristine:
        for i, arg in enumerate(args):
            if arg != "-drive":
                continue
            for field in args[i + 1].split(","):
                if field.startswith("file="):
                    shutil.copyfile(pristine, field[len("file="):])
                    break
    os.execvp("qemu-system-riscv32", ["qemu-system-riscv32"] + args)
    return 0


if __name__ == "__main__":
    sys.exit(main())
