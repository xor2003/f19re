#!/usr/bin/env python3
"""Generate batch-2 dosunit specs: START.EXE vs STUTIL.EXE and EGAME.EXE vs
EG3DMAP.EXE / EGUI.EXE / self (scaleCoordToLod harvest).

Patch model: with ds=<oracle DGRP para> the oracle reads its real initialized
data; every candidate-side dseg cell it touches is patched at the candidate's
own offset. BSS-only data (grid buffers, dyn tile pool) gets identical
synthetic bytes patched at both offsets. Cross-segment far pointers
(gameData) point at seg 0x7000 where per-vector patches place fields.
"""
import json
import os
import re
import struct
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, 'tools'))
from duspec import cand_off, cand_dgrp, rand_seed_cell, arg_lit_off
EN = '/home/xor/games/f19/F19'


def img(path):
    d = open(path, 'rb').read()
    return d[struct.unpack('<H', d[8:10])[0] * 16:]


def ext(exe, off, size):
    """hex bytes from exe load image at image offset off."""
    return img(exe)[off:off + size].hex()


def w16(v):
    return struct.pack('<H', v & 0xFFFF)


def c(mappath, sym):
    """_sym -> dseg off in an MSC LINK map — survives test-exe relinks."""
    v = cand_off(os.path.join(ROOT, mappath), sym)
    assert v is not None, (mappath, sym)
    return v


def dg(mappath):
    """(cand_dgrp, code_c) for a module test exe, derived from its MAP."""
    d = cand_dgrp(os.path.join(ROOT, mappath))
    assert d is not None, mappath
    return d, [[0, d]]


START_O = EN + '/START.EXE'
START_OM = 'map/start_en.map'
STUTIL = 'build/STUTIL.EXE'
STUTIL_M = 'build/STUTIL.MAP'
EGAME_O = EN + '/EGAME.EXE'
EGAME_OM = 'map/egame_en.map'
EG3DMAP = 'build/EG3DMAP.EXE'
EG3DMAP_M = 'build/EG3DMAP.MAP'
EGUI = 'build/EGUI.EXE'
EGUI_M = 'build/EGUI.MAP'
EGMATH_M = 'build/EGMATH.MAP'

# oracle dgroup image offsets: START dseg@0x10000, EGAME Data1@0x1ecf0
START_DGRP = 0x10000
EGAME_DGRP = 0x1ECF0

# candidate dgrp/code windows — derived from each module's LINK map so a
# stubs/layout relink never silently invalidates a spec
DG_STUTIL, CODE_STUTIL = dg(STUTIL_M)
DG_3DMAP, CODE_3DMAP = dg(EG3DMAP_M)
DG_EGUI, CODE_EGUI = dg(EGUI_M)
DG_EGMATH, CODE_EGMATH = dg(EGMATH_M)


def cell(pairs, ob, cb, data):
    """both-side patch: bytes `data` at oracle ds:ob and cand ds:cb."""
    return {'o_off': hex(ob), 'c_off': hex(cb), 'bytes': data.hex()
            if isinstance(data, (bytes, bytearray)) else data}


def ramp(n, mod=256, seed=1):
    return bytes(((i * 5 + seed) & 0xFF) % mod for i in range(n))


# ---------------------------------------------------------------- START util
start = []

# cand rngState cell + the theater-name literal pushed by formatGridRef
# (first `mov ax,imm; push ax` src arg of its mystrcpy call) — both move
# with test-exe relinks
RNG_S = c(STUTIL_M, 'rngState')
TH_LIT = arg_lit_off('STUTIL', 'formatGridRef', 'mystrcpy', 0, 1)

# srand(seed): writes rngState dword (o:0x7A50 c:0xB000)
start.append({
    'fn': 'srand', 'oracle_exe': START_O, 'oracle_map': START_OM,
    'oracle_off': 0xe29e, 'oracle_dgrp': START_DGRP,
    'cand_exe': STUTIL, 'cand_map': STUTIL_M, 'cand_dgrp': DG_STUTIL,
    'ds': '0x2000',
    'observe': [{'o_off': '0x7a50', 'c_off': hex(RNG_S), 'size': 4}],
    'check_regs': [],
    'code_o': [[0, 0x10000]], 'code_c': CODE_STUTIL,
    'vectors': [[0], [1], [0x7fff], [-1], [-32768], [0x1234], [0x4000]],
})

