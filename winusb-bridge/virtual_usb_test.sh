#!/usr/bin/env bash
set -euo pipefail

source_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
build_dir="$source_dir/build"
log_file=$(mktemp)
bridge_pid=
test_prefix=${WINEPREFIX:-"$HOME/.wine"}
mkdir -p "$test_prefix"
if [[ ! -f "$test_prefix/system.reg" ]]; then
  WINEPREFIX="$test_prefix" wineboot -u >/dev/null
fi

cleanup() {
  if [[ -n "$bridge_pid" ]]; then
    kill "$bridge_pid" 2>/dev/null || true
    wait "$bridge_pid" 2>/dev/null || true
  fi
  rm -f "$log_file"
}
trap cleanup EXIT

"$source_dir/build.sh" >/dev/null
if python3 -c 'import socket; s=socket.create_connection(("127.0.0.1",47821),timeout=0.2); s.close()' 2>/dev/null; then
  printf 'Port 47821 is already in use; stop its process before running the virtual USB test.\n' >&2
  exit 2
fi

"$build_dir/bridge-virtual" >"$log_file" 2>&1 &
bridge_pid=$!
ready=0
for _ in {1..50}; do
  if python3 -c 'import socket; s=socket.create_connection(("127.0.0.1",47821),timeout=0.2); s.close()' 2>/dev/null; then
    ready=1
    break
  fi
  sleep 0.1
done
if [[ "$ready" != 1 ]]; then
  cat "$log_file" >&2
  printf 'Virtual USB bridge failed to start.\n' >&2
  exit 3
fi

if ! WINEPREFIX="$test_prefix" \
    WINEDLLPATH="$build_dir" WINEDLLOVERRIDES=winusb=n WINEDEBUG=-all \
    timeout 15s wine "$build_dir/probe_virtual_usb.exe"; then
  cat "$log_file" >&2
  printf 'WinUSB virtual-device probe failed.\n' >&2
  exit 4
fi

grep -q 'open usb vid=0000 pid=0000 iface=0 => 0' "$log_file"
grep -q 'W session=.*endpoint=01 length=4 actual=4 status=0' "$log_file"
grep -q 'R session=.*endpoint=81 length=4 actual=4 status=0' "$log_file"
grep -q 'R session=.*endpoint=82 length=4 actual=0 status=-10' "$log_file"
cat "$log_file"
printf 'virtual USB probe passed: device descriptor, bulk write/read, interrupt abort, and free.\n'
