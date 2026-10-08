#!/usr/bin/env python3
"""Dedicated specs for routines the generic probe only partially covers.

The generic 5-vector sweep seeds DS pulls from each side's own image
bytes — fine for pure computation, wrong where a routine's behavior
derives from seeded far pointers, DOS service results, or calls into
regions the probe cannot run.  Each case below replaces the asymmetric
input with a modeled one:

  - allocBuffer: `call dos_alloc` patched to `mov ax,imm` on BOTH sides —
    success path returns a real segment (0x4000 window), failure path
    (imm<0x10) runs with its cleanup/print/exit calls nopped so the
    post-exit fallthrough is compared, not DOS process teardown.
  - drawStringCentered (START alloc+zerofill twin): same dos_alloc patch
    aimed at the scratch DS; the memset lands at ds:0 and is observed.
  - drawStringCentered (EGAME record-set + driver draw): resident-driver
    lcall stubbed (cand thunk already returns ax=0); seeded ASCIIZ at the
    string arg so strlen/strupr are bounded; rec fields + uppercased
    string observed.
  - drawClippedLineEx / drawClippedLineRegion: every in-graph lcall
    stubbed `zero` on the oracle (the cand build's thunk bodies already
    return ax=0) and the near clip-line callee stubbed too — its divide
    is fed by driver-call results that do not exist under the sandbox.
    The routine's own window/store computation is observed via the
    shared-DS pairing.
  - drawGaugeBar: the two lcalls inside callee fillRectBoth stubbed on
    the oracle (the real driver dispatcher writes into the image's
    runtime ljmp-slot table -> instruction_memory_write).  Window-record
    pointers seeded at fixed scratch cells so the callee's `[bx+4]`
    stores become observable.
  - drawStoreIcons: oracle lcalls stubbed + the drawString near calls
    nopped — their targets are unpopulated overlay space in the oracle
    image (zeros).  The `es:[bx+si+0x38]` store-records are pointed at a
    seeded scratch window covering both loop arms (kind==0x13 skips).
    Two cases: first lcall -> 0 vs -> 1 selects the byte_98E6 branch.
  - loadPicFromFileAt: `call showPicFile` nopped on both sides — the PIC
    decoder is hand-asm skeleton keyed to real file data; openFile and
    closeFile run real code through int_stub/dsss_stub so the wrapper's
    handle marshaling is still exercised.

Cand-side stub sites are resolved from the LINK map and disassembled
fresh each run — post-relink regeneration is mandatory, same rule as
gen_probe.
"""
import os
import struct
import sys

import capstone

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, 'tools'))
sys.path.insert(0, os.path.join(ROOT, 'dosunit'))
from duspec import emit, cand_dgrp, cand_off, EXE       # noqa: E402
from gen_edge import mz_img, oracle_extent, cand_extent, case_for  # noqa: E402
from gen_probe import find_ints, pick_ds               # noqa: E402

MD = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_16)


def call_patch(size, form):
    """Replacement bytes for a stubbed call site (gen_probe model)."""
    if form == 'void':
        return b'\x90' * size
    if form == 'zero':
        return b'\x31\xc0' + b'\x90' * (size - 2)
    if form[0] == 'const':
        return b'\xb8' + form[1].to_bytes(2, 'little') + b'\x90' * (size - 3)
    if form[0] == 'param':
        return b'\x8b\xdc\x8b\x07\x90' if size == 5 else b'\x58\x90\x90'
    return None


def call_sites(img, hdr, off, end):
    """(site, target, size) for every near-call (E8) / lcall (9A) site
    reachable from the routine — find_ints' BFS discipline: near calls,
    jumps and lcall far immediates are followed; each visited address is
    decoded once (iseen) so overlapping callee windows cannot burn the
    budget."""
    imax = len(img) - hdr
    seen, work, budget = set(), [(off, end)], 40000
    iseen, out = set(), []
    while work and budget > 0:
        cur, cend = work.pop()
        if cur in seen or not (0 <= cur < imax):
            continue
        seen.add(cur)
        for i in MD.disasm(img[hdr + cur:hdr + min(cend, imax)], cur):
            if i.address in iseen:
                break
            iseen.add(i.address)
            budget -= 1
            if not budget or i.mnemonic in ('ret', 'retf', 'iret'):
                break
            b = i.bytes
            if not b:
                continue
            if b[0] == 0xE8:
                disp = int.from_bytes(b[1:3], 'little', signed=True)
                tgt = (i.address + 3 + disp) & 0xFFFF
                out.append((i.address, tgt, 3))
                if 0 <= tgt < imax:
                    work.append((tgt, min(tgt + 0x1800, imax)))
            elif b[0] in (0x9A, 0xEA):
                tgt = (int.from_bytes(b[3:5], 'little') * 16
                       + int.from_bytes(b[1:3], 'little'))
                if b[0] == 0x9A:
                    out.append((i.address, tgt, 5))
                if 0 <= tgt < imax:
                    work.append((tgt, min(tgt + 0x1800, imax)))
            elif b[0] in (0xE9, 0xEB):
                n = 3 if b[0] == 0xE9 else 2
                disp = int.from_bytes(b[1:n], 'little', signed=True)
                tgt = (i.address + n + disp) & 0xFFFF
                if 0 <= tgt < imax:
                    work.append((tgt, min(tgt + 0x1800, imax)))
    return out


