#!/usr/bin/env python3
"""Edge-case dedicated specs for routines the generic probe can't cover:

  - BSS-table reads that are initialized on the oracle side (divisor faults:
    positionUnit, spawnEnemyAircraft, fireAirThreat) — seed identical bytes
    on both sides so indexed table reads agree and divisors stay nonzero.
  - Far-pointer destination artifacts (commFetch, memAppend) — point moveDst
    at a scratch window and observe the bytes movedata lands there.
  - Buffer-drain writes through an arg pointer (bufReadBytes, bufReadFile) —
    observe the dst window plus the shared read-pos cell.

Mechanics: disassemble the routine on both sides (oracle map extent, cand
next-distinct-symbol extent), pair every DS memory operand in instruction
order (byte-exact ports keep operand order), then:

  * non-store operand pairs -> span patch: same bytes written to both cells,
    oracle image bytes where in extent else a nonzero pattern.
  * store operand pairs -> span observations: byte-exact ports write the
    i-th store to the logical same cell; a span covers indexed stores at
    any vector index.

Span width SPAN covers idx*stride for idx<=3 at strides up to ~0x2a; vectors
keep small indices so reads/writes stay inside seeded spans.
"""
import json, os, re, struct, sys
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
                + '/tools')
from duspec import emit, cand_dgrp, EXE
import capstone

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SPAN = 0x200         # bytes covered per operand pair (idx*stride products)
WRITE_MN = {'mov', 'xchg', 'pop', 'inc', 'dec', 'neg', 'not',
            'add', 'sub', 'adc', 'sbb', 'and', 'or', 'xor',
            'shl', 'shr', 'sar', 'rol', 'ror', 'imul', 'mul'}
ABS_RE = re.compile(
    r'(?:word|byte|dword) ptr (?:(ds|es|cs|ss|fs|gs):)?\[([^\]]+)\]')
TOK_RE = re.compile(r'([+-])?\s*(bx|bp|si|di|0x[0-9a-fA-F]+|[0-9a-fA-F]+h?)')


def mz_img(path):
    d = open(path, 'rb').read()
    return d, struct.unpack('<H', d[8:10])[0] * 16


def oracle_extent(name, omap):
    for line in open(omap, errors='replace'):
        m = re.match(r'(\w+): \w+ (NEAR|FAR) ([0-9a-f]+)-([0-9a-f]+)',
                     line.strip())
        if m and m.group(1) == name:
            return int(m.group(3), 16), int(m.group(4), 16), m.group(2)
    return None, None, None


def cand_extent(map_path, name, dgrp):
    pat = re.compile(r'([0-9A-Fa-f]+):([0-9A-Fa-f]+)\s+_(\w+)\s*$')
    tgt_seg = tgt_off = None
    seg_offs = {}
    for line in open(map_path, errors='replace'):
        m = pat.search(line)
        if not m:
            continue
        seg, off, nm = int(m.group(1), 16), int(m.group(2), 16), m.group(3)
        if seg * 16 + off >= dgrp:
            continue
        seg_offs.setdefault(seg, set()).add(off)
        if nm == name:
            tgt_seg, tgt_off = seg, off
    if tgt_off is None:
        return None, None
    end = min((o for o in seg_offs[tgt_seg] if o > tgt_off), default=None)
    return tgt_off, end


def mem_ops(i):
    """(imm, is_store, indexed, size) for one insn's DS mem operands."""
    out = []
    for k, op in enumerate(p.strip() for p in i.op_str.split(',')):
        m = ABS_RE.search(op)
        if not m or (m.group(1) not in (None, 'ds')):
            continue                      # explicit non-DS prefix -> not DS
        # tokenize the bracketed expr: [bx + di - 0x7a04] etc.
        imm, idx, ss_rel = 0, False, False
        for sgn, tok in TOK_RE.findall(m.group(2)):
            if tok in ('bx', 'si', 'di'):
                idx = True
            elif tok == 'bp':
                ss_rel = True
            else:
                try:
                    v = int(tok.rstrip('h'), 16)
                except ValueError:
                    continue
                imm = imm - v if sgn == '-' else imm + v
        if ss_rel:
            continue
        size = 4 if 'dword' in op else 1 if 'byte' in op else 2
        out.append((imm & 0xFFFF, k == 0 and i.mnemonic in WRITE_MN,
                    idx, size))
    return out


