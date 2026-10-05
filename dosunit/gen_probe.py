#!/usr/bin/env python3
# Bulk differential probe: emits arg-only vectors plus auto-derived DS-cell
# pulls AND store-cell observations for every ported routine not yet covered
# by a dedicated spec.
#
# Pull cells are found by scanning each routine's disasm for absolute DS
# refs — `[0xNNNN]` directly, or `[bx/si/di + 0xNNNN]` (probe registers start
# at zero, so the effective address equals the immediate).  Bytes are pulled
# from that side's own image so the routine sees its natural initialized
# data.
#
# Store cells (`mov/inc/add/...  [0xNNNN]`, first operand memory) become
# paired post-state observations — this is what makes VOID routines testable:
# their ax is dead-register noise, but their stores are real behavior.
#
# check_regs is derived from the C return type:
#   int32/uint32/long -> ax+dx,  anything else non-void -> ax,  void -> []
#
# The scratch DS is auto-placed ABOVE both images: START/END/SU oracle images
# extend past the old 0x2000 window, which made oracle pointer-arg derefs see
# real image bytes while the candidate read zeros (spurious DIFFs).
import glob, json, os, re, struct, sys
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
                + '/tools')
from duspec import emit, cand_off, cand_dgrp, EXE
import capstone

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

SUITES = {
    'probe_egame': dict(srcdir='src_en',    omap='map/egame_en.map',
                       exe='EGAME', mod='EGFRAME'),
    'probe_start': dict(srcdir='src_start', omap='map/start_en.map',
                       exe='START', mod='STGEN'),
    'probe_end':   dict(srcdir='src_end',   omap='map/end_en.map',
                       exe='END',   mod='ENBRIEF'),
    'probe_su':    dict(srcdir='src_su',    omap='map/su_en.map',
                       exe='SU',    mod='SUUTIL'),
}

STACK_LO, STACK_HI = 0x40000, 0x50000   # SS=0x4000 window

# 6 words pushed regardless of true arity — extras sit harmlessly on the
# stack; missing args read deterministic zeros.
VECTORS = [
    [0, 0, 0, 0, 0, 0],
    [1, 2, 3, 4, 5, 6],
    [0x100, 0x80, 0x40, 0x20, 0x10, 8],
    [0x7fff, -0x8000, -1, 1, 0, 0x4000],
    [0x4000, 0x4000, 0x4000, 0x4000, 0x4000, 0x4000],
]

SKIP = {  # entry points / unsynthesizable: hw io, int21, argv/env, overlays
    'main', 'gfxInit', 'installCBreakHandler', 'setInt9Handler',
    'openFile', 'closeFile', 'picBlit', 'openBlitClosePic', 'load15Flt3d3',
    'waitForKeyPress', 'runGameSession', 'fillSpanRect', 'projectSceneObject',
}

WRITE_MN = {
    'mov', 'xchg', 'pop', 'inc', 'dec', 'neg', 'not',
    'add', 'sub', 'adc', 'sbb', 'and', 'or', 'xor',
    'shl', 'shr', 'sar', 'sal', 'rol', 'ror', 'rcl', 'rcr',
}
READ_MN = {'lea', 'push', 'call', 'cmp', 'test', 'jmp'}  # + all jcc (j*)
ABS_RE = re.compile(r'(?:word|byte|dword) ptr (?:(ds|es|cs|ss|fs|gs):)?\['
                    r'(?:(bx|si|di|bp)\s*([+-]))?\s*'
                    r'(0x[0-9a-fA-F]+|[0-9a-fA-F]+h?)\]')


def mz_img(path):
    data = open(path, 'rb').read()
    return data, struct.unpack('<H', data[8:10])[0] * 16


def seg_bases(map_path):
    segs = {}
    for line in open(map_path, errors='replace'):
        m = re.match(r'(\w+) (CODE|DATA|STACK) ([0-9a-fA-F]+)', line.strip())
        if m:
            segs[m.group(1)] = int(m.group(3), 16) * 16
    return segs


def oracle_extent(name, map_path, segs):
    for line in open(map_path, errors='replace'):
        m = re.match(r'(\w+): (\w+) (NEAR|FAR) ([0-9a-fA-F]+)-([0-9a-fA-F]+)',
                     line.strip())
        if m and m.group(1) == name:
            return (segs[m.group(2)] + int(m.group(4), 16),
                    segs[m.group(2)] + int(m.group(5), 16), m.group(3))
    return None, None, None


