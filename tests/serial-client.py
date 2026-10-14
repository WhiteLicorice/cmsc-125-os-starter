#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
#
# Stream the guest console over a Unix socket, and send the case input.
#
# The controller starts QEMU with a socket chardev in server mode. This client
# connects, copies every byte the guest writes to the serial log, and sends the
# input fixture once the guest prints the case begin record. A missing or empty
# fixture sends nothing, so the case meets the deadline.
#
# Usage: tests/serial-client.py --socket PATH --log PATH [--input PATH]

import argparse
import socket
import sys
import time


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--socket", required=True)
    parser.add_argument("--log", required=True)
    parser.add_argument("--input", default="")
    parser.add_argument("--marker", default="KTEST BEGIN")
    parser.add_argument("--connect-timeout", type=float, default=10.0)
    parser.add_argument("--idle-timeout", type=float, default=120.0)
    args = parser.parse_args()

    sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    deadline = time.monotonic() + args.connect_timeout
    while True:
        try:
            sock.connect(args.socket)
            break
        except OSError:
            if time.monotonic() >= deadline:
                return 1
            time.sleep(0.05)

    payload = b""
    if args.input:
        try:
            with open(args.input, "rb") as handle:
                payload = handle.read()
        except OSError:
            payload = b""

    marker = args.marker.encode()
    seen = b""
    sent = False
    last = time.monotonic()
    sock.settimeout(1.0)
    with open(args.log, "wb") as log:
        while True:
            try:
                data = sock.recv(4096)
            except socket.timeout:
                data = None
            if data:
                log.write(data)
                log.flush()
                last = time.monotonic()
                seen = (seen + data)[-4096:]
                if not sent and marker in seen:
                    if payload:
                        sock.sendall(payload)
                    sent = True
            elif data == b"":
                break
            if time.monotonic() - last > args.idle_timeout:
                break
    sock.close()
    return 0


if __name__ == "__main__":
    sys.exit(main())