# rand(): MSVC LCG on rngState; returns (state>>16)&0x7fff
start.append({
    'fn': 'rand', 'oracle_exe': START_O, 'oracle_map': START_OM,
    'oracle_off': 0xe2b0, 'oracle_dgrp': START_DGRP,
    'cand_exe': STUTIL, 'cand_map': STUTIL_M, 'cand_dgrp': DG_STUTIL,
    'ds': '0x2000',
    'observe': [{'o_off': '0x7a50', 'c_off': hex(RNG_S), 'size': 4}],
    'code_o': [[0, 0x10000]], 'code_c': CODE_STUTIL,
    'vectors': [
        {'args': [], 'patches': [cell(None, 0x7a50, RNG_S, struct.pack('<I', s))]}
        for s in (0, 1, 0x12345678, 0xffffffff, 0x7fffffff, 0x0000ffff,
                  0xdeadbeef, 0x00008000)
    ],
})

# randMul(arg): (rand()*arg)>>15 — patch state, observe ax + evolved state
start.append({
    'fn': 'randMul', 'oracle_exe': START_O, 'oracle_map': START_OM,
    'oracle_off': 0x40ae, 'oracle_dgrp': START_DGRP,
    'cand_exe': STUTIL, 'cand_map': STUTIL_M, 'cand_dgrp': DG_STUTIL,
    'ds': '0x2000',
    'observe': [{'o_off': '0x7a50', 'c_off': hex(RNG_S), 'size': 4}],
    'code_o': [[0, 0x10000]], 'code_c': CODE_STUTIL,
    'vectors': [
        {'args': [a], 'patches': [cell(None, 0x7a50, RNG_S, struct.pack('<I', s))]}
        for s, a in ((1, 0), (1, 1), (1, 5), (1, 100), (1, 0x7fff),
                     (1, 0x8000), (1, 0xffff), (0x12345678, 10),
                     (0x12345678, 0x7fff), (0x12345678, -1),
                     (0xffffffff, 7), (0x7fffffff, 0x1000), (1, -32768))
    ],
})

# mystrcpy(dst, src): leaf copy
strs = ['', 'A', 'TD74', 'HELLO WORLD', 'x' * 40, 'abc.def', 'a:b|c']
start.append({
    'fn': 'mystrcpy', 'oracle_exe': START_O, 'oracle_map': START_OM,
    'oracle_off': 0x5120, 'oracle_dgrp': START_DGRP,
    'cand_exe': STUTIL, 'cand_map': STUTIL_M, 'cand_dgrp': DG_STUTIL,
    'ds': '0x2000',
    'observe': [{'off': '0xe200', 'size': 48}],
    'check_regs': [],
    'code_o': [[0, 0x10000]], 'code_c': CODE_STUTIL,
    'vectors': [
        {'args': [0xe200, 0xe100],
         'patches': [{'off': '0xe100', 'bytes': s.encode().hex() + '00'}]}
        for s in strs
    ],
})

# my_itoa(v, buf): args [v, bufoff]
start.append({
    'fn': 'my_itoa', 'oracle_exe': START_O, 'oracle_map': START_OM,
    'oracle_off': 0x3fb3, 'oracle_dgrp': START_DGRP,
    'cand_exe': STUTIL, 'cand_map': STUTIL_M, 'cand_dgrp': DG_STUTIL,
    'ds': '0x2000',
    'observe': [{'off': '0xe300', 'size': 12}],
    'check_regs': [],
    'code_o': [[0, 0x10000]], 'code_c': CODE_STUTIL,
    'vectors': [
        {'args': [v, 0xe300]} for v in
        (0, 1, 9, 10, 99, 100, 999, 1000, 9999, 10000, 12345, 32767,
         -1, -9, -99, -999, -1000, -9999, -10000, -32768, 0x4000, 60)
    ],
})

# my_ltoa(v32, buf): args [vlo, vhi, bufoff]
lvals = [0, 1, 9, 99, 999, 1000, 9999, 10000, 99999, 100000, 999999,
         1000000, 9999999, 0x7fffffff, 123456789, -1, -999, -1000,
         -9999, -32768, -2147483648, 0x10000, 5]
start.append({
    'fn': 'my_ltoa', 'oracle_exe': START_O, 'oracle_map': START_OM,
    'oracle_off': 0x3e7c, 'oracle_dgrp': START_DGRP,
    'cand_exe': STUTIL, 'cand_map': STUTIL_M, 'cand_dgrp': DG_STUTIL,
    'ds': '0x2000',
    'observe': [{'off': '0xe340', 'size': 16}],
    'check_regs': [],
    'code_o': [[0, 0x10000]], 'code_c': CODE_STUTIL,
    'vectors': [
        {'args': [v & 0xffff, (v >> 16) & 0xffff, 0xe340]} for v in lvals
    ],
})

