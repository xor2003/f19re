#!/usr/bin/env python3
"""Verify a reconstructed F19 function compiles to an instruction-identical
routine in the original binary.

Usage:
    python3 tools/verify_f19.py <routine_name> [--exe egame|start|end|su] [--testexe build/x.exe]

- Looks up the routine's extents (segment-relative offsets) in map/<exe>.map
  (the mzmap-derived map, which carries mzdup-assigned names).
- Finds the rebuilt function's offset in the linker map of the test exe
  (build/<x>.map, MS LINK /M output) by public name _<routine_name>.
- Runs mzdiff on the two ranges with --nocall --loose.
"""
import os
import re
import struct
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
MZDIFF = os.environ.get('MZDIFF', os.path.join(ROOT, 'mzretools', 'build', 'mzdiff'))


def routine_extents(name, map_path):
    """name -> (seg_base_linear, start_off, end_off) using segment table."""
    segs = {}
    with open(map_path, errors='replace') as f:
        for line in f:
            line = line.strip()
            m = re.match(r'([\w.$]+) (CODE|DATA|STACK) ([0-9a-fA-F]+)', line)
            if m:
                segs[m.group(1)] = int(m.group(3), 16) * 16
                continue
            m = re.match(r'([\w.$]+): ([\w.$]+) (NEAR|FAR) ([0-9a-fA-F]+)-([0-9a-fA-F]+)', line)
            if m and m.group(1) == name:
                seg = segs[m.group(2)]
                return seg + int(m.group(4), 16), seg + int(m.group(5), 16)
    return None


def public_offset(name, linkmap_path):
    """_name -> linear offset of the public in the linked exe image."""
    with open(linkmap_path, errors='replace') as f:
        for line in f:
            m = re.search(r'([0-9a-fA-F]+):([0-9a-fA-F]+)\s+_' + re.escape(name) + r'\s*$', line)
            if m:
                return int(m.group(1), 16) * 16 + int(m.group(2), 16)
    return None


def main():
    args = sys.argv[1:]
    exe = 'egame'
    testexe = None
    if '--exe' in args:
        i = args.index('--exe')
        exe = args[i + 1]
        del args[i:i + 2]
    if '--testexe' in args:
        i = args.index('--testexe')
        testexe = args[i + 1]
        del args[i:i + 2]
    if not args:
        print(__doc__)
        sys.exit(1)
    name = args[0]

    ref_exe = os.path.join(ROOT, exe.upper() + '.EXE')
    map_path = os.path.join(ROOT, 'map', exe + '.map')
    if testexe is None:
        testexe = os.path.join(ROOT, 'build', exe + 'test.exe')
    linkmap = os.path.splitext(testexe)[0] + '.map'

    ext = routine_extents(name, map_path)
    if not ext:
        print(f'routine {name} not found in {map_path}')
        sys.exit(1)
    tgt_off = public_offset(name, linkmap)
    if tgt_off is None:
        print(f'_{name} not found in {linkmap}')
        sys.exit(1)

    spec_ref = f'{ref_exe}:0x{ext[0]:x}-0x{ext[1]:x}'
    spec_tgt = f'{testexe}:0x{tgt_off:x}'
    cmd = [MZDIFF, spec_ref, spec_tgt, '--nocall', '--loose', '--map', map_path]
    print(' '.join(cmd))
    r = subprocess.run(cmd)
    sys.exit(r.returncode)


if __name__ == '__main__':
    main()
