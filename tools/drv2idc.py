#!/usr/bin/env python3
"""Generate an IDC script describing a MicroProse driver overlay for
ada.py (-s script.idc).

Without it, ada treats the whole load image as one code segment and
disassembles the OvlHeader + jump table as instructions. The script:

- deletes the flat segments, redeclares seg000 (header/data) as DATA and
  the code segment (from the OvlHeader code_segment field) as CODE
- marks the jump-table words as `word` data items
- seeds every map routine with add_func(start,end) + set_name

Usage: drv2idc.py DRIVER.EXE map/x.map out.idc
"""
import struct
import sys
import os

sys.path.insert(0, os.path.dirname(__file__))
from compare_exe import load_image
from drv2asm import parse_map

IMG_BASE = 0x10000   # ada loads the image at linear 0x10000


def main():
    exe, mapfile, out = sys.argv[1], sys.argv[2], sys.argv[3]
    img = load_image(exe)
    segs, routines = parse_map(mapfile)
    seg_para = {n: p for n, p in segs.items()}
    code_seg = max(seg_para, key=seg_para.get)
    code_lin0 = seg_para[code_seg] * 16
    code_para = 0x1000 + seg_para[code_seg]
    img_end = IMG_BASE + len(img)

    code_rs = [r for r in routines if r['seg'] == code_seg]
    L = []
    L.append('// auto-generated overlay description for ada.py -s')
    L.append(f'// {os.path.basename(exe)}  map={mapfile}')
    L.append('delete_all_segments();')
    L.append(f'add_segm_ex(0X{IMG_BASE:X}, 0X{IMG_BASE + code_lin0:X}, '
             f'0X{0x1000:X}, 0, 1, 2, "seg000");')
    L.append(f'add_segm_ex(0X{IMG_BASE + code_lin0:X}, 0X{img_end:X}, '
             f'0X{code_para:X}, 0, 1, 2, "seg001");')
    L.append(f'SegRename(0X{IMG_BASE:X}, "seg000");')
    L.append(f'SegRename(0X{IMG_BASE + code_lin0:X}, "seg001");')
    L.append(f'segclass(0X{IMG_BASE:X}, "DATA");')
    L.append(f'segclass(0X{IMG_BASE + code_lin0:X}, "CODE");')
    # ds inside the code segment points at seg000 (the driver's data)
    L.append(f'segdefreg(0X{IMG_BASE + code_lin0:X}, "ds", 0X{0x1000:X});')
    L.append('')
    # header fields + jump table as data words
    _, _, first, size1, size2, count = struct.unpack_from('<6H', img, 0x18)
    for i in range(0x18):
        L.append(f'create_byte(0X{IMG_BASE + i:X});')
    for i in range(0x18, 0x24 + 2 * count, 2):
        L.append(f'create_word(0X{IMG_BASE + i:X});')
    L.append('')
    for r in sorted(code_rs, key=lambda r: r['lo']):
        s = IMG_BASE + code_lin0 + r['lo']
        e = IMG_BASE + code_lin0 + r['hi'] + 1
        L.append(f'add_func(0X{s:X}, 0X{e:X});')
        L.append(f'set_name(0X{s:X}, "{r["name"]}");')
        if r['type'] == 'FAR':
            L.append(f'set_func_flags(0X{s:X}, 2);')  # FUNC_FAR
    L.append('')
    open(out, 'w').write('\n'.join(L) + '\n')
    print(f'wrote {out}: {len(code_rs)} funcs, code at '
          f'{code_lin0:#x}..{len(img):#x}')


if __name__ == '__main__':
    main()