def callee_refs(img, hdr, off, end, md):
    """mem refs of an inlined callee; stops at the first return."""
    out = []
    for i in md.disasm(img[hdr + off:hdr + off + (end - off)], off):
        out.extend(mem_ops(i))
        if i.mnemonic in ('ret', 'retf', 'int', 'iret'):
            break
    return out


def refs(img, hdr, off, end, deep=False):
    """(top_refs, {call_ordinal: callee_refs}) for DS mem operands.

    Near-call targets are inlined one level deep so callee table reads get
    seeded too.  Callee refs are keyed by call-site ordinal (byte-exact
    ports share the call sequence) so a stubbed-out callee on one side
    skips only its own segment instead of shifting the whole pairing.
    """
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_16)
    top, calls, seen = [], {}, set()
    imax = len(img) - hdr
    ncall = 0
    for i in md.disasm(img[hdr + off:hdr + off + (end - off)], off):
        top.extend(mem_ops(i))
        if deep and i.mnemonic == 'call' and i.bytes[0] == 0xE8:
            ncall += 1
            # near call rel16: compute the wrapped target ourselves
            disp = int.from_bytes(i.bytes[1:3], 'little', signed=True)
            tgt = (i.address + 3 + disp) & 0xFFFF
            if 0 <= tgt < imax and tgt not in seen:
                seen.add(tgt)
                calls[ncall] = callee_refs(img, hdr, tgt,
                                           min(tgt + 0x300, imax), md)
    return top, calls


def pattern(n):
    """Deterministic nonzero-ish test bytes (keeps divisors/ptrs benign)."""
    return bytes(((i * 7 + (i >> 3) + 3) & 0x7f) or 1 for i in range(n))


