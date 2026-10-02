#!/usr/bin/env python3
"""Repair the MZ header of a skeleton-built exe so it runs under DOS.

exe2asm.py skeletons emit code/data as raw bytes; the original's intra-image
far calls and `mov ax,seg x` immediates therefore carry no fixups, and LINK
writes an almost-empty relocation table (SU: 1 entry vs 33).  DOS applies
the table at load time, so the unpatched exe hangs on unrelocated far refs.

The load image is already verified byte-identical, and MZ reloc entries are
image-relative (seg:off word -> frame para to fix up), so the original's
table transplants verbatim.  Also copied: the runtime header fields the
skeleton link computes differently (e_minalloc/e_maxalloc/e_ss/e_sp/e_ip/
e_cs/e_ovno).  e_cblp/e_cp/e_cparhdr are recomputed for our header size.

Usage: mzhdrfix.py ORIG.EXE BUILT.EXE     (BUILT.EXE patched in place)
"""
import struct
import sys

sys.path.insert(0, __file__.rsplit('/', 1)[0])
from compare_exe import load_image


def image_bounds(data):
    cblp, cp, hdr_paras = struct.unpack_from('<HHH', data, 2)[0], \
        struct.unpack_from('<H', data, 4)[0], struct.unpack_from('<H', data, 8)[0]
    hdr = hdr_paras * 16
    size = (cp - 1) * 512 + (cblp or 512) - hdr
    return hdr, size


def main():
    orig, tgt = sys.argv[1], sys.argv[2]
    a = open(orig, 'rb').read()
    b = open(tgt, 'rb').read()
    ha, ia = image_bounds(a)
    hb, ib = image_bounds(b)
    imga, imgb = a[ha:ha + ia], b[hb:hb + ib]
    if imga != imgb:
        print('mzhdrfix: load images differ, refusing', file=sys.stderr)
        return 1

    rlc = struct.unpack_from('<H', a, 6)[0]
    lfa = struct.unpack_from('<H', a, 24)[0]
    table = a[lfa:lfa + rlc * 4]

    # new header: fields block then table at 0x1c, cparhdr sized to fit
    lfa_new = 0x1c
    cparhdr = (lfa_new + len(table) + 15) // 16
    hsize = cparhdr * 16
    h = bytearray(hsize)
    h[0:2] = b'MZ'
    total = hsize + len(imgb)
    cblp, cp = total % 512, (total + 511) // 512
    struct.pack_into('<H', h, 2, cblp)
    struct.pack_into('<H', h, 4, cp)
    struct.pack_into('<H', h, 6, rlc)
    struct.pack_into('<H', h, 8, cparhdr)
    # runtime fields straight from the original header
    for off in (10, 12, 14, 16, 18, 20, 22, 26):      # minalloc..ovno
        struct.pack_into('<H', h, off, struct.unpack_from('<H', a, off)[0])
    struct.pack_into('<H', h, 24, lfa_new)
    h[lfa_new:lfa_new + len(table)] = table
    open(tgt, 'wb').write(bytes(h) + imgb)
    print(f'mzhdrfix: {tgt} rlc={rlc} hdr={hsize}B total={total}B')
    return 0


if __name__ == '__main__':
    sys.exit(main())