# evalChoiceExpr(pp_cell, idx): cell at 0xe000 holds stream off 0xe100
streams = [
    ')', '|', ':0', ':5', ':99', ':32767',
    '(|))', '((x))|', '(a(b)c):7', 'x:3', '((())):12', ':1' + 'x' * 30 + ')',
]
start.append({
    'fn': 'evalChoiceExpr', 'oracle_exe': START_O, 'oracle_map': START_OM,
    'oracle_off': 0x669f, 'oracle_dgrp': START_DGRP,
    'cand_exe': STUTIL, 'cand_map': STUTIL_M, 'cand_dgrp': DG_STUTIL,
    'ds': '0x2000',
    'observe': [{'off': '0xe000', 'size': 2}],
    'code_o': [[0, 0x10000]], 'code_c': CODE_STUTIL,
    'vectors': [
        {'args': [0xe000, 0],
         'patches': [{'off': '0xe000', 'bytes': '00e1'},
                     {'off': '0xe100',
                      'bytes': s.encode().hex() + '00'}]}
        for s in streams
    ],
})

# lookupGridCell(level,col,row): recursive quadtree
# bufs: oracle L4@0xB374(16B) L3@0xA4C8(256B) L2@0xA2C6(512B) L1@0x9B54(512B)
#        L0@0x994E(512B); cand L4@0x8A84 L3@0x8A9E L2@0x8CA2 L1@0x8EA8 L0@0x9454
# parent results feed (parent<<4) sub-index — keep all buf values <= 28 so
# 512B buffers stay in bounds; gridLevelSize real [1024,256,64,16,4].
def gridpatches():
    # 8 words incl. trailing real data so OOB level reads are equal
    dims8 = img(START_O)[START_DGRP + 0x3bb4:START_DGRP + 0x3bb4 + 16]
    pats = [cell(None, 0x3bb4, c(STUTIL_M, 'gridLevelSize'), dims8)]
    # oracle (off,size) vs candidate off — L4 16B, L3 256B, L2/L1/L0 512B
    for o_off, c_off, sz, mod in ((0xb374, c(STUTIL_M, 'gridBuf1'), 16, 29),
                                (0xa4c8, c(STUTIL_M, 'gridBuf2'), 256, 29),
                                (0xa2c6, c(STUTIL_M, 'gridBuf3'), 512, 29),
                                (0x9b54, c(STUTIL_M, 'gridBuf4'), 512, 29),
                                (0x994e, c(STUTIL_M, 'gridBuf5'), 512, 29)):
        pats.append(cell(None, o_off, c_off, ramp(sz, mod)))
    return pats

lv = []
for level in (4, 3, 2, 1, 0):
    dim = [1024, 256, 64, 16, 4][level]
    lv += [
        {'args': [level, 0, 0]},
        {'args': [level, dim - 1, dim - 1]},
        {'args': [level, dim, 0]},
        {'args': [level, 0, dim]},
        {'args': [level, -1, 0]},
        {'args': [level, 0, -1]},
        {'args': [level, dim >> 1, dim >> 1]},
    ]
lv += [
    {'args': [5, 0, 0]},          # out-of-table level (dim[5]=12593 real)
    {'args': [0, 1023, 1023]},    # deepest descent
    {'args': [1, 100, 200]},
    {'args': [2, 63, 63]},        # max in-range for dim64
    {'args': [2, 64, 64]},        # just out
    {'args': [3, 15, 15]},
    {'args': [4, 3, 3]},
    {'args': [4, 4, 0]},
]
start.append({
    'fn': 'lookupGridCell', 'oracle_exe': START_O, 'oracle_map': START_OM,
    'oracle_off': 0x70e0, 'oracle_dgrp': START_DGRP,
    'cand_exe': STUTIL, 'cand_map': STUTIL_M, 'cand_dgrp': DG_STUTIL,
    'ds': '0x2000',
    'patches': gridpatches(),
    'code_o': [[0, 0x10000]], 'code_c': CODE_STUTIL,
    'vectors': lv,
})