def extent_of(exe_key, name):
    oexe, omap, odgrp = EXE[exe_key]
    oimg, ohdr = mz_img(oexe)
    f, e, kind = oracle_extent(name, os.path.join(ROOT, omap))
    assert f is not None, name
    return oexe, omap, odgrp, oimg, ohdr, f, e


def cand_img(mod):
    cexe = os.path.join(ROOT, 'build/%s.EXE' % mod)
    cmap = os.path.join(ROOT, 'build/%s.MAP' % mod)
    cimg, chdr = mz_img(cexe)
    cdgrp = cand_dgrp(cmap)
    return cexe, cmap, cimg, chdr, cdgrp


def cand_ext(cmap, cimg, name, cdgrp):
    f, e = cand_extent(cmap, name, cdgrp)
    assert f is not None, name
    return f, e


def int_dsss(img, hdr, off, end):
    """(int_sites, dsss_sites) reachable from the routine — thin wrapper
    over find_ints' collector for the dedicated spec."""
    ds = []
    return find_ints(img, hdr, off, end, dsss=ds), ds


def sites_to(img, hdr, off, end, target):
    """Near-call sites whose rel16 target is `target`."""
    return [s for s, t, z in call_sites(img, hdr, off, end)
            if z == 3 and t == target]


def lcall_sites(img, hdr, off, end):
    return [(s, t) for s, t, z in call_sites(img, hdr, off, end) if z == 5]


def map_entry(omap, name):
    """name -> image offset of a routine entry in an oracle map."""
    import re
    for line in open(omap, errors='replace'):
        m = re.match(r'(\w+): \w+ (?:NEAR|FAR) ([0-9a-f]+)-',
                     line.strip())
        if m and m.group(1) == name:
            return int(m.group(2), 16)
    return None


CASES = []


def allocbuffer(exe_key, mod, seg_model):
    """`call dos_alloc` -> `mov ax,<seg>` on both sides; the <0x10 error
    arm gets cleanup/dos_printstring/exit calls nopped so the
    post-exit fallthrough (return stored seg) is compared."""
    oexe, omap, odgrp, oimg, ohdr, f, e = extent_of(exe_key, 'allocBuffer')
    cexe, cmap, cimg, chdr, cdgrp = cand_img(mod)
    cf, ce = cand_ext(cmap, cimg, 'allocBuffer', cdgrp)
    oda = map_entry(os.path.join(ROOT, omap), 'dos_alloc')
    cda = cand_off(cmap, 'dos_alloc')
    ds = pick_ds(oexe, ohdr, cexe, chdr)
    dsi = int(ds, 16)
    out = []
    for tag, seg in (('ok', dsi), ('err', 8)):
        ostub = [(s, call_patch(3, ('const', seg)))
                 for s in sites_to(oimg, ohdr, f, e, oda)]
        cstub = [(s, call_patch(3, ('const', seg)))
                 for s in sites_to(cimg, chdr, cf, ce, cda)]
        if tag == 'err':
            keep = {s for s, p in ostub}
            ostub += [(s, call_patch(3, 'void')) for s, t, z in
                      call_sites(oimg, ohdr, f, e) if z == 3 and s not in keep]
            keep = {s for s, p in cstub}
            cstub += [(s, call_patch(3, 'void')) for s, t, z in
                      call_sites(cimg, chdr, cf, ce) if z == 3 and s not in keep]
        case = {'fn': 'allocBuffer', 'exe': exe_key, 'mod': mod,
                'ds': ds, 'vectors': [[0x10], [0x40], [0x400]],
                'regs': ['ax'], 'call_stub': ostub, 'call_stub_c': cstub}
        if tag == 'err':
            # keep the pair distinct so both fixtures get emitted
            case['oof'], case['cof'] = f, cf
        out.append(case)
    return out


