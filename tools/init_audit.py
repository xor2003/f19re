#!/usr/bin/env python3
"""Audit: positionally pair direct [imm] operands oracle<->cand across all
probe suites and compare each side's image bytes.  Cells where the oracle
has initialized data but the candidate has BSS-zero (or different bytes)
are init-data fidelity gaps in the port (stubs.c globals/tables)."""
import capstone, json, os, re, struct, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, 'tools'))
from duspec import EXE, cand_dgrp

SUITES = {
    'egame': ('build/EGFRAME.EXE', 'build/EGFRAME.MAP', 'probe_egame_%d.json', 3),
    'start': ('build/STGEN.EXE',   'build/STGEN.MAP',   'probe_start_%d.json', 3),
    'end':   ('build/ENBRIEF.EXE', 'build/ENBRIEF.MAP', 'probe_end_%d.json',   3),
    'su':    ('build/SUUTIL.EXE',  'build/SUUTIL.MAP',  'probe_su_%d.json',    4),
}

ABS_RE = re.compile(r'(?:word|byte|dword) ptr (?:(ds|es|cs|ss|fs|gs):)?\['
                    r'(?:(bx|si|di|bp)\s*([+-]))?\s*'
                    r'(0x[0-9a-fA-F]+|[0-9a-fA-F]+h?)\]')


def mz(path):
    d = open(path, 'rb').read()
    return d, struct.unpack('<H', d[8:10])[0] * 16


def extent(cmap_lines, name, dgrp):
    pat = re.compile(r'([0-9A-Fa-f]+):([0-9A-Fa-f]+)\s+(\S+)')
    seg_offs, tgt = {}, None
    for line in cmap_lines:
        m = pat.search(line)
        if not m:
            continue
        seg, off, nm = int(m.group(1), 16), int(m.group(2), 16), m.group(3)
        if seg * 16 + off >= dgrp:
            continue
        seg_offs.setdefault(seg, set()).add(off)
        if nm.lstrip('_') == name:
            tgt = (seg, off)
    if tgt is None:
        return None, None
    end = min((o for o in seg_offs[tgt[0]] if o > tgt[1]), default=None)
    return tgt[1], end


def direct_imms(img, off, end):
    """Ordered direct [imm] memory operands (both reads and writes),
    plus indexed-base immediates marked for span comparison."""
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_16)
    span = (end - off) if end else 0x400
    out = []
    for i in md.disasm(img[off:off + span], off):
        for op in i.op_str.split(','):
            m = ABS_RE.search(op)
            if not m:
                continue
            seg, reg, sgn, imm_s = m.groups()
            if seg in ('ss',) or reg == 'bp':
                continue
            imm = int(imm_s.rstrip('h'), 16)
            if sgn == '-':
                imm = (-imm) & 0xFFFF
            out.append((i.address, imm, bool(reg)))
    return out


def main():
    gaps = {}
    for suite, (cexe, cmapf, specp, nshards) in SUITES.items():
        oexe, omap, odgrp = EXE[suite.upper()]
        odata, oh = mz(oexe)
        oimg = odata[oh:]
        cdata, ch = mz(os.path.join(ROOT, cexe))
        cimg = cdata[ch:]
        cdgrp = cand_dgrp(os.path.join(ROOT, cmapf))
        omap_lines = open(os.path.join(ROOT, omap), errors='replace').readlines()
        segbase = {}
        for l in omap_lines:
            m = re.match(r'([\w.$]+)\s+CODE\s+([0-9a-fA-F]+)', l.strip())
            if m:
                segbase[m.group(1)] = int(m.group(2), 16) * 16
        oext = {}
        for l in omap_lines:
            m = re.match(r'(\S+):\s+([\w.$]+)\s+NEAR\s+([0-9a-fA-F]+)-([0-9a-fA-F]+)',
                         l.strip())
            if m and m.group(2) in segbase:
                oext[m.group(1)] = (segbase[m.group(2)] + int(m.group(3), 16),
                                    segbase[m.group(2)] + int(m.group(4), 16))
        cmap_lines = open(os.path.join(ROOT, cmapf), errors='replace').readlines()
        seen = set()
        for sh in range(nshards):
            p = os.path.join(ROOT, 'dosunit', specp % sh)
            if not os.path.exists(p):
                continue
            for case in json.load(open(p)):
                fn = case['fn']
                if fn in seen or fn not in oext:
                    continue
                seen.add(fn)
                fo, eo = oext[fn]
                fc, ec = extent(cmap_lines, fn, cdgrp)
                if fc is None:
                    continue
                o_imms = direct_imms(oimg, fo, eo)
                c_imms = direct_imms(cimg, fc, ec)
                for (oa, oi, oi_ix), (ca, ci, ci_ix) in zip(o_imms, c_imms):
                    if oi_ix != ci_ix:
                        continue
                    o_lin, c_lin = odgrp + oi, cdgrp + ci
                    o_in = o_lin + 2 <= len(oimg)
                    c_in = c_lin + 2 <= len(cimg)
                    ob = oimg[o_lin:o_lin + 2] if o_in else b'\x00\x00'
                    cb = cimg[c_lin:c_lin + 2] if c_in else b'\x00\x00'
                    if ob != cb and (o_in or c_in):
                        gaps.setdefault((suite, oi, ci), []).append(
                            (fn, hex(oa), hex(ca), ob.hex(),
                             cb.hex() if c_in else 'BSS',
                             'ix' if oi_ix else 'dir'))
    rows = sorted(gaps.items(), key=lambda kv: (kv[0][0], kv[0][1]))
    print(f'{len(rows)} init-data mismatches')
    for (suite, oi, ci), uses in rows:
        fn, oa, ca, ob, cb, kind = uses[0]
        print(f'{suite:5} ds:{oi:#06x} vs cand:{ci:#06x}  oracle={ob} cand={cb}'
              f'  [{kind}] ({fn} @{oa}/{ca}, {len(uses)} refs)')


if __name__ == '__main__':
    main()
