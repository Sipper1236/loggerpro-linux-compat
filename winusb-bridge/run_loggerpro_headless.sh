#!/usr/bin/env bash
set -euo pipefail

source_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
build_dir="$source_dir/build"

if [[ $# -ne 1 || -z "${WINEPREFIX:-}" ]]; then
    printf 'Usage: WINEPREFIX=/path/to/prefix %s /path/to/LoggerPro.exe\n' "$0" >&2
    exit 2
fi
loggerpro_exe=$(realpath -- "$1")
if [[ ! -f "$loggerpro_exe" ]]; then
    printf 'Logger Pro executable not found: %s\n' "$loggerpro_exe" >&2
    exit 2
fi
if ! command -v xvfb-run >/dev/null || ! command -v xauth >/dev/null; then
    printf 'This headless runner needs xvfb-run and xauth.\n' >&2
    exit 2
fi

"$source_dir/build.sh" >/dev/null
export LD_LIBRARY_PATH="$build_dir/wineusb-backend${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
export LOGGERPRO_USB_ENUM_LOG=${LOGGERPRO_USB_ENUM_LOG:-"${TMPDIR:-/tmp}/loggerpro-usb-enumeration.log"}
export WINEDEBUG=${WINEDEBUG:--all}
wine_bin=${WINE:-wine}
registry_helper="$build_dir/virtual_hid_registry.exe"
registry_installed=0

cleanup() {
    if [[ "$registry_installed" == 1 ]]; then
        "$registry_helper" remove >/dev/null 2>&1 || true
    fi
}
trap cleanup EXIT

case "${LOGGERPRO_VIRTUAL_HID_REGISTRY:-1}" in
    1)
        "$registry_helper" install >/dev/null
        registry_installed=1
        ;;
    0)
        ;;
    *)
        printf 'LOGGERPRO_VIRTUAL_HID_REGISTRY must be 0 or 1.\n' >&2
        exit 2
        ;;
esac

xvfb-run -a -s '-screen 0 1920x1080x24 -nolisten tcp' \
    bash -c 'cd -- "$(dirname -- "$1")"; exec "$2" "$1"' \
    _ "$loggerpro_exe" "$wine_bin"