def drawstringcentered_start(mod='STGEN'):
    """START's drawStringCentered = allocBuffer(n) + far memset(seg:0,n).
    Force dos_alloc -> scratch DS on both sides; observe the zeroed
    head of the window."""
    oexe, omap, odgrp, oimg, ohdr, f, e = extent_of(
        'START', 'drawStringCentered')
    cexe, cmap, cimg, chdr, cdgrp = cand_img(mod)
    cf, ce = cand_ext(cmap, cimg, 'drawStringCentered', cdgrp)
    oda = map_entry(os.path.join(ROOT, omap), 'dos_alloc')
    cda = cand_off(cmap, 'dos_alloc')
    ds = pick_ds(oexe, ohdr, cexe, chdr)
    dsi = int(ds, 16)
    ostub = [(s, call_patch(3, ('const', dsi)))
             for s in sites_to(oimg, ohdr, f, e, oda)]
    cstub = [(s, call_patch(3, ('const', dsi)))
             for s in sites_to(cimg, chdr, cf, ce, cda)]
    obs = [(0, 0, 2), (2, 2, 2), (4, 4, 2), (6, 6, 2),
           (8, 8, 2), (0xA, 0xA, 2), (0xC, 0xC, 2), (0xE, 0xE, 2)]
    return [{'fn': 'drawStringCentered', 'exe': 'START', 'mod': mod,
             'ds': ds, 'vectors': [[0x10], [0x40], [0x100]],
             'regs': ['ax'], 'call_stub': ostub, 'call_stub_c': cstub,
             'obs': obs}]


def drawstringcentered_egame(mod='EGFRAME'):
    """EGAME's drawStringCentered (Code1:90fa) is a different routine —
    rec field stores + strlen + strupr + a resident-driver draw lcall.
    Stub the lcall, seed an ASCIIZ at the string arg, observe the rec
    fields and the uppercased string."""
    oexe, omap, odgrp, oimg, ohdr, f, e = extent_of(
        'EGAME', 'drawStringCentered')
    cexe, cmap, cimg, chdr, cdgrp = cand_img(mod)
    cf, ce = cand_ext(cmap, cimg, 'drawStringCentered', cdgrp)
    ds = pick_ds(oexe, ohdr, cexe, chdr)
    ostub = [(s, call_patch(5, 'zero'))
             for s, t in lcall_sites(oimg, ohdr, f, e)]
    cstub = [(s, call_patch(5, 'zero'))
             for s, t in lcall_sites(cimg, chdr, cf, ce)]
    REC, STR = 0x200, 0x240
    vecs = []
    for v8, vA, vC, txt in ((0x11, 0x22, 0x33, b'f19-test\x00'),
                            (0, 0x63, 0x3f, b'hello\x00'),
                            (0xffff, 1, 0x7f, b'A\x00')):
        vecs.append({'args': [REC, STR, v8, vA, vC],
                     'patch': [{'seg': ds, 'off': hex(STR),
                                'bytes': (txt + b'\x00' * 12).hex()},
                               {'seg': ds, 'off': hex(REC),
                                'bytes': (b'\x77' * 16).hex()}]})
    obs = [(REC + k, REC + k, 2) for k in (4, 8, 0xA, 0xC)]
    obs += [(STR + k, STR + k, 2) for k in range(0, 12, 2)]
    return [{'fn': 'drawStringCentered', 'exe': 'EGAME', 'mod': mod,
             'ds': ds, 'vectors': vecs, 'regs': ['ax'],
             'call_stub': ostub, 'call_stub_c': cstub, 'obs': obs}]


def clip_line(fn, exe_key, mod, extra_vecs=()):
    """Clip-window setup routines: driver lcalls + the near clip callee
    are all sandbox-dead (driver returns feed the divisor); stub every
    in-graph call to `zero`/`void` and compare the window-cell stores."""
    oexe, omap, odgrp, oimg, ohdr, f, e = extent_of(exe_key, fn)
    cexe, cmap, cimg, chdr, cdgrp = cand_img(mod)
    cf, ce = cand_ext(cmap, cimg, fn, cdgrp)
    ds = pick_ds(oexe, ohdr, cexe, chdr)
    ostub = [(s, call_patch(z, 'zero'))
             for s, t, z in call_sites(oimg, ohdr, f, e)]
    cstub = [(s, call_patch(z, 'zero'))
             for s, t, z in call_sites(cimg, chdr, cf, ce)]
    return dict(fn=fn, exe=exe_key, omap=omap, mod=mod, ds=ds,
                vectors=None, obs=None, _stub=(ostub, cstub),
                extra_vecs=extra_vecs)