# replaceExtension(path, ext): scans to '.' or NUL, mystrcpy there
rep = [
    ('FILE.PIC', '.3DG'), ('A.B.C', '.X'), ('NOEXT', '.EXT'),
    ('.DOT', '.E'), ('EMPTY', ''), ('LONGNAME.OLD', '.NNNNNN'),
    ('x', '.y'), ('dir\\file.dat', '.BIN'),
]
start.append({
    'fn': 'replaceExtension', 'oracle_exe': START_O, 'oracle_map': START_OM,
    'oracle_off': 0x7534, 'oracle_dgrp': START_DGRP,
    'cand_exe': STUTIL, 'cand_map': STUTIL_M, 'cand_dgrp': DG_STUTIL,
    'ds': '0x2000',
    'observe': [{'off': '0xe400', 'size': 40}],
    'check_regs': [],
    'code_o': [[0, 0x10000]], 'code_c': CODE_STUTIL,
    'vectors': [
        {'args': [0xe400, 0xe500],
         'patches': [{'off': '0xe400', 'bytes': p.encode().hex() + '00'},
                     {'off': '0xe500', 'bytes': e.encode().hex() + '00'}]}
        for p, e in rep
    ],
})

# getItemCoordStr(idx): worldObjects[i] (16B stride: +0 x, +2 y) -> formatGridRef
# gameData far ptr o:0x991C/c:0xB4E6 -> 0x7000:0; theater at 0x7000:0x38
objs = []
for i in range(6):
    x, y = (0x100 * i + 0x20, 0x80 * i - 0x40)  # arbitrary nonzero coords
    rec = bytearray(16)
    struct.pack_into('<h', rec, 0, x)
    struct.pack_into('<h', rec, 2, y)
    objs.append(bytes(rec))
wo = b''.join(objs)
gics = []
for i, th in ((0, 0), (1, 1), (2, 2), (3, 3), (4, 0), (5, 2), (0, 3), (2, 1)):
    gics.append({'args': [i],
                 'patches': [{'seg': '0x7000', 'off': '0x38',
                              'bytes': w16(th).hex()}]})
start.append({
    'fn': 'getItemCoordStr', 'oracle_exe': START_O, 'oracle_map': START_OM,
    'oracle_off': 0x8d74, 'oracle_dgrp': START_DGRP,
    'cand_exe': STUTIL, 'cand_map': STUTIL_M, 'cand_dgrp': DG_STUTIL,
    'ds': '0x2000',
    'check_regs': [],
    'patches': [
        cell(None, 0x991c, c(STUTIL_M, 'gameData'), b'\x00\x00\x00\x70'),   # gameData = 7000:0
        cell(None, 0xb390, c(STUTIL_M, 'worldObjects'), wo),                    # worldObjects
        {'c_off': hex(TH_LIT), 'size': 26},                # theater name literals
    ],
    'observe': [{'o_off': '0x98cc', 'c_off': hex(c(STUTIL_M, 'bufCoordStr')), 'size': 8}],
    'code_o': [[0, 0x10000]], 'code_c': CODE_STUTIL,
    'vectors': gics,
})

# ------------------------------------------------------------- EGAME 3D map
e3d = []
M = EG3DMAP_M
pure = lambda fn, off, vals: {
    'fn': fn, 'oracle_exe': EGAME_O, 'oracle_map': EGAME_OM,
    'oracle_off': off, 'oracle_dgrp': EGAME_DGRP,
    'cand_exe': EG3DMAP, 'cand_map': EG3DMAP_M, 'cand_dgrp': DG_3DMAP,
    'ds': '0x6000',
    'code_o': [[0, 0xfd80]], 'code_c': CODE_3DMAP,
    'vectors': [[v] for v in vals],
}
e3d.append(pure('aspectScaleY', 0x19c8,
                [0, 1, 3, 4, 5, 7, 8, 100, 200, 1000, 0x4000, 0x7fff,
                 -1, -4, -5, -100, -1024, -32768]))

# worldToTileIndex(wX,wY,&col,&row): out ptrs at ds:0xf000/0xf002
# oracle cells: viewCX 0x9a38 viewCY2 0x65c4 orgX 0x6326 orgY 0x6328 tileSz 0x6322
# cand cells resolved by symbol: g_viewCenterX/g_viewCenterY2/g_mapOriginX/Y/g_tileWorldSize
def w2t_patches(vcx, vcy, ox, oy, tsz):
    return [cell(None, 0x9a38, c(M, 'g_viewCenterX'), w16(vcx)),
            cell(None, 0x65c4, c(M, 'g_viewCenterY2'), w16(vcy)),
            cell(None, 0x6326, c(M, 'g_mapOriginX'), w16(ox)),
            cell(None, 0x6328, c(M, 'g_mapOriginY'), w16(oy)),
            cell(None, 0x6322, c(M, 'g_tileWorldSize'), w16(tsz))]

