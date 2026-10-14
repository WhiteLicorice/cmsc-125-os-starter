<!--no-pdf-->
# CMSC 125 operating system starter

This repository is the starting point for the CMSC 125 RV32 operating system.
A laboratory pair forks it once and adds one laboratory stage at a time. The
kernel boots, writes to the serial console, and does nothing else. The stages
add the processes, the memory allocator, the page tables, the system calls, the
disk, and the file system.

The test badge is red until the selected stage passes. That is the correct
state for a fresh fork.

## Contents

| Path | Purpose |
|:--|:--|
| `kernel.c` | The entry point, the SBI call, and the console writer |
| `common.c` | The small libc. Memory and string operations and `printf` |
| `assert.h` | The assertion macro and the build identity |
| `Makefile` | The build, run, and test targets |
| `kernel.ld` | The kernel memory layout |
| `tests/run-tests.sh` | The QEMU controller. It builds, boots, and judges |
| `tests/parse-log.awk` | The transcript parser. It decides the verdict |
| `tests/serial-client.py` | The console client for a case that reads input |
| `tests/manifest.tsv` | The case list, with the count, boots, fault, and input of each case |
| `tests/cases/` | The stage seed cases. One case per stage |
| `tests/selftest/` | The controller self-tests |
| `firmware/` | The vendored OpenSBI firmware and its licence |

## Requirements

The gate runs on Ubuntu 24.04. On Windows, install WSL2 with Ubuntu 24.04 and
run every command in WSL.

    sudo apt update
    sudo apt install -y clang llvm lld qemu-system-misc

On macOS, install Homebrew and the LLVM and QEMU formulae.

    brew install llvm lld qemu

The `-mac` targets set `CC` to Homebrew's clang, because Apple's `clang` has no
32-bit RISC-V target. Each target also puts Homebrew's LLVM `bin` folder and
Homebrew's `bin` folder ahead of `PATH`, so `ld.lld` and `llvm-objcopy` resolve.
Run a `-mac` target in a fresh shell with no other setup.

    make build-mac
    make run-mac
    make test-mac STAGE=1
    make test-harness-mac

`make build-mac` builds the kernel. `make run-mac` boots the kernel and prints
the banner. Stop QEMU with Ctrl-A, then X.

Run the lab commands from your fork. Give the stage as 1 to 4.

The controller uses Python 3 in two places: `tests/serial-client.py`, which
feeds a case that reads console input, and the persistence-reset self-test in
`make test-harness`. Install Python 3 before `make test-harness` or a case with
an input fixture. Ubuntu ships it. On macOS it comes with the Xcode Command
Line Tools, and Homebrew installs those tools with its first formula.

The validated versions are these.

| Tool | Version |
|:--|:--|
| Ubuntu | 24.04.3 LTS |
| clang | 18.1.3 |
| lld | 18.1.3 |
| llvm-objcopy | 18.1.3 |
| qemu-system-riscv32 | 8.2.2 |
| GNU Make | 4.3 |

The macOS runner reports these versions.

| Tool | Version |
|:--|:--|
| macOS | 26.6.2, build 25G83, arm64 |
| clang on `PATH` | Apple clang 21.0.0, no 32-bit RISC-V target |
| clang via `CC` | Homebrew clang 23.1.0 |
| lld | Homebrew LLD 23.1.0 |
| qemu-system-riscv32 | 11.1.1 |
| GNU Make | 3.81 |
| bash | 3.2.57 |
| Python | 3.14.7 |

Ubuntu 24.04 ships no 32-bit RISC-V OpenSBI image, so the firmware is
vendored. `firmware/opensbi-riscv32-generic-fw_dynamic.bin` is 123072 bytes.
Its SHA-256 is
`3de92c4ac5acb09a4e8678ce730491d9ff017c17669c67f4a9188f480c6fad42`.
`firmware/opensbi-LICENSE` holds the OpenSBI licence. The Makefile passes the
firmware to QEMU with `-bios`.

## Build and run

    make
    make run

`make` builds the kernel. `make run` builds the kernel and boots it under QEMU.
Press Ctrl-A, then X, to stop QEMU.

## Test

    make test STAGE=1
    make test-harness

`make test STAGE=N` runs every case of stages 1 to N. The command rejects a
missing stage and a stage outside 1 to 4. `make test-harness` runs the
controller self-tests.

`make test` builds the pristine test image from `disk/` into
`build/test-disk.tar`. Every case copies that file, so a case never writes the
interactive image that `make run` uses.

A test case is one C file under `tests/cases/`. The Makefile links the case
against the production kernel objects and defines `KTEST`. In test mode
`kernel_main` calls the case entry point. A case calls production functions
only. A case never contains a substitute scheduler, allocator, page table, or
file system.

