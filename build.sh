#!/bin/bash
set -e
cd -- "$(dirname -- "$0")"
gcc -m32 -O2 -ffreestanding -fno-pic -fno-stack-protector -fno-asynchronous-unwind-tables -S loggerpro_bluetooth.c -o shim-elf.s
python - <<'PY'
from pathlib import Path
s=Path('shim-elf.s').read_text(); lines=[]
for l in s.splitlines():
 if l.strip().startswith(('.type','.size','.ident','.local','.section\t.note')):continue
 lines.append(l.replace('.section\t.rodata','.section .rdata,"dr"').replace('DllGetActivationFactory','_DllGetActivationFactory'))
Path('shim.s').write_text('\n'.join(lines)+'\n')
PY
llvm-mc -triple=i686-pc-windows-msvc -filetype=obj shim.s -o shim.obj
lld-link /dll /noentry /nodefaultlib /machine:x86 /safeseh:no /export:DllGetActivationFactory /out:loggerpro_bluetooth.dll shim.obj
