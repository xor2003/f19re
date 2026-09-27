#!/usr/bin/env python3
"""Map generator for MicroProse *GRAPHIC/*SOUND driver overlays.

These drivers are ordinary MZ executables whose load image begins with an
OvlHeader: a signature string, two segment pointers, and a jump table of
near offsets into the code segment.  mzmap's reachability analysis finds
only `start` on these files because the exported routines are reached via
the table, not via call instructions -- so this tool parses the table and
writes an MS-link-format seed file for `mzmap --linkmap`.

    struct OvlHeader {            // see f15se2-re/src/overlay.c
        uint8  description[0x18]; // "MGRAPHIC.EXE09-19-88"
        uint16 code_segment;      // 0x18: paragraph of the code segment
        uint16 base_segment;      // 0x1a: paragraph of the data segment
        uint16 first_slot;        // 0x1c: index of first jump entry
        uint16 size1;             // 0x1e: bytes of load image
        uint16 size2;             // 0x20: bytes of code segment
        uint16 jump_count;        // 0x22: number of slot entries
        uint16 slot[jump_count];  // 0x24: near offsets into code_segment
    };

Usage:
    drvmap.py DRIVER.EXE out.link [names.txt]   # write mzmap seed linkmap
    drvmap.py DRIVER.EXE --names names.txt      # print slot -> name table

names.txt (optional) is a list of handler names indexed by slot; the f15
graphics ABI lives in f15se2-re/src/slot.h (slots 0x00-0x59 + misc) and the
AdLib ABI in f15se2-ex-integration/src/asound/asound_model.h (0x64-0x6d).
"""
import re
import struct
import sys
import os

sys.path.insert(0, os.path.dirname(__file__))
from compare_exe import load_image


def parse_header(img):
    code_seg, base_seg, first, size1, size2, count = struct.unpack_from('<6H', img, 0x18)
    slots = struct.unpack_from(f'<{count}H', img, 0x24)
    sig = img[:0x18].split(b'\0')[0].decode('ascii', 'replace').strip()
    return dict(sig=sig, code_seg=code_seg, base_seg=base_seg, first=first,
                size1=size1, size2=size2, count=count, slots=slots)


def main():
    if len(sys.argv) < 3:
        print(__doc__)
        return 1
    exe, out = sys.argv[1], sys.argv[2]
    img = load_image(exe)
    h = parse_header(img)
    seg1_lin = h['code_seg'] * 16
    print(f"{exe}: sig='{h['sig']}' first_slot={h['first']} slots={h['count']} "
          f"code para={h['code_seg']:#x} imgsize={len(img):#x}")

    names = {}
    if len(sys.argv) > 3:
        for ln in open(sys.argv[3]):
            m = re.match(r'\s*(0x[0-9a-f]+|\d+)\s+(\w+)', ln)
            if m:
                names[int(m.group(1), 0)] = m.group(2)

    entries = sorted(set(h['slots']))
    base = h['first']
    if out == '--names':
        for i, w in enumerate(h['slots']):
            print(f'slot {base + i:02x} -> seg001:{w:04x} {names.get(base + i, "")}')
        return 0

    with open(out, 'w') as f:
        f.write(' Start         Stop          Length    Name            Class\n')
        f.write(f' 10000H        {0x10000 + seg1_lin - 1:05X}H        {seg1_lin:05X}H    SEG000          CODE\n')
        f.write(f' {0x10000 + seg1_lin:05X}H        {0x10000 + len(img) - 1:05X}H        '
                f'{len(img) - seg1_lin:05X}H    SEG001          CODE\n\n')
        f.write('  Address       Publics by Value\n')
        for w in entries:
            slot_idx = h['slots'].index(w) + base
            nm = names.get(slot_idx, f'cmd_{w:04x}')
            f.write(f' {0x1000 + h["code_seg"]:04X}:{w:04X}     {nm}\n')
    print(f'wrote {out} with {len(entries)} routine seeds')
    return 0


if __name__ == '__main__':
    sys.exit(main())
