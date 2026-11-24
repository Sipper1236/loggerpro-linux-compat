#!/usr/bin/env python3
"""Exercise gadget-mode protocol I/O over a PTY without kernel gadget privileges."""

import os
import pty
import select
import signal
import subprocess
import sys
import time
import tty
from pathlib import Path


def read_exact(fd: int, size: int, timeout: float = 2.0) -> bytes:
    deadline = time.monotonic() + timeout
    data = bytearray()
    while len(data) < size and time.monotonic() < deadline:
        ready, _, _ = select.select([fd], [], [], max(0.0, deadline - time.monotonic()))
        if ready:
            try:
                data.extend(os.read(fd, size - len(data)))
            except OSError:
                pass
    if len(data) != size:
        raise AssertionError(f"expected {size} response bytes, got {data.hex()}")
    return bytes(data)


def send_report(fd: int, report: bytes) -> None:
    if len(report) != 8 or os.write(fd, report) != 8:
        raise AssertionError("failed to write one 8-byte HID report")


def main() -> int:
    binary = Path(__file__).resolve().parent / "build" / "virtual_golink_uhid"
    master, slave = pty.openpty()
    tty.setraw(slave)
    process = subprocess.Popen(
        [str(binary), "--gadget", os.ttyname(slave)],
        stdout=subprocess.DEVNULL,
        stderr=subprocess.PIPE,
        text=True,
    )
    os.close(slave)
    try:
        time.sleep(0.08)
        send_report(master, bytes([0x28]) + bytes(7))
        assert read_exact(master, 8) == bytes([0x5F, 0x28, 2, 0, 0, 0, 0, 0])

        send_report(master, bytes([0x1B, 250, 0, 0, 0, 0, 0, 0]))
        assert read_exact(master, 8)[:2] == bytes([0x5A, 0x1B])
        send_report(master, bytes([0x1C]) + bytes(7))
        assert read_exact(master, 8)[:6] == bytes([0x5D, 0x1C, 250, 0, 0, 0])

        send_report(master, bytes([0x18]) + bytes(7))
        assert read_exact(master, 8)[:2] == bytes([0x5A, 0x18])
        measurement = read_exact(master, 8, timeout=1.0)
        assert measurement[0] == 1 and measurement[1] == 0
        send_report(master, bytes([0x19]) + bytes(7))
        assert read_exact(master, 8)[:2] == bytes([0x5A, 0x19])

        process.send_signal(signal.SIGINT)
        if process.wait(timeout=2) != 0:
            raise AssertionError("gadget-mode service did not exit cleanly")
        print(
            "Gadget-mode protocol I/O passed over PTY: sensor ID, period set/get, "
            "start, synthetic measurement, stop, and clean shutdown. "
            "This does not verify kernel USB enumeration."
        )
        return 0
    except (AssertionError, OSError, subprocess.TimeoutExpired) as error:
        print(f"gadget-mode PTY test failed: {error}", file=sys.stderr)
        if process.poll() is None:
            process.kill()
            process.wait()
        return 1
    finally:
        os.close(master)
        if process.poll() is None:
            process.kill()
            process.wait()
        stderr = process.stderr.read() if process.stderr else ""
        if stderr:
            print(stderr, file=sys.stderr, end="")
        if process.stderr:
            process.stderr.close()


if __name__ == "__main__":
    raise SystemExit(main())
