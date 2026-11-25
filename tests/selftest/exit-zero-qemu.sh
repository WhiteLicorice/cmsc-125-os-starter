#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
#
# A stand-in for QEMU that leaves with status 0 before the guest runs. QEMU did
# not fail to start, so the controller must report the guest's own reason.
exit 0