w2t = []
geo = (0x4000, 0x2000, 0x1000, 0x800, 0x400)   # vcx vcy orgx orgy tsz=1024
for wx, wy in [(0, 0), (0x4000, 0x2000), (0x4100, 0x2100), (0x3fff, 0x1fff),
               (0, 0x2000), (0x4000, 0), (-0x4000, -0x2000), (0x4400, 0x2400),
               (0x4000 + 1023, 0x2000 + 1023), (0x4000 + 1024, 0x2000 + 1024),
               (0x4000 - 1, 0x2000 - 1), (0x7fff, 0x7fff), (-32768, -32768),
               (0x4000, -0x2000)]:
    w2t.append({'args': [wx, wy, 0xf000, 0xf002],
                'patches': w2t_patches(*geo)})
for tsz in (1, 2, 0x100, 0x400, 0x1000, 0x7fff):   # tile-size sweep
    w2t.append({'args': [0x4000 + tsz * 2 + tsz - 1, 0x2000, 0xf000, 0xf002],
                'patches': w2t_patches(0x4000, 0x2000, 0x1000, 0x800, tsz)})
w2t.append({'args': [0, 0, 0xf000, 0xf002],
            'patches': w2t_patches(0x7fff, -32768, -1, 0x7fff, 3)})
e3d.append({
    'fn': 'worldToTileIndex', 'oracle_exe': EGAME_O, 'oracle_map': EGAME_OM,
    'oracle_off': 0x173a, 'oracle_dgrp': EGAME_DGRP,
    'cand_exe': EG3DMAP, 'cand_map': EG3DMAP_M, 'cand_dgrp': DG_3DMAP,
    'ds': '0x6000',
    'observe': [{'off': '0xf000', 'size': 4}],
    'check_regs': [],
    'code_o': [[0, 0xfd80]], 'code_c': CODE_3DMAP,
    'vectors': w2t,
})

# computeTileBounds(&minX,&maxX,&minY,&maxY): + clipMax (o:0x3b83/85
# c:0x5b4a/0x5c8e) + tileGridDim (o:0x6324 c:0x57e4)
def ctb_patches(vcx, vcy, ox, oy, tsz, cx, cy, dim):
    return w2t_patches(vcx, vcy, ox, oy, tsz) + [
        cell(None, 0x3b83, c(M, 'g_clipMaxX'), w16(cx)),
        cell(None, 0x3b85, c(M, 'g_clipMaxY'), w16(cy)),
        cell(None, 0x6324, c(M, 'g_tileGridDim'), w16(dim))]

ctb = []
for cx, cy, dim in [(319, 199, 16), (319, 199, 4), (0, 0, 16), (319, 199, 1),
                    (0x7fff, 0x7fff, 16), (319, 199, 64), (160, 100, 8)]:
    ctb.append({'args': [0xf000, 0xf002, 0xf004, 0xf006],
                'patches': ctb_patches(0x4000, 0x2000, 0x1000, 0x800, 0x400,
                                       cx, cy, dim)})
ctb.append({'args': [0xf000, 0xf002, 0xf004, 0xf006],   # negative view origin
            'patches': ctb_patches(-0x1000, -0x800, 0x1000, 0x800, 0x400,
                                   319, 199, 16)})
e3d.append({
    'fn': 'computeTileBounds', 'oracle_exe': EGAME_O, 'oracle_map': EGAME_OM,
    'oracle_off': 0x16de, 'oracle_dgrp': EGAME_DGRP,
    'cand_exe': EG3DMAP, 'cand_map': EG3DMAP_M, 'cand_dgrp': DG_3DMAP,
    'ds': '0x6000',
    'observe': [{'off': '0xf000', 'size': 8}],
    'check_regs': [],
    'code_o': [[0, 0xfd80]], 'code_c': CODE_3DMAP,
    'vectors': ctb,
})

# lookupTileEntry(lod,subIdx,tx,ty): pool o:0x8b38 c:0x8e2e, count o:0x664a
# c:0x4506, idx o:0x6320 c:0x8d18; entry = {lod,sub,tx,ty : u8 x4, value i16, pad}
def te(lod, sub, tx, ty, val):
    return struct.pack('<BBBBHH', lod & 0xff, sub & 0xff, tx & 0xff,
                       ty & 0xff, val & 0xffff, 0)