def gaugebar():
    """fillRectBoth's driver lcalls patched on the oracle; the record
    pointers [0x57b0]/[0x57c8] are seeded at fixed scratch cells so the
    callee's [bx+4] stores are observed; [0x4efa] gates the draw path
    per-vector."""
    name = 'drawGaugeBar'
    cfg = dict(fn=name, exe='EGAME', omap='map/egame_en.map',
               mod='EGFRAME', ds=None, deep=True, vectors=None)
    oexe, omap, odgrp, oimg, ohdr, f, e = extent_of('EGAME', name)
    cexe, cmap, cimg, chdr, cdgrp = cand_img(cfg['mod'])
    cf, ce = cand_ext(cmap, cimg, name, cdgrp)
    ds = pick_ds(oexe, ohdr, cexe, chdr)
    cfg['ds'] = ds
    ostub = [(s, call_patch(5, 'zero'))
             for s, t in lcall_sites(oimg, ohdr, f, e)]
    # pair the record-pointer cells: oracle 0x57b0/0x57c8 read in the
    # callees; the cand partners are found through refs() pairing inside
    # case_for — but extra_patch needs explicit offsets, so locate them
    # by scanning the cand callees for their `mov bx,[imm]` loads.
    crd = []
    for i in MD.disasm(cimg[chdr + cf:chdr + ce], cf):
        pass
    # cand mirror cells: the cand's fn8e7d twin loads [0x1604]/[0x1606]
    # (verified by disasm) — resolve positionally: oracle [0x57b0] pairs
    # with the first `mov bx,[imm]` inside the cand's first callee.
    cg1 = cg2 = None
    # first callee = fn8e7d twin, second = fillRectBoth twin
    csites = call_sites(cimg, chdr, cf, ce)
    near = [t for s, t, z in csites if z == 3]
    if len(near) >= 2:
        for i in MD.disasm(cimg[chdr + near[0]:chdr + near[0] + 0x40],
                           near[0]):
            if i.mnemonic == 'mov' and 'bx, word ptr [0x' in i.op_str:
                cell = int(i.op_str.split('[')[1].rstrip(']'), 16)
                if cg1 is None:
                    cg1 = cell
                elif cg2 is None:
                    cg2 = cell
                    break
    extra = []
    if cg1 is not None and cg2 is not None:
        extra = [(0x57B0, cg1, 2, '0001'), (0x57C8, cg2, 2, '0002')]
    extra_obs = [(0x104, 0x104, 2), (0x204, 0x204, 2)]
    cfg['vectors'] = []
    for val, gate in ((0x40, 1), (-0x30 & 0xFFFF, 1), (0, 1), (0x7F, 0)):
        v = {'args': [val, 1, 0, 0xB]}
        v['patch'] = [(0x4EFA, None, 2, '0100' if gate else '0000'),
                      (cg1 if False else 0, 0, 0, None)][0:1]
        # gate flag pairs oracle 0x4efa <-> cand 0x1602
        v['patch'] = [(0x4EFA, 0x1602, 2, '0100' if gate else '0000')]
        cfg['vectors'].append(v)
    c = case_for(name, cfg, oexe, odgrp, oimg, ohdr,
                 cmap, cimg, chdr, cdgrp, tuple(extra), tuple(extra_obs))
    c['call_stub'] = ostub
    return c


def storeicons():
    name = 'drawStoreIcons'
    cfg = dict(fn=name, exe='START', omap='map/start_en.map',
               mod='STGEN', ds=None, deep=True, vectors=None)
    oexe, omap, odgrp, oimg, ohdr, f, e = extent_of('START', name)
    cexe, cmap, cimg, chdr, cdgrp = cand_img(cfg['mod'])
    cf, ce = cand_ext(cmap, cimg, name, cdgrp)
    ds = pick_ds(oexe, ohdr, cexe, chdr)
    cfg['ds'] = ds
    dsi = int(ds, 16)
    olc = lcall_sites(oimg, ohdr, f, e)
    clc = lcall_sites(cimg, chdr, cf, ce)
    # es:[bx+si+0x38] reads a store-record table through the far ptr at
    # [0xd066] (cand [0x6a00]): point it at ds:0x40 and seed 4 records so
    # indices 0,2 take the 0x13 skip and 1,3 take the draw arm.
    segbytes = struct.pack('<HH', 0x40, dsi)
    tab = bytearray(0x50)
    struct.pack_into('<H', tab, 0x20, 0x55)          # es:[bx+0x20] arg
    for i, v in enumerate((0x13, 0x27, 0x13, 0x42)):
        struct.pack_into('<H', tab, 0x38 + i * 2, v)
    extra = [(0xD066, 0x6A00, 4, segbytes.hex()),
             (0x40, 0x40, 0x50, bytes(tab).hex())]
    out = []
    for tag, gatev in (('lo', 0), ('hi', 1)):
        ostub = [(s, call_patch(5, 'zero')) for s, t in olc]
        cstub = []
        # near calls into unpopulated overlay space (oracle) — nop;
        # cand's drawStringAt/drawStringFar ports run for real.
        ostub += [(s, call_patch(3, 'void')) for s, t, z in
                  call_sites(oimg, ohdr, f, e) if z == 3]
        if gatev:
            # flip the first lcall's answer so the byte_98E6!=0 arm runs
            ostub[0] = (ostub[0][0], call_patch(5, ('const', gatev)))
            if clc:
                cstub = [(clc[0][0], call_patch(5, ('const', gatev)))]
        cfg['vectors'] = [[0, 0, 0, 0]] * 3
        c = case_for(name, cfg, oexe, odgrp, oimg, ohdr,
                     cmap, cimg, chdr, cdgrp, tuple(extra), ())
        c['call_stub'] = ostub
        if cstub:
            c['call_stub_c'] = cstub
        if tag == 'hi':
            c['oof'], c['cof'] = f, cf
        out.append(c)
    return out


