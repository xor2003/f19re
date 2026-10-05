#!/usr/bin/env python3
# Generates dosunit/start_math.json — START.EXE math/format helpers vs
# build/STGEN.EXE.  Candidate cells resolve through STGEN.MAP or the
# routine's own disasm, so test-exe relinks never invalidate the spec.
import os, re, struct, sys
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
                + '/tools')
from duspec import emit, cand_off, arg_lit_off
import capstone

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
MOD = 'STGEN'
CMAP = os.path.join(ROOT, 'build/STGEN.MAP')
CEXE = os.path.join(ROOT, 'build/STGEN.EXE')
DS = '0x2000'


def w16(v):
    return struct.pack('<H', v & 0xFFFF).hex()


def lit(fn, callee='mystrcpy', nth=0, argn=1):
    """DS offset of the literal arg on the nth `call callee` inside fn."""
    return arg_lit_off(MOD, fn, callee, nth, argn)


def sin_lut():
    """Cand sine-LUT dseg offset: _sine tail-calls its interp helper, which
    indexes `mov ax, word ptr [si + LUT]`.  Deriving it keeps the LUT pull
    valid across relinks."""
    data = open(CEXE, 'rb').read()
    hdr = struct.unpack('<H', data[8:10])[0] * 16
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_16)
    off = cand_off(CMAP, 'sine')
    ins = list(md.disasm(data[hdr + off:hdr + off + 0x80], off))
    for i in ins:
        if i.mnemonic != 'call':
            continue
        helper = int(i.op_str, 16) & 0xFFFF
        for j in md.disasm(data[hdr + helper:hdr + helper + 0x80], helper):
            m = re.search(r'word ptr \[(?:si|bx) \+ (0x[0-9a-f]+)\]',
                          j.op_str)
            if m:
                return int(m.group(1), 16)
    raise AssertionError('sin LUT base not found')


LUT = sin_lut()