pool3 = te(2, 0, 5, 6, 0x1111) + te(1, 7, 9, 2, 0x2222) + te(2, 0, 5, 6, 0x3333)
lte_patches = [cell(None, 0x664a, c(M, 'g_tileEntryCount'), w16(3)),
               cell(None, 0x8b38, c(M, 'g_dynTileEntries'), pool3)]
lte = []
for lod, sub, tx, ty in [(2, 0, 5, 6), (1, 7, 9, 2), (2, 0, 5, 7), (0, 0, 0, 0),
                         (2, 0, 5, 6), (4, 0, 0, 0), (2, 1, 5, 6),
                         (0x102, 0, 5, 6), (-1, 0, 5, 6)]:
    lte.append({'args': [lod, sub, tx, ty], 'patches': lte_patches})
lte.append({'args': [2, 0, 5, 6],
            'patches': [cell(None, 0x664a, c(M, 'g_tileEntryCount'), w16(0))]})  # empty pool
lte.append({'args': [1, 7, 9, 2],
            'patches': [cell(None, 0x664a, c(M, 'g_tileEntryCount'), w16(1)),
                        cell(None, 0x8b38, c(M, 'g_dynTileEntries'), pool3)]})  # count<pool
lte.append({'args': [2, 0, 5, 6],
            'patches': [cell(None, 0x664a, c(M, 'g_tileEntryCount'), w16(-1))]})  # neg count
e3d.append({
    'fn': 'lookupTileEntry', 'oracle_exe': EGAME_O, 'oracle_map': EGAME_OM,
    'oracle_off': 0x130c, 'oracle_dgrp': EGAME_DGRP,
    'cand_exe': EG3DMAP, 'cand_map': EG3DMAP_M, 'cand_dgrp': DG_3DMAP,
    'ds': '0x6000',
    'observe': [{'o_off': '0x6320', 'c_off': hex(c(M, 'g_tileEntryIdx')), 'size': 2}],
    'code_o': [[0, 0xfd80]], 'code_c': CODE_3DMAP,
    'vectors': lte,
})

# addTileEntry(rec, value, tag): fake rec at ds:0xf100 (entry ptr +0xc ->
# 0xf200), payload copied rec+0xe..0x15 into pool[count]; count cell o:0x664a
# c:0x4506; pool o:0x8b38 c:0x8e2e; rec->entry->shape(+6) |= 0x80
def atvec(count, value, tag):
    rec = bytearray(0x16)
    struct.pack_into('<H', rec, 0x0c, 0xf200)      # entry ptr
    rec[0x0e] = 2                                  # lod
    rec[0x0f] = 1                                  # subIndex
    rec[0x10] = 9                                  # tileX
    rec[0x11] = 8                                  # tileY
    ent = bytearray(8)
    ent[6] = 0x05                                  # shape byte to OR
    return {'args': [0xf100, value, tag],
            'patches': [{'off': '0xf100', 'bytes': bytes(rec).hex()},
                        {'off': '0xf200', 'bytes': bytes(ent).hex()},
                        cell(None, 0x664a, c(M, 'g_tileEntryCount'), w16(count)),
                        cell(None, 0x8b38, c(M, 'g_dynTileEntries'), bytes(64).hex())]}

e3d.append({
    'fn': 'addTileEntry', 'oracle_exe': EGAME_O, 'oracle_map': EGAME_OM,
    'oracle_off': 0x12ca, 'oracle_dgrp': EGAME_DGRP,
    'cand_exe': EG3DMAP, 'cand_map': EG3DMAP_M, 'cand_dgrp': DG_3DMAP,
    'ds': '0x6000',
    'observe': [{'off': '0xf100', 'size': 0x16},
                {'off': '0xf206', 'size': 1},
                {'o_off': '0x8b30', 'c_off': hex(c(M, 'g_dynTileEntries') - 8), 'size': 24},
                {'o_off': '0x664a', 'c_off': hex(c(M, 'g_tileEntryCount')), 'size': 2}],
    'check_regs': [],
    'code_o': [[0, 0xfd80]], 'code_c': CODE_3DMAP,
    'vectors': [atvec(0, 0x1234, 1), atvec(1, -1, 0), atvec(3, 0x7fff, 0x80),
                atvec(7, 0, 0xff), atvec(-1, 0x1111, 2)],
})

