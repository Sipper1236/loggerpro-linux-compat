#!/usr/bin/env bash
set -euo pipefail

gadget=/sys/kernel/config/usb_gadget/loggerpro_golink
function_name=hid.usb0
udc_name=dummy_udc.0
config_path="$gadget/configs/c.1"
function_path="$gadget/functions/$function_name"

remove_gadget() {
    if [[ ! -d "$gadget" ]]; then
        return
    fi
    if [[ -e "$gadget/UDC" ]]; then
        printf '\n' > "$gadget/UDC" || true
    fi
    if [[ -L "$config_path/$function_name" ]]; then
        unlink "$config_path/$function_name"
    fi
    for path in \
        "$function_path" \
        "$config_path/strings/0x409" \
        "$config_path" \
        "$gadget/strings/0x409" \
        "$gadget/strings" \
        "$gadget/configs" \
        "$gadget/functions"; do
        if [[ -d "$path" ]]; then
            rmdir "$path"
        fi
    done
    rmdir "$gadget"
}

cleanup_after_start_failure() {
    trap - ERR
    remove_gadget || true
}

if (( EUID != 0 )); then
    printf 'Run this script with sudo; it creates a temporary USB gadget.\n' >&2
    exit 1
fi

action=${1:-}
case "$action" in
    start)
        if [[ -e "$gadget" ]]; then
            printf 'The virtual Go!Link gadget already exists: %s\n' "$gadget" >&2
            exit 1
        fi
        if ! mountpoint -q /sys/kernel/config; then
            printf '/sys/kernel/config is not mounted.\n' >&2
            exit 1
        fi

        modprobe dummy_hcd
        modprobe usb_f_hid
        if [[ ! -e "/sys/class/udc/$udc_name" ]]; then
            printf 'USB gadget controller not found: %s\n' "$udc_name" >&2
            exit 1
        fi

        trap cleanup_after_start_failure ERR
        mkdir "$gadget"
        printf '0x08f7' > "$gadget/idVendor"
        printf '0x0003' > "$gadget/idProduct"
        printf '0x0100' > "$gadget/bcdDevice"
        printf '0x0200' > "$gadget/bcdUSB"
        mkdir "$gadget/strings/0x409"
        printf 'Vernier' > "$gadget/strings/0x409/manufacturer"
        printf 'Go!Link Protocol Test Emulator' > "$gadget/strings/0x409/product"
        printf 'loggerpro-golink-gadget-test' > "$gadget/strings/0x409/serialnumber"

        mkdir "$config_path"
        mkdir "$config_path/strings/0x409"
        printf 'Go!Link HID test interface' > "$config_path/strings/0x409/configuration"
        mkdir "$function_path"
        printf '0' > "$function_path/protocol"
        printf '0' > "$function_path/subclass"
        printf '8' > "$function_path/report_length"
        printf '\x06\x00\xff\x09\x01\xa1\x01\x09\x02\x15\x00\x26\xff\x00\x75\x08\x95\x08\x81\x02\x09\x03\x95\x08\x91\x02\x09\x04\x95\x08\xb1\x02\xc0' \
            > "$function_path/report_desc"
        ln -s "$function_path" "$config_path/$function_name"
        printf '%s' "$udc_name" > "$gadget/UDC"
        udevadm settle

        if [[ ! -e /dev/hidg0 ]]; then
            printf 'The gadget bound, but /dev/hidg0 did not appear.\n' >&2
            cleanup_after_start_failure
            exit 1
        fi
        chgrp wheel /dev/hidg0
        chmod 0660 /dev/hidg0
        trap - ERR
        printf 'Created USB gadget 08f7:0003 on %s.\n' "$udc_name"
        printf 'Protocol endpoint: /dev/hidg0 (group wheel, mode 0660).\n'
        printf 'Start the emulator with: ./build/virtual_golink_uhid --gadget /dev/hidg0\n'
        ;;
    stop)
        if [[ ! -d "$gadget" ]]; then
            printf 'The virtual Go!Link gadget is not present.\n'
            exit 0
        fi
        remove_gadget
        printf 'Removed the virtual Go!Link USB gadget.\n'
        ;;
    *)
        printf 'Usage: sudo %s start|stop\n' "$0" >&2
        exit 2
        ;;
esac
