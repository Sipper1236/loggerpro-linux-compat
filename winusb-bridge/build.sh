#!/usr/bin/env bash
set -euo pipefail
source_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
build_dir="$source_dir/build"
mkdir -p "$build_dir"

build_coff_object() {
    local source_file="$1"
    local base_name="$2"
    gcc -m32 -S -ffreestanding -fno-pic -fno-stack-protector \
        -fno-asynchronous-unwind-tables -fno-unwind-tables \
        "$source_dir/$source_file" -o "$build_dir/$base_name.s"
    sed -E '/^[[:space:]]*\.(type|size|ident|cfi|section[[:space:]]+\.note\.GNU-stack)/d' \
        "$build_dir/$base_name.s" > "$build_dir/${base_name}_coff.s"
    llvm-mc -triple=i686-pc-windows-gnu -filetype=obj \
        "$build_dir/${base_name}_coff.s" -o "$build_dir/$base_name.obj"
    objcopy --prefix-symbols=_ "$build_dir/$base_name.obj" "$build_dir/${base_name}_pref.obj"
}

llvm-dlltool -m i386 -k -D ws2_32.dll -d "$source_dir/ws2_32.def" -l "$build_dir/ws2_32.lib"
llvm-dlltool -m i386 -k -D kernel32.dll -d "$source_dir/kernel32.def" -l "$build_dir/kernel32.lib"
llvm-dlltool -m i386 -k -D winusb.dll -d "$source_dir/winusb_import.def" -l "$build_dir/winusb_import.lib"
llvm-dlltool -m i386 -k -D kernel32.dll -d "$source_dir/kernel32_test.def" -l "$build_dir/kernel32_test.lib"

build_coff_object winusb.c winusb
lld-link /dll /noentry /machine:x86 /safeseh:no \
    /def:"$source_dir/winusb.def" /out:"$build_dir/winusb.dll" \
    "$build_dir/winusb_pref.obj" "$build_dir/ws2_32.lib" "$build_dir/kernel32.lib"

gcc -O2 -Wall -Wextra -Werror -o "$build_dir/bridge" "$source_dir/bridge.c" \
    $(pkg-config --cflags --libs libusb-1.0) -pthread
gcc -O2 -Wall -Wextra -Werror -o "$build_dir/bridge-virtual" \
    "$source_dir/bridge.c" "$source_dir/virtual_libusb.c" \
    $(pkg-config --cflags libusb-1.0) -pthread

for probe in probe.c probe_no_device.c probe_virtual_usb.c; do
    base_name="${probe%.c}"
    build_coff_object "$probe" "$base_name"
    lld-link /subsystem:console /machine:x86 /safeseh:no /entry:test_entry \
        /out:"$build_dir/$base_name.exe" "$build_dir/${base_name}_pref.obj" \
        "$build_dir/winusb_import.lib" "$build_dir/kernel32_test.lib"
done

printf 'Built artifacts in %s\n' "$build_dir"