The stage 1 to 4 cases fail on the starter, because the starter has no such
mechanism. The build error identifies the missing symbol. Each case passes when
you implement its stage.

## Record grammar

A test kernel prints the records on the serial console. Each record starts on a
new line. The controller accepts a record only at the start of a line, so case
console output never merges into a record.

    KTEST BUILD id=<hash>
    KTEST BEGIN name=<case>
    KTEST ASSERT ok at=<file>:<line> expr=<expression>
    KTEST ASSERT fail at=<file>:<line> expr=<expression>
    KTEST END name=<case> assertions=<n> failures=<n>
    KTEST IDLE

The expression is the rest of the line. The controller reads the structure of
the whole transcript. It requires exactly one build record, one begin record
with the case name, the assertions between the begin and the end record, and
exactly one end record. The end record must name the case, and its counts must
match the assertions and the count in `tests/manifest.tsv`. A malformed,
duplicate, or out-of-order record fails the case. A record after a panic fails
the case. A panic ends the transcript. A positive case passes only with no
failed assertion.

The build identity is a hash of the compiler dependency list, the linker script
`kernel.ld`, the selected case path, and the compiler flags. The dependency list
holds every file the compiler reads, including a header beside the case and a
header whose name holds a space. The Makefile embeds the identity in the test
kernel and the controller recomputes it from the same inputs, so a stale image
never passes. The controller rebuilds the test kernel for every boot. A
dependency scan that fails stops the case with `fail reason=build-identity`, and
the controller never compares an empty or partial identity.

The controller checks the structure of the transcript. It cannot prove what the
kernel computed. A kernel can print a fabricated transcript.

A negative case declares an expected fault in the manifest as a panic pattern.
The case passes only when that fault appears after the begin record, alone, and
the case prints no failed assertion. The controller recognizes a panic wherever
the complete prefix `PANIC: <file>:<line>: <message>` starts in a line. The
kernel prints a panic without a leading newline. Bare `PANIC: ` text with no
file and line fields is ordinary output. Two panics, on one line or on separate
lines, fail with `multiple-panics`. An unrelated panic fails with its own
reason.

A case that reads console input names a fixture in the manifest, or on the
controller command line with `--input PATH`. The controller opens a socket
console and sends the fixture bytes after the begin record. A missing or empty
fixture sends nothing, so the case meets the deadline.

## End state

The test kernel stops in a documented idle loop. The starter has no process, so
`ktest_idle` prints `KTEST IDLE` and idles.

In a kernel with processes, the last process exits, `yield` returns to the idle
process, and `kernel_main` panics with `switched to idle process`. The
controller accepts that panic only when the whole message equals `switched to
idle process`, the panic comes after a valid completion record, and it is the
only panic. A suffix, a prefix, a second panic, or an earlier panic fails the
case.

A valid completion record before the twenty-second deadline separates the
documented end state from a hang. The controller stops QEMU when it reads the
completion record and the end state.

## Stage map

| Stage | Laboratory | Book chapters | Mechanism |
|:--|:--|:--|:--|
| starter | Lab 0 | 4 to 7 | Boot, SBI console, libc, panic |
| 1 | Lab 1 | 8 to 10 | Exceptions, page allocation, processes |
| 2 | Lab 2 | 11 | Page tables |
| 3 | Lab 3 | 12 to 14 | Applications, user mode, system calls |
| 4 | Lab 4 | 15 and 16 | Disk and files |

## Diagnostics

The controller writes one serial log, one QEMU log, and one build log for each
boot under `build/logs/`. The files are `<case>.boot1.serial.log`,
`<case>.boot1.qemu.log`, and `<case>.boot1.build.log`. The controller builds
before every run. It measures the deadline with a fractional clock when the host
`date` gives fractional seconds, and falls back to whole seconds on another
host. The log scan and the process scheduler add to the measured time, so the
deadline bounds a run rather than cutting it at an exact moment. It stops every
QEMU process it starts and removes its temporary files on every exit path,
including Ctrl-C.

## Continuous integration

`.github/workflows/test.yml` runs one job on Ubuntu 24.04 and one job on macOS,
with `fail-fast: false`. The Ubuntu job installs the toolchain with apt and runs
`make test STAGE=$STAGE`. The macOS job installs the toolchain with Homebrew and
runs `make test-mac STAGE=$STAGE`. Both jobs are red until the selected stage
passes. That is the correct state for a fresh fork. The workflow builds from the
pushed commit.

## Licence

The operating-system code derives from "Operating System in 1,000 Lines" by
Seiya Nuta, at commit `012efb92b40be6a8a37ae3e1c2a0a15234315bfe`. The licence
is MIT. See `LICENSE`. The vendored OpenSBI firmware keeps its own BSD-2-Clause
licence in `firmware/opensbi-LICENSE`.