def case_for(name, cfg, oexe, odgrp, oimg, ohdr, cmap, cimg, chdr, cdgrp,
             extra_patch=(), extra_obs=()):
    f_o, e_o, kind = oracle_extent(name, os.path.join(ROOT, cfg['omap']))
    f_c, e_c = cand_extent(cmap, name, cdgrp)
    otop, ocalls = refs(oimg, ohdr, f_o, e_o, deep=cfg.get('deep'))
    ctop, ccalls = refs(cimg, chdr, f_c, e_c, deep=cfg.get('deep'))
    pairs = [(o, c, False) for o, c in zip(otop, ctop)]
    for k in sorted(set(ocalls) & set(ccalls)):
        pairs += [(o, c, True) for o, c in zip(ocalls[k], ccalls[k])]
    orf = [o for o, c, _ in pairs]
    crf = [c for o, c, _ in pairs]
    odata_end = len(oimg) - ohdr - odgrp
    # The replay shares ONE DS window between the guests: a patch entry at a
    # DS offset lands in BOTH runs, so the byte at each offset must satisfy
    # both sides. Collect per-byte demands with a precedence —
    #   4 explicit setup, 3 absolute cells, 2 indexed read spans,
    #   1 store-region zeroing —
    # then force obs-pair positions to a consensus byte so untouched obs
    # cells compare equal instead of leaking each side's own init data.
    dem = {}
    obs_pairs = []

    def put(off, byte, prio):
        dem.setdefault(off & 0xFFFF, []).append((prio, byte))

    obs = []
    obs_drop = set(cfg.get('obs_drop', ()))
    for (oimm, ost, oidx, osz), (cimm, cst, cidx, csz), deep in pairs:
        if ost != cst or oidx != cidx:
            continue                      # diverged shapes — skip pairing
        width = SPAN if oidx else osz
        if oimm in obs_drop:
            continue                      # e.g. stored value is a pointer
                                        # constant -> differs by layout
        if ost:
            # span_obs=False scopes observation to the routine's own
            # top-level scalar stores: indexed spans and inlined-callee
            # stores land in record/string tables whose written content
            # derives from each binary's own init data (name tables,
            # message buffers) — inherently different bytes, not a port
            # signal.
            if cfg.get('span_obs', True) or not (oidx or deep):
                for d in range(0, width, 2):
                    obs.append(((oimm + d) & 0xffff, (cimm + d) & 0xffff,
                                2))
                    obs_pairs.append(((oimm + d) & 0xffff,
                                      (cimm + d) & 0xffff))
            for k in range(width):
                put(oimm + k, 0, 1)
                put(cimm + k, 0, 1)
        else:
            # indexed spans that fall in BSS must be zeroed, not patterned:
            # pattern bytes make index-feeding fields (planeType etc.) huge,
            # so chained table reads land in unseeded space and div0
            b = (oimg[ohdr + odgrp + oimm:ohdr + odgrp + oimm + width]
                 if oimm + width <= odata_end
                 else b'\x00' * width if oidx
                 else pattern(width))
            for k in range(width):
                put(oimm + k, b[k], 2 if oidx else 3)
                put(cimm + k, b[k], 2 if oidx else 3)
    for oo, co, sz, hx in extra_patch:
        for k, byte in enumerate(bytes.fromhex(hx)):
            put(oo + k, byte, 4)
            put(co + k, byte, 4)
    for oo, co, sz in extra_obs:
        obs.append((oo, co, sz))
        obs_pairs += [(oo + k, co + k) for k in range(sz)]
    if name in BUFPOS:
        # pos = 0x180 drains patterned bytes but stays under the 0x1ff
        # refill threshold (the refill callee is a stub in the cand build —
        # crossing it is a separate coverage gap)
        opos, cpos, obuf = BUFPOS[name]
        # cand buf base = cand partner of the oracle buf operand pair
        cbuf = next((c[0] for o, c in zip(orf, crf) if o[0] == obuf), None)
        if cbuf is None:
            cbuf = obuf
        for k, byte in enumerate(b'\x80\x01'):
            put(opos + k, byte, 4)
            put(cpos + k, byte, 4)
        obs.append((opos, cpos, 2))
        obs_pairs += [(opos + k, cpos + k) for k in range(2)]
        for d in range(0, 0x40, 2):
            obs.append((0x8800 + d, 0x8800 + d, 2))
        for k, byte in enumerate(pattern(0x200)):
            put(obuf + k, byte, 4)
            put(cbuf + k, byte, 4)

    def top(off):
        ds = dem.get(off)
        return max(ds) if ds else (0, 0)

    final = {}
    for a, b in obs_pairs:
        pa, ba = top(a)
        pb, bb = top(b)
        final[a] = final[b] = ba if (pa, ba) >= (pb, bb) else bb
    for off in dem:
        if off not in final:
            final[off] = top(off)[1]
    patch = []
    offs = sorted(final)
    run = []
    for off in offs + [None]:
        if off is not None and run and off == run[-1] + 1:
            run.append(off)
            continue
        if run:
            patch.append({'off': hex(run[0]),
                          'bytes': bytes(final[o] for o in run).hex()})
        run = [off] if off is not None else []
    return {'fn': name, 'exe': cfg['exe'], 'mod': cfg['mod'],
            'ds': cfg['ds'], 'vectors': cfg['vectors'],
            'regs': cfg.get('regs', []),
            'patch': patch, 'obs': obs}