# process3dg(lod,col,row): lodGridDim[5]={1024,256,64,16,8}; buffers patched
# with ramp values <=3 so sub-index parent<<4 stays within the 64B sub-bufs.
# oracle bufs: lod4 top @0x7f50(64) lod3 @0x6eae(256) lod2 @0x6c58(64)
#              lod1 @0x6852(64) lod0 @0x664c(64)   dims @0x5ea
# cand bufs:   0x539c        0x598c        0x5a90        0x5b04        0x5b56
#              dims @0x63d8
def p3dg_patches():
    p = [cell(None, 0x5ea, c(M, 'g_lodGridDim'), struct.pack('<8H', 1024, 256, 64, 16, 8,
                                              0, 0, 0))]
    for o_off, c_off, sz in ((0x7f50, c(M, 'g_topLodGrid'), 64),
                             (0x6eae, c(M, 'buf1_3dg'), 256),
                             (0x6c58, c(M, 'buf2_3dg'), 64),
                             (0x6852, c(M, 'buf3_3dg'), 64),
                             (0x664c, c(M, 'buf4_3dg'), 64)):
        p.append(cell(None, o_off, c_off, ramp(sz, 4)))
    return p

p3v = []
for lod, col_, r in [(4, 0, 0), (4, 5, 5), (4, 6, 6), (4, -3, -3), (4, 3, 7),
                  (3, 0, 0), (3, 15, 15), (3, 16, 0), (3, -1, 5),
                  (2, 0, 0), (2, 63, 63), (2, 64, 0), (2, 1, 2), (2, -4, 0),
                  (1, 0, 0), (1, 255, 255), (1, 100, 200),
                  (0, 0, 0), (0, 1023, 1023), (0, 517, 93),
                  (5, 0, 0), (6, 0, 0), (-1, 0, 0)]:
    p3v.append({'args': [lod, col_, r]})
e3d.append({
    'fn': 'process3dg', 'oracle_exe': EGAME_O, 'oracle_map': EGAME_OM,
    'oracle_off': 0x988, 'oracle_dgrp': EGAME_DGRP,
    'cand_exe': EG3DMAP, 'cand_map': EG3DMAP_M, 'cand_dgrp': DG_3DMAP,
    'ds': '0x6000',
    'patches': p3dg_patches(),
    'code_o': [[0, 0xfd80]], 'code_c': CODE_3DMAP,
    'vectors': p3v,
})

# ------------------------------------------------------ EGAME mission clock
# formatMissionClock(time): time += tick; nameBuf = ":" + f2(t/0x708) + ":" +
# f2(t/0x1e) + ":" + f2(t<<1); nameBuf[0] += nightMode+1
# oracle: tick w@0x65c0 night byte@0x4ef4 nameBuf@0x65c6 (own consts via ds)
# cand cells by symbol; const ''/':'/':'/'0' pulled via the literal args the
# cand pushes to strcpy/strcat — both move with test-exe relinks
fmv = []
for t, tick, night in [(0, 0, 0), (0, 0, 1), (1, 0, 0), (30, 0, 0),
                       (60, 0, 0), (0x708, 0, 0), (0x70e, 0, 0),
                       (0x800, 0, 0), (0x1000, 0, 1), (0x7fff, 0, 0),
                       (0xffff, 0, 0), (0, 0x800, 0), (0x4000, 0x4000, 1),
                       (5, -1, 0), (0, 0, 2), (0x5555, 0x111, 1)]:
    fmv.append({'args': [t],
                'patches': [cell(None, 0x65c0, c(EGUI_M, 'g_missionTick'), w16(tick)),
                            cell(None, 0x4ef4, c(EGUI_M, 'g_nightMode'), bytes([night]))]})
clock = [{
    'fn': 'formatMissionClock', 'oracle_exe': EGAME_O, 'oracle_map': EGAME_OM,
    'oracle_off': 0x9c83, 'oracle_dgrp': EGAME_DGRP,
    'cand_exe': EGUI, 'cand_map': EGUI_M, 'cand_dgrp': DG_EGUI,
    'ds': '0x2ecf',
    'check_regs': [],
    'patches': [{'c_off': hex(arg_lit_off('EGUI', 'formatMissionClock',
                                         'strcpy', 0, 1)), 'size': 1},  # ''
                {'c_off': hex(arg_lit_off('EGUI', 'formatMissionClock',
                                          'strcat', 0, 1)), 'size': 2}, # ':'
                {'c_off': hex(arg_lit_off('EGUI', 'formatMissionClock',
                                          'strcat', 1, 1)), 'size': 2}, # ':'
                {'c_off': hex(arg_lit_off('EGUI', 'formatTwoDigit',
                                          'strcat', 0, 1)), 'size': 2}],# '0'
    'observe': [{'o_off': '0x65c6', 'c_off': hex(c(EGUI_M, 'g_nameBuf')),
                 'size': 10}],
    'code_o': [[0, 0xfd80]], 'code_c': CODE_EGUI,
    'vectors': fmv,
}]