def loadpic():
    name = 'loadPicFromFileAt'
    oexe, omap, odgrp, oimg, ohdr, f, e = extent_of('END', name)
    cexe, cmap, cimg, chdr, cdgrp = cand_img('ENBRIEF')
    cf, ce = cand_ext(cmap, cimg, name, cdgrp)
    ds = pick_ds(oexe, ohdr, cexe, chdr)
    # showPicFile = hand-asm PIC decoder looping over seeded bytes —
    # nop on both sides; keep openFile/closeFile real via int/dsss stubs.
    oshow = map_entry(os.path.join(ROOT, omap), 'showPicFile')
    cshow = cand_off(cmap, 'showPicFile')
    ostub = [(s, call_patch(3, 'void'))
             for s in sites_to(oimg, ohdr, f, e, oshow)]
    cstub = [(s, call_patch(3, 'void'))
             for s in sites_to(cimg, chdr, cf, ce, cshow)]
    ints_o, dsss_o = int_dsss(oimg, ohdr, f, e)
    ints_c, dsss_c = int_dsss(cimg, chdr, cf, ce)
    case = {'fn': name, 'exe': 'END', 'mod': 'ENBRIEF', 'ds': ds,
            'vectors': [[0x2000, 0x40, 0x10], [0x800, 0x60, 0x20]],
            'regs': ['ax'], 'call_stub': ostub, 'call_stub_c': cstub}
    if ints_o:
        case['int_stub'] = ints_o
    if ints_c:
        case['int_stub_c'] = ints_c
    if dsss_o:
        case['dsss_stub'] = dsss_o
    if dsss_c:
        case['dsss_stub_c'] = dsss_c
    return [case]


def main():
    cases = []
    cases += allocbuffer('END', 'ENBRIEF', None)
    cases += allocbuffer('START', 'STGEN', None)
    cases += drawstringcentered_start()
    cases += drawstringcentered_egame()
    for fn, exe_key, mod in (('drawClippedLineEx', 'END', 'ENBRIEF'),
                             ('drawClippedLineRegion', 'EGAME', 'EGFRAME')):
        spec = clip_line(fn, exe_key, mod)
        ostub, cstub = spec.pop('_stub')
        n = 9 if fn == 'drawClippedLineRegion' else 8
        vecs = [[0x10, 0x20, 0x60, 0x50, 0, 0, 0x40, 0x30, 0][:n],
                [0x30, 0x30, 0x30, 0x60, 5, 5, 0x10, 0x10, 1][:n],
                [0, 0, 0x7F, 0x7F, 9, 9, 0x60, 0x40, 0][:n]]
        cfg = dict(fn=fn, exe=exe_key, omap=spec['omap'], mod=mod,
                   ds=spec['ds'], deep=False, vectors=vecs,
                   span_obs=True)
        oexe, omap, odgrp, oimg, ohdr, f, e = extent_of(exe_key, fn)
        cexe, cmap, cimg, chdr, cdgrp = cand_img(mod)
        c = case_for(fn, cfg, oexe, odgrp, oimg, ohdr,
                     cmap, cimg, chdr, cdgrp)
        c['call_stub'] = ostub
        c['call_stub_c'] = cstub
        cases.append(c)
    cases.append(gaugebar())
    cases += storeicons()
    cases += loadpic()
    emit(cases, os.path.join(ROOT, 'dosunit/partial.json'))


if __name__ == '__main__':
    main()