CASES = [
    dict(fn='positionUnit', deep=True, exe='START', omap='map/start_en.map',
         mod='STGEN', ds='0x3000',
         vectors=[[0, 0], [1, 0], [0, 2], [2, 1], [3, 3]]),
    dict(fn='spawnEnemyAircraft', deep=True, exe='EGAME', omap='map/egame_en.map',
         mod='EGFRAME', ds='0x5000',
         vectors=[[0, 0], [1, 0], [0, 2], [2, 1], [3, 0]]),
    # span_obs=False: record/string-table writes derive from each binary's
    # own init name tables — coverage stays on regs + scalar stores.
    # obs_drop 0x948c: g_pct = 0x64*(rangeResult+g)/div — the range helper's
    # inputs are si-indexed negative-disp reads that reach each binary's own
    # init tables; the computed percentage legitimately differs by layout.
    dict(fn='fireAirThreat', deep=True, exe='EGAME', omap='map/egame_en.map',
         mod='EGFRAME', ds='0x5000', span_obs=False, obs_drop={0x948c},
         vectors=[[0], [1], [2], [3], [0]]),
    dict(fn='commFetch', exe='START', omap='map/start_en.map',
         mod='STGEN', ds='0x3000',
         vectors=[[0x7000, 4, 2], [0x7000, 1, 8], [0x7000, 2, 0x40],
                  [0x7000, 8, 0], [0x7000, 3, 0x11]]),
    dict(fn='memAppend', exe='START', omap='map/start_en.map',
         mod='STGEN', ds='0x3000',
         vectors=[[0x7000, 4, 2, 0], [0x7000, 1, 8, 0],
                  [0x7000, 2, 0x40, 0], [0x7000, 8, 0, 0],
                  [0x7000, 3, 0x11, 0]]),
    dict(fn='bufReadBytes', exe='START', omap='map/start_en.map',
         mod='STGEN', ds='0x3000', regs=['ax'],
         vectors=[[0x8800, 4], [0x8800, 0x20], [0x8800, 1],
                  [0x8800, 0], [0x8800, 0x40]]),
    dict(fn='bufReadFile', exe='START', omap='map/start_en.map',
         mod='STGEN', ds='0x3000', regs=['ax'],
         vectors=[[0x8800, 4, 0], [0x8800, 0x20, 0], [0x8800, 1, 0],
                  [0x8800, 0, 0], [0x8800, 0x40, 0]]),
    # commData -> ds:0xC000 scratch; a synthetic world image at +0x7a feeds
    # readWorldData's movedata chain on both sides.  Observed: every copied
    # field (paired cells) + worldBufPtr's final advance — identical because
    # the source is shared scratch.  Unobservable, inherent to the arena
    # packing each binary's linker chose: worldStrings[] pointer values
    # (own buf base + split offset — obs_drop covers the worldStrings[0]
    # store too), wpCount (cand flightData 0x5464..0x5a64 overlaps its
    # 0x5572 cell), objCount (oracle unitType 0x998a..0x99ee covers 0x9920),
    # buf head (cand overlap below +0xfc).
    dict(fn='loadWorldStrings', deep=True, exe='END', omap='map/end_en.map',
         mod='ENBRIEF', ds='0x3000', span_obs=False, obs_drop={0x9a22},
         vectors=[[]]),
]

# moveDst far-ptr (oracle ds:0x98c8 / cand ds:0x3f18) -> ds:0x8000 scratch
# NB: cand offsets shift on every test-exe relink — re-derive by disasm.
MOVEDST = {'commFetch': (0x98c8, 0x3f18), 'memAppend': (0x98c8, 0x3f18)}
# shared read-pos cell + oracle buffer base (cand base derived by pairing)
BUFPOS = {'bufReadBytes': (0x1714, 0x686e, 0x12c2),
          'bufReadFile': (0x1714, 0x686e, 0x12c2)}
# loadWorldStrings: commData far-ptr cells (o/c) and every movedata dest
# pair, in readWorldData's call order, lifted from each side's disasm.
WORLDSRC = {
    'loadWorldStrings': dict(
        comm=(0x9ed6, 0x3caa),          # far ptr -> repointed at ds:0x8000
        bufptr=(0x6c26, 0x545e),        # worldBufPtr (off,seg) advanced end
        ready=(0x99f2, 0x49b4),         # worldDataReady
        fields=[(0x86b6, 0x666a, 2),    # worldRouteTable
                (0x9ed4, 0x337c, 2),    # worldRouteCount
                (0x86c6, 0x87b6, 16),   # worldObjects   (objCount<<4)
                (0x9a1c, 0x496a, 2),    # worldSamCount
                (0x8ea8, 0x7368, 72),   # worldSamTable  (36*samCount)
                (0x998a, 0x3b92, 100),  # unitTypeTable
                (0x9922, 0x60e0, 100),  # worldUnitFlags
                (0x9bea, 0x5a72, 0x1f0),# worldStringBuf tail (cand head
                                        #  0x5974 overlapped by flightData
                                        #  through 0x5a72 = head+0xfe)
                (0x8b7a, 0x5354, 0x100),# gridFlags
                (0x99f0, 0x7bbc, 2),    # worldGridSize
                (0x86a6, 0x3332, 2),    # worldMiscHeader
                (0x42,   0x8dce, 16),   # weaponDataBlock
                (0x8c7a, 0x50e2, 36),   # targetBlockWd
                (0x9182, 0x5472, 0x600)]  # flightDataBuf
    ),
}


