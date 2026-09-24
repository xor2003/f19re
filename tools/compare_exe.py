#!/usr/bin/env python3
"""Byte-compare the load images of the original and rebuilt F19 executables.

Usage:
    python3 tools/compare_exe.py [ref_exe [tgt_exe]]

Defaults: EGAME.EXE vs build/egame.exe. Reports grouped byte-diff ranges in
linear load-image offsets. Exit status 0 when identical in the common range.
"""
import itertools
import os
import struct
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)


def load_image(path):
    """Return the MZ load image, bounded by the header's declared size.

    e_cp (offset 4) counts 512-byte pages; e_cblp (offset 2) is the byte count
    in the last page (0 means full). Anything past that is file slack DOS never
    loads and is ignored here."""
    data = open(path, 'rb').read()
    hdr_paras = struct.unpack('<H', data[8:10])[0]
    cblp, cp = struct.unpack('<HH', data[2:6])
    image_size = (cp - 1) * 512 + (cblp or 512) - hdr_paras * 16
    return data[hdr_paras * 16:hdr_paras * 16 + image_size]


def main():
    ref = sys.argv[1] if len(sys.argv) > 1 else os.path.join(ROOT, 'EGAME.EXE')
    tgt = sys.argv[2] if len(sys.argv) > 2 else os.path.join(ROOT, 'build', 'egame.exe')
    a, b = load_image(ref), load_image(tgt)
    print(f'{os.path.basename(ref)}: {len(a):#x} bytes, {os.path.basename(tgt)}: {len(b):#x} bytes')
    diffs = [k for k in range(min(len(a), len(b))) if a[k] != b[k]]
    print(f'total byte diffs in common range: {len(diffs)}')
    for _, g in itertools.groupby(enumerate(diffs), lambda t: t[1] - t[0]):
        grp = [x[1] for x in g]
        s, e = grp[0], grp[-1]
        print(f'@{s:05x}-{e:05x} ({e - s + 1}B): orig={a[max(0, s - 4):e + 5].hex()} ours={b[max(0, s - 4):e + 5].hex()}')
    if len(a) != len(b):
        print(f'load-image size differs by {len(b) - len(a)} bytes')
    sys.exit(1 if diffs or len(a) != len(b) else 0)


if __name__ == '__main__':
    main()