# -------------------------------------- scaleCoordToLod oracle-only harvest
# asm-only in the port -> candidate = oracle image itself (self-compare just
# verifies the harness; the harvested ax:dx get pinned into the native test).
scl = [{
    'fn': 'scaleCoordToLod', 'oracle_exe': EGAME_O, 'oracle_map': EGAME_OM,
    'oracle_off': 0x906, 'oracle_dgrp': EGAME_DGRP,
    'cand_exe': EGAME_O, 'cand_map': EGAME_OM, 'cand_dgrp': EGAME_DGRP,
    'cand_off': 0x906,
    'ds': '0x2ecf',
    'check_regs': ['ax', 'dx'],
    'code_o': [[0, 0xfd80]], 'code_c': [[0, 0xfd80]],
    'vectors': [
        {'args': [lod, v & 0xffff, (v >> 16) & 0xffff]}
        for lod in (0, 1, 2, 3, 4, 5, -1)
        for v in (0, 1, 2, 7, 8, 0x1f, 0x20, 0x3f, 0x40, 0xff, 0x100,
                  0x3ff, 0x400, 0xfff, 0x1000, 0xffff, 0x10000, 0x7fff0000,
                  0xffff0000, 0xffffffff, 0x12345678)
    ],
}]

# randomRange(maxVal): rand LCG state — oracle dword@0x622c, cand CRT cell
# derived from _rand's disasm (moves with every test-exe relink)
RR_SEED = rand_seed_cell('EGMATH')
rrv = []
for seed in (0, 1, 0x12345678, 0xffffffff, 0x7fffffff):
    for mx in (0, 1, 2, 45, 100, 0x400, 0x4000, 0x7fff, -1, -32768):
        rrv.append({'args': [mx],
                    'patches': [cell(None, 0x622c, RR_SEED,
                                     struct.pack('<I', seed))]})
math2 = [{
    'fn': 'randomRange', 'oracle_exe': EGAME_O, 'oracle_map': EGAME_OM,
    'oracle_off': 0xd34b, 'oracle_dgrp': EGAME_DGRP,
    'cand_exe': 'build/EGMATH.EXE', 'cand_map': 'build/EGMATH.MAP',
    'cand_dgrp': DG_EGMATH,
    'ds': '0x2ecf',
    'observe': [{'o_off': '0x622c', 'c_off': hex(RR_SEED), 'size': 4}],
    'code_o': [[0, 0xfd80]], 'code_c': CODE_EGMATH,
    'vectors': rrv,
}, {
    'fn': 'signOf', 'oracle_exe': EGAME_O, 'oracle_map': EGAME_OM,
    'oracle_off': 0xd316, 'oracle_dgrp': EGAME_DGRP,
    'cand_exe': 'build/EGMATH.EXE', 'cand_map': 'build/EGMATH.MAP',
    'cand_dgrp': DG_EGMATH,
    'ds': '0x6000',
    'code_o': [[0, 0xfd80]], 'code_c': CODE_EGMATH,
    'vectors': [[v] for v in (0, 1, -1, 5, -5, 0x7fff, -32768, 100, -300)],
}, {
    'fn': 'signExtendByte', 'oracle_exe': EGAME_O, 'oracle_map': EGAME_OM,
    'oracle_off': 0xd2f9, 'oracle_dgrp': EGAME_DGRP,
    'cand_exe': 'build/EGMATH.EXE', 'cand_map': 'build/EGMATH.MAP',
    'cand_dgrp': DG_EGMATH,
    'ds': '0x6000',
    'code_o': [[0, 0xfd80]], 'code_c': CODE_EGMATH,
    'vectors': [[v] for v in (0, 1, 0x7f, 0x80, 0x81, 0xfe, 0xff, -1, -256,
                              0x100, 0x1ff, -32768, 0x55)],
}]

base = os.path.join(ROOT, 'dosunit')
for name, spec in (('start_util', start), ('egame_3d', e3d),
                   ('egame_clock', clock), ('egame_scalelod', scl),
                   ('egame_math2', math2)):
    path = os.path.join(base, name + '.json')
    json.dump(spec, open(path, 'w'), indent=1)
    nv = sum(len(c['vectors']) for c in spec)
    print(f'{name}: {len(spec)} fns, {nv} vectors')