def world_image():
    """Synthetic comm-buffer world image read from ds:0x807a upward, in
    readWorldData's field order.  Counts are small nonzero so the
    variable-size reads (objects<<4, samTable*36) stay exercised."""
    w = bytearray()
    w += struct.pack('<HHHH', 3, 1, 0x2222, 7)      # counts + routeTable
    w += pattern(16)                                # objects  (1*16)
    w += struct.pack('<H', 2)                       # samCount
    w += pattern(72)                                # samTable (2*36)
    w += pattern(200)                               # unitType + unitFlags
    sb = bytearray(pattern(750))                    # worldStringBuf:
    for j in range(0, 0x200, 9):                    #   NUL every 9 -> splits
        sb[j] = 0
    for j in range(0x260, 0x2e0):                   #   dense-NUL tail hits
        sb[j] = 0                                   #   the i>=100 bound
    w += sb
    w += pattern(0x100)                             # gridFlags
    w += struct.pack('<HH', 0x40, 0x1234)           # gridSize + misc
    w += pattern(16)                                # weaponData
    w += pattern(36)                                # targetBlock
    w += pattern(0x600)                             # flightDataBuf
    return bytes(w)                                 # len == 0xB50


def main():
    out_cases = []
    for cfg in CASES:
        name = cfg['fn']
        oexe, _, odgrp = EXE[cfg['exe']]
        if not os.path.isabs(oexe):
            oexe = os.path.join(ROOT, oexe)
        oimg, ohdr = mz_img(oexe)
        cexe = os.path.join(ROOT, 'build/%s.EXE' % cfg['mod'])
        cmap = os.path.join(ROOT, 'build/%s.MAP' % cfg['mod'])
        cimg, chdr = mz_img(cexe)
        cdgrp = cand_dgrp(cmap)
        extra_patch, extra_obs = [], []
        ds_seg = int(cfg['ds'], 16)
        if name in MOVEDST:
            oo, co = MOVEDST[name]
            # moveDst = ds:0x8000 both sides; observe the ptr (4 bytes) and
            # the first 0x40 bytes movedata lands at the scratch dst.
            extra_patch.append((oo, co, 4, struct.pack('<HH', 0x8000,
                                                       ds_seg).hex()))
            extra_obs.append((oo, co, 4))
            for d in range(0, 0x40, 2):
                extra_obs.append((0x8000 + d, 0x8000 + d, 2))
            extra_patch.append((0x7000, 0x7000, 0x40, pattern(0x40).hex()))
        if name in WORLDSRC:
            w = WORLDSRC[name]
            oo, co = w['comm']
            # commData = ds:0xC000 -> worldBufPtr lands at ds:0xC07a where
            # the synthetic image sits (same offsets both guests).  0xC000
            # keeps the source above every movedata dest cell — a dest write
            # into the source window would corrupt later reads.
            extra_patch.append((oo, co, 4, struct.pack('<HH', 0xC000,
                                                       ds_seg).hex()))
            img_bytes = world_image()
            extra_patch.append((0xC07a, 0xC07a, len(img_bytes),
                                img_bytes.hex()))
            extra_obs.append((w['ready'][0], w['ready'][1], 2))
            extra_obs.append((w['bufptr'][0], w['bufptr'][1], 4))
            extra_obs += list(w['fields'])
        out_cases.append(case_for(name, cfg, oexe, odgrp, oimg, ohdr,
                                  cmap, cimg, chdr, cdgrp,
                                  extra_patch, extra_obs))
    emit(out_cases, os.path.join(ROOT, 'dosunit/edge.json'))
    print('wrote dosunit/edge.json:', len(out_cases), 'cases')


if __name__ == '__main__':
    main()