cases = [
    {'fn': 'calcBearing', 'exe': 'START', 'mod': MOD, 'ds': DS,
     'vectors': [[0, 0], [0, 5], [0, -5], [5, 0], [-5, 0],
                 [100, 100], [100, -100], [-100, 100], [-100, -100],
                 [300, 100], [100, 300], [-300, 100], [-100, 300],
                 [300, -100], [100, -300], [-300, -100], [-100, -300],
                 [7, 7], [1, 0], [0, 1], [32767, 32767], [-32768, -32768],
                 [32767, 1], [1, 32767], [16384, -16384]]},

    {'fn': 'rangeApprox', 'exe': 'START', 'mod': MOD, 'ds': DS,
     'vectors': [[0, 0], [1, 0], [0, 1], [1, 1], [-1, -1], [3, 4],
                 [-3, -4], [4, -3], [16384, 4096], [-1, 1],
                 [32767, 0], [0, 32767], [32767, 32767], [-32768, -32768],
                 [32767, -32768], [-32768, 32767], [4660, 22136],
                 [-300, -400]]},

    {'fn': 'clampValue', 'exe': 'START', 'mod': MOD, 'ds': DS,
     'vectors': [[5, 0, 10], [-1, 0, 10], [-16384, 0, 10], [0, 0, 0],
                 [11, 0, 10], [10, 0, 10], [0, 0, 10], [9, 0, 10],
                 [-32768, -32768, 32767], [32767, -32768, 32767],
                 [5, 5, 5], [5, 10, 0]]},

    # itemDistance(i,j): |worldObjects[i] - worldObjects[j]| via rangeApprox
    {'fn': 'itemDistance', 'exe': 'START', 'mod': MOD, 'ds': '0x6000',
     'patch': [(0xb390, 'worldObjects', None,
                '6400c800000000000000000000000000900158020000000000000000'
                '00000000ceff32000000000000000000000000000010002000000000'
                '0000000000000000000000000000000000000000000000080ff7f000'
                '000000000000000000000000ff7f0080000000000000000000000000'
                '07000700000000000000000000000000')],
     'vectors': [[0, 1], [1, 0], [0, 2], [3, 1], [2, 2], [4, 4],
                 [5, 6], [6, 5], [7, 0], [0, 7], [4, 5]]},

    # sinMul/cosMul: args -> ax; LUT pull keeps the interp table populated.
    # Oracle needs no pull: its loaded image tail overlaps the DS window, so
    # oracle DS:<dseg off> reads the image's own bytes for free (image base
    # 0x10000 == oracle dseg image offset).  Cand's dseg offsets differ, so
    # cand cells must be materialized explicitly.
    {'fn': 'sinMul', 'exe': 'START', 'mod': MOD, 'ds': DS,
     'patch': [(None, LUT, 516)],
     'vectors': [[0, 16384], [16384, 16384], [8192, 16384],
                 [5461, 16384], [10922, 16384], [2048, 16384],
                 [-16384, 16384], [10922, -8192], [10922, 4660],
                 [16383, 16384], [16385, 16384], [255, 16384],
                 [256, 16384], [16640, 16384], [32767, 16384],
                 [-1, 16384], [-32768, 16384], [0, 0], [16384, 0],
                 [16384, 32767], [16384, -32768], [21845, 32767],
                 [21845, -32768]]},

    {'fn': 'cosMul', 'exe': 'START', 'mod': MOD, 'ds': DS,
     'patch': [(None, LUT, 516)],
     'vectors': [[0, 16384], [16384, 16384], [-32768, 16384],
                 [5461, 4660], [2048, 4660], [-16384, 16384],
                 [32767, 16384], [-1, 16384], [16383, 16384],
                 [0, 0], [16384, 0], [10922, 32767]]},

    # formatGridRef(wx,wy): gameData->theater picks the 4-letter prefix,
    # then grid coords land in bufCoordStr.  gameData is a far ptr patched
    # to 7000:0 so theater reads the per-vector poke at 0x38.
    {'fn': 'formatGridRef', 'exe': 'START', 'mod': MOD, 'ds': DS,
     'regs': [],
     'patch': [(0x991c, 'gameData', None, '00000070'),
               (None, lit('formatGridRef'), 20)],    # "TD00 JZ00 WX00 CC00"
     'obs': [(0x98cc, 'bufCoordStr', 8)],
     'vectors': [{'args': [wx, wy, 0],
                  'patch': [{'seg': '0x7000', 'off': '0x38',
                             'bytes': w16(th)}]}
                 for th in range(4)
                 for wx, wy in [(0, 0), (0x800, 0x800), (0x4000, 0x4000),
                                (0xbf8, 0xbf8), (0xccc, 0), (0, 0xccc),
                                (0x7000, 0x7000), (0xffff, 0xffff),
                                (-0x800, -0x800), (0x1400, 0x600)]]},

    # formatTimeStr(buf,v): buf=ds:0xf000 (obs); "HH:MM" with flag bias
    # from missionTimeFlag; '00:00' literal pulled for both sides.
    {'fn': 'formatTimeStr', 'exe': 'START', 'mod': MOD, 'ds': DS,
     'regs': [],
     'patch': [(None, lit('formatTimeStr', argn=0), 6)],     # "00:00"
     'obs': [{'off': '0xf000', 'size': 8}],
     'vectors': [{'args': [0xf000, v],
                  'patch': [(0x44e4, 'missionTimeFlag', None, w16(fl))]}
                 for v, fl in [(0xF000, 0), (0xF000, 1), (59, 0), (60, 0),
                               (299, 0), (300, 0), (3599, 0), (3600, 0),
                               (4515, 0), (4515, 1), (86399, 0),
                               (86400, 0), (86400, 1), (32767, 0),
                               (-1, 0), (-1, 1)]]},
]

emit(cases, os.path.join(ROOT, 'dosunit/start_math.json'))