def cand_extent(map_path, name, dgrp):
    """(start, end) for a cand routine — end = next code public's offset
    in the SAME code segment (dup pubdef lines collapse away)."""
    pat = re.compile(r'([0-9A-Fa-f]+):([0-9A-Fa-f]+)\s+_(\w+)\s*$')
    tgt_seg = tgt_off = None
    seg_offs = {}
    for line in open(map_path, errors='replace'):
        m = pat.search(line)
        if not m:
            continue
        seg, off, nm = int(m.group(1), 16), int(m.group(2), 16), m.group(3)
        if seg * 16 + off >= dgrp:      # keep only code syms (below DGROUP)
            continue
        seg_offs.setdefault(seg, set()).add(off)
        if nm == name:
            tgt_seg, tgt_off = seg, off
    if tgt_off is None:
        return None, None
    end = min((o for o in seg_offs[tgt_seg] if o > tgt_off), default=None)
    return tgt_off, end


def mem_refs(img, hdr, off, end=None):
    span = (end - off) if end else 0x400
    """(pull_offsets, store_sites_in_disasm_order).

    Store sites count only DIRECT `[0xNNNN]` destinations: an indexed
    `[reg+imm]` store can land anywhere, while pulls on indexed refs are
    harmless even when the register ends up nonzero (the seeded cell is
    simply unused).  bp-based refs use SS — never pulled.  Store sites keep
    PROGRAM order so oracle/cand pair by position — the ports are
    byte-structured, so store i on one side is store i on the other; the
    /* word_NNNNN */ comments drift between binary revs and cannot be
    trusted for joining.
    """
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_16)
    pulls, stores = set(), []
    seen_st = set()
    for i in md.disasm(img[hdr + off:hdr + off + span], off):
        dst = i.op_str.split(',', 1)[0]
        for op, is_dst in ((i.op_str, False), (dst, True)):
            m = ABS_RE.search(op)
            if not m:
                continue
            seg, reg, sgn = m.group(1), m.group(2), m.group(3)
            imm = int(m.group(4).rstrip('h'), 16)
            if sgn == '-':
                imm = (-imm) & 0xFFFF
            if seg == 'ss' or reg == 'bp':
                continue                    # SS-relative — not DS scratch
            pulls.add(imm)
            if is_dst and i.mnemonic in WRITE_MN and reg is None \
                    and imm not in seen_st:
                seen_st.add(imm)
                size = 4 if 'dword' in i.op_str else \
                       1 if 'byte ptr' in i.op_str else 2
                stores.append((imm, size))
    return sorted(pulls), stores


def covered():
    cov = set()
    for f in glob.glob(os.path.join(ROOT, 'dosunit/*.json')):
        b = os.path.basename(f)
        if (b.startswith('probe_') or '.g' in b
                or b.endswith('out.json') or b.endswith('vectors.json')):
            continue
        try:
            d = json.load(open(f))
        except Exception:
            continue
        if isinstance(d, list):
            cov.update(c['fn'] for c in d)
    return cov


DEF_RE = re.compile(
    r'^(int16|int32|uint16|uint32|int8|uint8|void|char|int|long'
    r'|[A-Za-z_]\w*\s*\*+)\s*'
    r'(?:FAR\s+|NEAR\s+)?(?:\*\s*)?(\w+)\s*\([^;{}()]*\)\s*\{', re.M)


def ported_defs(srcdir, omap_set):
    """(name, ret_type) for real DEFINITIONS — prototypes and stub files
    (ststubs.c et al. are deliberate placeholders, not verified ports)."""
    defs = {}
    for f in glob.glob(os.path.join(ROOT, srcdir, '*.c')):
        if 'stubs' in os.path.basename(f):
            continue
        txt = open(f, errors='replace').read()
        for m in DEF_RE.finditer(txt):
            if m.group(2) in omap_set:
                defs[m.group(2)] = m.group(1)
    return defs


