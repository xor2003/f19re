#!/usr/bin/env python3
"""Dump the Ghidra-decompiled C for a routine in map/<exe>.map.

Ghidra loads the MZ image at paragraph 0x1000, so a routine at image offset
<image_off> (mzmap space) appears as FUN_<segpara>_<off> where
segpara = 0x1000 + image_off//16*16 is only approximate; we locate the file by
scanning decompiled/ for the function whose image offset matches the routine
start. Ghidra offsets equal IDA offsets within each segment (both are relative
to the same paragraph-aligned segment base).

Usage:
    python3 tools/ghidra_src.py <routine> [--exe egame]
    python3 tools/ghidra_src.py --index          # build routine->file index
"""
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
DEC = os.path.join(ROOT, 'decompiled')

# mzmap segment para base -> ghidra segment prefix (image base at 0x1000)
SEGPARA = {0x0000: 0x1000, 0x0FEF: 0x1FEF, 0x1208: 0x2208,
           0x12D6: 0x22D6, 0x1EE7: 0x2EE7}


def routine_start(name, map_path):
    segs = {}
    with open(map_path, errors='replace') as f:
        for line in f:
            m = re.match(r'([\w.$]+) (CODE|DATA|STACK) ([0-9a-fA-F]+)', line.strip())
            if m:
                segs[m.group(1)] = int(m.group(3), 16)
                continue
            m = re.match(r'([\w.$]+): ([\w.$]+) (NEAR|FAR) ([0-9a-fA-F]+)-([0-9a-fA-F]+)', line.strip())
            if m and m.group(1) == name:
                return segs[m.group(2)], int(m.group(4), 16), int(m.group(5), 16)
    return None


def ghidra_file(seg_para, off):
    """Return decompiled path whose FUN name matches this seg:off, or None."""
    gseg = 0x1000 + seg_para
    cand = os.path.join(DEC, 'FUN_%04x_%04x_%04x_%04x.c' % (gseg, off, gseg, off))
    if os.path.exists(cand):
        return cand
    # fall back: scan for any file whose range covers or starts at off
    pat = re.compile(r'FUN_%04x_([0-9a-f]{4,})' % gseg)
    for fn in os.listdir(DEC):
        m = pat.match(fn)
        if m and int(m.group(1), 16) == off:
            return os.path.join(DEC, fn)
    return None


def main():
    args = sys.argv[1:]
    exe = 'egame'
    if '--exe' in args:
        i = args.index('--exe')
        exe = args[i + 1]
        del args[i:i + 2]
    map_path = os.path.join(ROOT, 'map', exe + '.map')
    if args and args[0] == '--index':
        with open(map_path, errors='replace') as f:
            segs = {}
            for line in f:
                m = re.match(r'([\w.$]+) (CODE|DATA|STACK) ([0-9a-fA-F]+)', line.strip())
                if m:
                    segs[m.group(1)] = int(m.group(3), 16)
                    continue
                m = re.match(r'([\w.$]+): ([\w.$]+) (NEAR|FAR) ([0-9a-fA-F]+)-', line.strip())
                if m and m.group(2) in segs:
                    p = ghidra_file(segs[m.group(2)], int(m.group(4), 16))
                    if p:
                        print('%s\t%s' % (m.group(1), os.path.basename(p)))
        return
    if not args:
        print(__doc__)
        sys.exit(1)
    for name in args:
        r = routine_start(name, map_path)
        if not r:
            print('%s: not in %s' % (name, map_path), file=sys.stderr)
            continue
        p = ghidra_file(r[0], r[1])
        if not p:
            print('%s: no decompiled file (seg %04x off 0x%x)' % (name, r[0], r[1]),
                  file=sys.stderr)
            continue
        print('/* ==== %s : seg%04x:0x%04x-0x%04x -> %s ==== */'
              % (name, r[0], r[1], r[2], os.path.basename(p)))
        sys.stdout.write(open(p, errors='replace').read())


if __name__ == '__main__':
    main()