def pick_ds(oexe, ohdr, cexe, chdr):
    """First 64K-aligned segment whose window sits above BOTH images and
    clear of the scratch stack — prevents image/scratch aliasing skew."""
    end = max(0x10000 + len(open(oexe, 'rb').read()) - ohdr,
              0x10000 + len(open(cexe, 'rb').read()) - chdr)
    seg = ((end + 0xFFFF) // 0x10000) * 0x1000   # 64K page -> segment
    if seg * 16 < STACK_HI and seg * 16 + 0xFFFF >= STACK_LO:
        seg = STACK_HI // 16                    # skip the SS=0x4000 window
    return '0x%x' % seg


for suite, cfg in SUITES.items():
    segs = seg_bases(os.path.join(ROOT, cfg['omap']))
    omap_set = set()
    for line in open(os.path.join(ROOT, cfg['omap']), errors='replace'):
        m = re.match(r'(\w+): \w+ (?:NEAR|FAR) [0-9a-f]+-', line.strip())
        if m:
            omap_set.add(m.group(1))
    defs = ported_defs(cfg['srcdir'], omap_set)
    defs = {n: t for n, t in defs.items()
            if n not in covered() and n not in SKIP}

    # candidate module exe + its dseg image window for pull guards
    cexe = os.path.join(ROOT, 'build/%s.EXE' % cfg['mod'])
    cmap = os.path.join(ROOT, 'build/%s.MAP' % cfg['mod'])
    cimg, chdr = mz_img(cexe)
    cdgrp = cand_dgrp(cmap)
    cdata_end = len(cimg) - chdr - cdgrp   # initialized data extent

    # oracle image for pull guards
    oexe, omap_rel, odgrp = EXE[cfg['exe']]
    oimg, ohdr = mz_img(oexe)
    odata_end = len(oimg) - ohdr - odgrp

    ds = pick_ds(oexe, ohdr, cexe, chdr)
    cases, skipped, vacuous = [], [], []
    for name in sorted(defs):
        f_o, e_o, kind = oracle_extent(name, os.path.join(ROOT, cfg['omap']),
                                       segs)
        if f_o is None:
            skipped.append((name, 'not in oracle map'))
            continue
        if kind == 'FAR':
            skipped.append((name, 'FAR frame'))
            continue
        f_c, e_c = cand_extent(cmap, name, cdgrp)
        if f_c is None:
            skipped.append((name, 'not in cand map'))
            continue

        o_pull, o_st = mem_refs(oimg, ohdr, f_o, e_o)
        c_pull, c_st = mem_refs(cimg, chdr, f_c, e_c)
        # instruction-order paired store-cell obs: byte-exact ports put the
        # i-th DS store on both sides at the same logical cell.  A count
        # mismatch means the paths diverged structurally — skip obs rather
        # than pair wrong cells.
        obs = []
        if o_st and len(o_st) == len(c_st):
            obs = [(o, c, min(osz, csz))
                   for (o, osz), (c, csz) in zip(o_st, c_st)][:24]
        # Seed every obs cell with a sentinel on BOTH sides.  A cell the
        # routine never writes then reads back identical a5a5 bytes; only a
        # real store (or a real divergence) can produce an obs mismatch.
        # Without this, untouched cells compare the two images' init bytes —
        # layout-legit differences masquerading as behavior diffs.
        sent = {'o': {o for o, _, _ in obs}, 'c': {c for _, c, _ in obs}}
        patch = []
        for off in o_pull:
            if off + 2 <= odata_end and off not in sent['o']:
                patch.append((off, None, 2))
        for off in c_pull:
            if off + 2 <= cdata_end and off not in sent['c']:
                patch.append((None, off, 2))
        patch += [(o, c, sz, 'a5' * sz) for o, c, sz in obs]

        rt = defs[name]
        regs = (['ax', 'dx'] if rt in ('int32', 'uint32', 'long')
                else [] if rt == 'void' else ['ax'])
        if not regs and not obs:
            vacuous.append(name)    # void + no paired stores = nothing checks
        case = {'fn': name, 'exe': cfg['exe'], 'mod': cfg['mod'],
                'ds': ds, 'patch': patch, 'vectors': VECTORS}
        if obs:
            case['obs'] = obs
        if regs != ['ax']:
            case['regs'] = regs
        cases.append(case)

    # shard into ~25-case files so each replay group finishes within the
    # subprocess timeout even with instruction-limit burners
    SHARD = 25
    if len(cases) <= SHARD:
        emit(cases, os.path.join(ROOT, 'dosunit/%s.json' % suite))
        print('%s: %d cases (%d skipped)' % (suite, len(cases), len(skipped)))
    else:
        for si in range(0, len(cases), SHARD):
            emit(cases[si:si + SHARD],
                 os.path.join(ROOT, 'dosunit/%s_%d.json' % (suite, si // SHARD)))
        print('%s: %d cases in %d shards (%d skipped)'
              % (suite, len(cases), (len(cases) + SHARD - 1) // SHARD,
                 len(skipped)))
    if skipped:
        print('   skipped:', ', '.join('%s=%s' % kv for kv in skipped[:8]))
    if vacuous:
        print('   vacuous (void, no paired stores): %d' % len(vacuous))
