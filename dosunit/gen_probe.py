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

# `int NN` sites find_ints' near-BFS cannot reach — dispatch through
# indirect calls hides them from the static call graph.  Harvested from
# replay `interrupt_or_iret` fault events (image offset = linear - load
# base); keyed (oracle exe stem, fn) so a case picks up every site its
# real execution reached.  Re-check after oracle changes (never — the
# oracle binaries are frozen) or new fault reports.
FORCE_INTS = {
    ('EGAME', 'drawCockpit'):      [0xe0af, 0xe156],
    ('EGAME', 'initFrameRandom'):  [0x1fac],
    ('END',   'allocBuffer'):      [0x2eca],
    ('END',   'allocClearBuf'):    [0x2eca],
    ('END',   'cleanup'):          [0x3a2f],
    ('END',   'drawMapView'):      [0x2eca],
    ('END',   'freeBuffer'):       [0x2ee2],
    ('END',   'initGraphics'):     [0x3846],
    ('END',   'loadFileSection'):  [0x14c1],
    ('END',   'loadMapView'):      [0x2eca],
    ('END',   'sub_17334'):        [0x3a2f],
    ('END',   'sub_175BC'):        [0x3a2f],
    ('END',   'writeFileSection'): [0x1418],
    ('START', 'allocBuffer'):      [0x68a0],
    ('START', 'cleanup'):          [0x5295],
    ('START', 'drawStoreIcons'):   [0x4841],
    ('START', 'drawStringCentered'): [0x68a0],
    ('START', 'freeBuffer'):       [0x68b8],
    ('START', 'initGraphics'):     [0x50bc],
    ('START', 'resFileReadBlock'): [0x4941],
    ('START', 'resFileWriteBlock'): [0x4898],
    ('START', 'saveHallfame'):     [0x40e3],
    ('START', 'setViewOrigin'):    [0x68a0],
    ('START', 'sub_10010'):        [0x46dc],
    ('START', 'sub_15460'):        [0x4841],
    ('START', 'sub_15B68'):        [0x68a0],
    ('START', 'sub_18F12'):        [0x4841],
    ('START', 'sub_193EE'):        [0x4841],
}

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
    'installCBreakHandler', 'setInt9Handler',
    'openFile', 'closeFile', 'picBlit', 'load15Flt3d3',
    'fillSpanRect', 'projectSceneObject',
}
# Entry points attempted under call_stub/int_stub/dsss coverage: they run
# to a typed boundary (budget/int/io) but their bodies are pure call
# sequences — the stub fixtures model everything they reach.  Any that
# still diverge or exhaust asymmetrically get documented artifacts.
PROBE_ATTEMPT = {
    'main', 'gfxInit', 'runGameSession', 'openBlitClosePic', 'waitForKeyPress',
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
    pulls, stores, segptrs = {}, [], set()
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
            # direct [imm] -> seed just that cell; indexed [reg+imm] ->
            # seed a forward span: probe regs start at 0 but arg*stride
            # products land the effective address past the bare cell —
            # an unseeded divisor/table cell there faults interrupt:0
            # or reads an untouched image byte asymmetrically.
            want = 2 if reg is None else 0x40
            pulls[imm] = max(pulls.get(imm, 0), want)
            if is_dst and i.mnemonic in WRITE_MN and reg is None \
                    and imm not in seen_st:
                seen_st.add(imm)
                size = 4 if 'dword' in i.op_str else \
                       1 if 'byte ptr' in i.op_str else 2
                stores.append((imm, size))
            # direct [imm] cells consumed as far-pointer parts: les/lds
            # reads a dword (off@imm, seg@imm+2); `mov es,[imm]` loads
            # the cell itself as a segment.  A natural image seg value
            # dereferences unmapped space, so these cells get forced to
            # the scratch DS at patch time.
            if i.mnemonic in ('les', 'lds') and reg is None:
                segptrs.add(imm + 2)
            elif i.mnemonic == 'mov' and is_dst is False and reg is None \
                    and i.op_str.split(',', 1)[0].strip() in ('es', 'ds'):
                segptrs.add(imm)
    return pulls, stores, sorted(segptrs)


def _bp_disp(i):
    """Signed [bp+d] disp of a no-index mem operand, or None."""
    for op in i.operands:
        if op.type == capstone.x86.X86_OP_MEM \
                and op.mem.base == capstone.x86.X86_REG_BP \
                and op.mem.index == capstone.x86.X86_REG_INVALID:
            d = op.mem.disp
            return d - 0x10000 if d > 0x7FFF else d
    return None


def far_arg_sites(img, hdr, off, end):
    """Arg word indices consumed as far pointers.  Under the near16 probe
    frame arg word w sits at [bp+4+2w] (push bp; mov bp,sp).  Sites:

      les/lds r16, [bp+d]            -> ptr pair at words (d-4)/2, +1
      push [bp+d+2]; push [bp+d]     -> far ptr forwarded by value to a
                                       callee (adjacent hi/lo pushes)
      mov es/ds, word ptr [bp+d]     -> manual seg load; off part is the
                                       word below -> pair (d-6)/2, (d-4)/2

    Fixed probe args point those words at unmapped segments; gen_probe
    rewrites them into a seeded window inside the mapped scratch DS."""
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_16)
    md.detail = True
    insns = list(md.disasm(img[hdr + off:hdr + end], off))
    idx = set()
    has_far = False
    run = []                     # descending [bp+d] push run, newest last
    for i in insns:
        d = _bp_disp(i)
        if i.mnemonic in ('les', 'lds'):
            has_far = True
            if d is not None and 4 <= d <= 0x20 and d % 2 == 0:
                idx.add((d - 4) // 2)
        elif i.mnemonic == 'mov' and i.operands \
                and i.operands[0].type == capstone.x86.X86_OP_REG \
                and i.operands[0].reg in (capstone.x86.X86_REG_ES,
                                          capstone.x86.X86_REG_DS):
            if d is not None and 6 <= d <= 0x20 and d % 2 == 0:
                idx.add((d - 6) // 2)
                has_far = True
        if i.mnemonic == 'push' and d is not None and d % 2 == 0:
            if run and d != run[-1] - 2:
                if len(run) >= 2:
                    # each adjacent descending pair is a *possible* far
                    # ptr forward — flag all of them; word values are
                    # assigned so any pair decodes to mapped space.
                    for lo in run[1:]:
                        if 4 <= lo <= 0x1E:
                            idx.add((lo - 4) // 2)
                run = []
            run.append(d)
        elif i.mnemonic != 'push' or d is None:
            if len(run) >= 2:
                for lo in run[1:]:
                    if 4 <= lo <= 0x1E:
                        idx.add((lo - 4) // 2)
            run = []
    if len(run) >= 2:
        for lo in run[1:]:
            if 4 <= lo <= 0x1E:
                idx.add((lo - 4) // 2)
    if has_far:
        # local far ptrs are commonly filled by an adjacent dword arg
        # read (`mov ax,[bp+6]; mov dx,[bp+8]` into a local that a later
        # les consumes) — flag the covered arg pair as well.
        for k in range(len(insns) - 1):
            a, b = insns[k], insns[k + 1]
            if a.mnemonic != 'mov' or b.mnemonic != 'mov':
                continue
            if not (a.operands and a.operands[0].type ==
                    capstone.x86.X86_OP_REG) \
                    or not (b.operands and b.operands[0].type ==
                            capstone.x86.X86_OP_REG):
                continue
            da, db = _bp_disp(a), _bp_disp(b)
            if da is None or db is None or abs(da - db) != 2:
                continue
            lo = min(da, db)
            if 4 <= lo <= 0x1E and lo % 2 == 0:
                idx.add((lo - 4) // 2)
    return sorted(idx)


def free_window(used, total):
    """Lowest scratch-DS offset with `total` contiguous bytes clear of
    every patched cell in `used` (sorted (lo,hi) intervals)."""
    cur = 0
    for lo, hi in used:
        if lo - cur >= total:
            return cur
        cur = max(cur, hi)
    return cur if 0x10000 - cur >= total else None


def reaches_lcall(img, hdr, off, end):
    """True when the routine's near-call graph can reach an `lcall` (0x9A)
    into the runtime overlay/driver jump table.  BFS over rel16 call
    targets inside the image, bounded — misses are just kept-incomplete,
    never misflagged cases."""
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_16)
    imax = len(img) - hdr
    seen, work, budget = set(), [(off, end)], 3000
    while work and budget > 0:
        cur, cend = work.pop()
        if cur in seen or not (0 <= cur < imax):
            continue
        seen.add(cur)
        for i in md.disasm(img[hdr + cur:hdr + min(cend, imax)], cur):
            budget -= 1
            if not budget or i.mnemonic in ('ret', 'retf', 'iret'):
                break
            if i.bytes and i.bytes[0] == 0x9A:
                return True
            if i.bytes and i.bytes[0] in (0xE8, 0xE9):
                disp = int.from_bytes(i.bytes[1:3], 'little', signed=True)
                tgt = (i.address + 3 + disp) & 0xFFFF
                if 0 <= tgt < imax:
                    work.append((tgt, min(tgt + 0x400, imax)))
            elif i.bytes and i.bytes[0] == 0xEB:
                disp = int.from_bytes(i.bytes[1:2], 'little', signed=True)
                tgt = (i.address + 2 + disp) & 0xFFFF
                if 0 <= tgt < imax:
                    work.append((tgt, min(tgt + 0x400, imax)))
    return False


def find_ints(img, hdr, off, end, windows=None, dsss=None):
    """Image offsets of `int NN` instructions reachable through the
    routine's call graph — BFS over rel16 call/jmp targets and lcall
    seg:off immediates inside the image.  Callee windows extend to the
    first ret (big file-IO helpers exceed a fixed 0x400 cap).  duspec
    turns them into a prepatched oracle fixture (`CD xx` -> `31 C0` xor
    ax,ax: a bounded "service succeeded, ax=0" answer symmetric to the
    cand build's zero-returning stubs).  int3/into/iret are left alone.
    When `windows` is a list, every visited scan window is appended so
    callers can re-scan callee code for data refs (callee-read globals —
    e.g. a divisor inside a helper — are invisible to body-only pulls).
    When `dsss` is a list, reachable `ds := ss` writes (`push ss;pop ds`,
    `mov r16,ss; mov ds,r16` — real-DOS no-ops that clobber the probe's
    scratch DS with the stack window) are appended as (site, bytes)
    nop-patches."""
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_16)
    imax = len(img) - hdr
    seen, work, budget = set(), [(off, end)], 40000
    sites = []
    dseen = set()
    # iseen dedups at instruction granularity: overlapping callee windows
    # would otherwise rescan shared code and exhaust the budget before
    # deep helpers (resFileOpen's int21 path) are ever reached.  Linear
    # decode converging at an already-scanned address reproduces the same
    # stream, so breaking there loses no sites.
    iseen = set()
    while work and budget > 0:
        cur, cend = work.pop()
        if cur in seen or not (0 <= cur < imax):
            continue
        seen.add(cur)
        if windows is not None:
            windows.append((cur, cend))
        prev = None
        for i in md.disasm(img[hdr + cur:hdr + min(cend, imax)], cur):
            if i.address in iseen:
                break
            iseen.add(i.address)
            budget -= 1
            if not budget or i.mnemonic in ('ret', 'retf', 'iret'):
                break
            if i.bytes and i.bytes[0] == 0xCD:
                sites.append(i.address)
            if dsss is not None and i.mnemonic and i.bytes:
                # `ds := ss` writes — real-DOS no-ops (DS==SS==DGROUP) that
                # clobber the probe's scratch DS with the SS window.  The
                # faithful model is a nop: DGROUP is the scratch DS.
                #   push ss; pop ds          16 1f   -> nop both
                #   mov r16,ss; mov ds,r16   8c dR 8e dR -> nop the `8e`
                if (prev and i.address == prev[0] + prev[1] and
                        i.bytes[0] == 0x1F and prev[0] not in dseen and
                        prev[2] == 'push' and prev[3] == 'ss'):
                    dseen.add(prev[0])
                    dsss.append((prev[0], b'\x90\x90'))
                elif (prev and i.address == prev[0] + prev[1] and
                      i.address not in dseen and
                      i.mnemonic == 'mov' and
                      i.op_str.split(',')[0].strip() == 'ds' and
                      prev[2] == 'mov' and prev[3].endswith(', ss') and
                      prev[3].split(',')[0].strip() ==
                      i.op_str.split(',')[1].strip()):
                    dseen.add(i.address)
                    dsss.append((i.address, b'\x90\x90'))
            prev = (i.address, i.size, i.mnemonic, i.op_str)
            if i.bytes and i.bytes[0] in (0x9A, 0xEA):
                # lcall / jmp far seg:off — image offsets are link-linear
                # (seg*16+off): resident far helpers, driver routines and
                # resident-driver table entries all land inside the image.
                tgt = (int.from_bytes(i.bytes[3:5], 'little') * 16
                       + int.from_bytes(i.bytes[1:3], 'little'))
                if 0 <= tgt < imax:
                    work.append((tgt, min(tgt + 0x1800, imax)))
            if i.bytes and i.bytes[0] in (0xE8, 0xE9):
                # near call/jump rel16 — jmp covers tail-called helpers
                disp = int.from_bytes(i.bytes[1:3], 'little', signed=True)
                tgt = (i.address + 3 + disp) & 0xFFFF
                if 0 <= tgt < imax:
                    work.append((tgt, min(tgt + 0x1800, imax)))
            elif i.bytes and i.bytes[0] == 0xEB:
                disp = int.from_bytes(i.bytes[1:2], 'little', signed=True)
                tgt = (i.address + 2 + disp) & 0xFFFF
                if 0 <= tgt < imax:
                    work.append((tgt, min(tgt + 0x1800, imax)))
    return sorted(sites)


def entry_names(map_path, segs):
    """image offset -> routine name for every map entry (all segs)."""
    out = {}
    for line in open(map_path, errors='replace'):
        m = re.match(r'(\w+): (\w+) (?:NEAR|FAR) ([0-9a-fA-F]+)-', line.strip())
        if m and m.group(2) in segs:
            out[segs[m.group(2)] + int(m.group(3), 16)] = m.group(1)
    return out


def stub_models(paths):
    """name -> stub model for candidate-side trivial defs: 'void' (empty
    body), 'zero' (`return 0`), ('const', v) (`return <imm>`), ('param',
    i) (`return <arg>`).  Anything with a real body is real work and must
    not be stubbed."""
    sig = re.compile(
        r'^(?:int16|int32|uint16|uint32|int8|uint8|void|char|int|long'
        r'|[A-Za-z_]\w*\s*\*+)\s*(?:far\s+|near\s+)?(\w+)\s*'
        r'\(([^{};()]*)\)\s*\{\s*([^{}]*)\}', re.M)
    out = {}
    for path in paths:
        if not os.path.exists(path):
            continue
        for m in sig.finditer(open(path, errors='replace').read()):
            name, params, body = m.group(1), m.group(2), m.group(3).strip()
            if not body:
                out[name] = 'void'
                continue
            rm = re.match(r'^return\s+(.+?)\s*;?$', body, re.S)
            if not rm:
                continue
            expr = rm.group(1)
            # peel C casts: `return (int16)a;` is still a param return
            expr = re.sub(r'^\(\s*[A-Za-z_]\w*[\s\*]*\)\s*', '', expr)
            if re.match(r'^(0x[0-9a-fA-F]+|\d+)$', expr):
                out[name] = ('const', int(expr, 0) & 0xFFFF)
            elif re.match(r'^\w+$', expr):
                plist = [p for p in params.split(',')
                         if p.strip() and p.strip() != 'void']
                names = [re.findall(r'\w+', p)[-1] for p in plist]
                if expr in names:
                    out[name] = ('param', names.index(expr))
    return out


def find_calls(img, hdr, off, end, entries, stubs, offstubs):
    """`call rel16`/`lcall seg:off` sites in the routine's reachable graph
    whose callee the candidate build supplies as a trivial stub — the
    probe must not run oracle code the candidate does not have (overlay
    routines, isr installers, dos helpers), so the site gets patched to
    the stub's observable answer.  BFS mirrors find_ints; near-call
    targets resolve through the map's entry table, far calls through the
    same table or the invented-name off index (ovl_*, ovlCall_*,
    textOp_* all encode the lcall's off field)."""
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_16)
    imax = len(img) - hdr
    seen, work, budget = set(), [(off, end)], 40000
    sites = {}              # site -> patch bytes; overlapping BFS windows
                            # rescan addresses, so dedupe is mandatory
    while work and budget > 0:
        cur, cend = work.pop()
        if cur in seen or not (0 <= cur < imax):
            continue
        seen.add(cur)
        for i in md.disasm(img[hdr + cur:hdr + min(cend, imax)], cur):
            budget -= 1
            if not budget or i.mnemonic in ('ret', 'retf', 'iret'):
                break
            op = i.bytes[0] if i.bytes else 0
            if op in (0x9A, 0xEA):
                tgt = (int.from_bytes(i.bytes[3:5], 'little') * 16
                       + int.from_bytes(i.bytes[1:3], 'little'))
                if 0 <= tgt < imax:
                    work.append((tgt, min(tgt + 0x1800, imax)))
                name = entries.get(tgt)
                form = stubs.get(name) if name is not None else None
                if form is None:
                    # invented stub names key on the lcall's off field
                    # (ovl_*, ovlCall_*, textOp_*) or on the target's
                    # linear address (sub_<lin>) — try both.  An extent
                    # aliased to a real name (map 'duplicate') still
                    # resolves to a cand stub by its sub_<lin> alias.
                    form = (offstubs.get(int.from_bytes(i.bytes[1:3],
                                                       'little'))
                            or offstubs.get(tgt))
                if op == 0x9A and form is not None and tgt != off:
                    p = call_patch(5, form)
                    if p is not None:
                        sites[i.address] = p
            elif op in (0xE8, 0xE9):
                disp = int.from_bytes(i.bytes[1:3], 'little', signed=True)
                tgt = (i.address + 3 + disp) & 0xFFFF
                if 0 <= tgt < imax:
                    work.append((tgt, min(tgt + 0x1800, imax)))
                if op == 0xE8 and tgt != off:
                    name = entries.get(tgt)
                    form = stubs.get(name) if name is not None else None
                    if form is None:
                        # near-call targets key on the sub_<lin> alias
                        # (linear = image base + offset); bare tgt would
                        # collide with ovl_* off-field keys.
                        form = offstubs.get(0x10000 + tgt)
                    if form is not None:
                        p = call_patch(3, form)
                        if p is not None:
                            sites[i.address] = p
            elif op == 0xEB:
                disp = int.from_bytes(i.bytes[1:2], 'little', signed=True)
                tgt = (i.address + 2 + disp) & 0xFFFF
                if 0 <= tgt < imax:
                    work.append((tgt, min(tgt + 0x1800, imax)))
    return sorted(sites.items())


def call_patch(size, form):
    """Replacement bytes for a stubbed call site: `void` preserves ax
    (an empty-body cand stub leaves it untouched), `zero`/`const` model
    the stub's return write.  `param` needs a stack-relative read the
    3-byte window cannot encode; on a 5-byte lcall only param0 fits
    (`mov bx,sp; mov ax,[bx]`)."""
    if form == 'void':
        return b'\x90' * size
    if form == 'zero':
        return b'\x31\xc0' + b'\x90' * (size - 2)
    if form[0] == 'const':
        return b'\xb8' + form[1].to_bytes(2, 'little') + b'\x90' * (size - 3)
    if form[0] == 'param' and form[1] == 0:
        if size == 5:
            return b'\x8b\xdc\x8b\x07\x90'
        # 3-byte near site: `pop ax` reads the last-pushed word (param0's
        # low word); the caller's `add sp,#args` absorbs the imbalance.
        return b'\x58\x90\x90'
    return None


def call_targets(img, hdr, off, end):
    """Image offsets the routine body's direct `call rel16` instructions
    target — one call level, enough to reach helper routines whose global
    reads (divisor cells, table fields) a body-only pull scan misses."""
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_16)
    imax = len(img) - hdr
    out = []
    for i in md.disasm(img[hdr + off:hdr + end], off):
        if i.bytes and i.bytes[0] == 0xE8:
            disp = int.from_bytes(i.bytes[1:3], 'little', signed=True)
            tgt = (i.address + 3 + disp) & 0xFFFF
            if 0 <= tgt < imax and tgt != off:
                out.append(tgt)
    return out


def deep_refs(img, hdr, targets, skip):
    """Merge pull + segptr cells from direct-call target windows (each
    scanned over a bounded extent).  Stores stay body-only — callee
    stores would break positional obs pairing."""
    pulls, segptrs = {}, set()
    imax = len(img) - hdr
    for tgt in dict.fromkeys(targets):
        if tgt == skip:
            continue
        p, _st, s = mem_refs(img, hdr, tgt, min(tgt + 0x400, imax))
        for off, psz in p.items():
            pulls[off] = max(pulls.get(off, 0), psz)
        segptrs |= set(s)
    return pulls, segptrs


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
    # every module the cand build compiles real code from: the suite dir,
    # plus base src/*.c when the suite is an override layer (src_en)
    # rather than a standalone satellite (src_start/src_end/src_su).
    ported = set(defs)
    if os.path.basename(os.path.abspath(
            os.path.join(ROOT, cfg['srcdir']))) == 'src_en':
        ported |= set(ported_defs('src', omap_set))
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
    oentries = entry_names(os.path.join(ROOT, omap_rel), segs)
    # candidate-side trivial stubs: suite stubs file + shared stubs —
    # calls into them are patched to the stub's observable answer.  A
    # name the suite ports for real (stubs.c keeps a guarded twin that
    # drops out of this build) is never stubbed.
    stubs = stub_models(
        glob.glob(os.path.join(ROOT, cfg['srcdir'], '*stub*.c'))
        + glob.glob(os.path.join(ROOT, 'src', '*stub*.c'))
        + [os.path.join(ROOT, 'src', '_stub.c')])
    stubs = {n: f for n, f in stubs.items() if n not in ported}
    # invented-name stubs (ovl_*, ovlCall_*, textOp_*) encode the lcall's
    # off field — index them by it for far-call targets the map doesn't
    # cover.  Only names that are not real map entries qualify.
    offstubs = {}
    entryname_set = set(oentries.values())
    for n, f in stubs.items():
        if n in entryname_set:
            continue
        m = re.search(r'_([0-9A-Fa-f]{3,5})$', n)
        if m:
            offstubs.setdefault(int(m.group(1), 16), f)

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

        o_pull, o_st, o_seg = mem_refs(oimg, ohdr, f_o, e_o)
        c_pull, c_st, c_seg = mem_refs(cimg, chdr, f_c, e_c)
        # Callee-read globals (a divisor inside a helper like
        # setup3DTransform) are invisible to body-only pulls: the oracle's
        # own pull covers its mirror while the cand reads scratch zeros —
        # one-sided div0.  Scan direct `call` targets one level deep for
        # pull/segptr cells only (stores stay body-only to protect
        # positional obs pairing).
        dp, ds2 = deep_refs(oimg, ohdr, call_targets(oimg, ohdr, f_o, e_o), f_o)
        for off, psz in dp.items():
            o_pull[off] = max(o_pull.get(off, 0), psz)
        o_seg = sorted(set(o_seg) | ds2)
        dp, ds2 = deep_refs(cimg, chdr, call_targets(cimg, chdr, f_c, e_c), f_c)
        for off, psz in dp.items():
            c_pull[off] = max(c_pull.get(off, 0), psz)
        c_seg = sorted(set(c_seg) | ds2)
        dsss_o, dsss_c = [], []
        ints_o = find_ints(oimg, ohdr, f_o, e_o, dsss=dsss_o)
        ints_c = find_ints(cimg, chdr, f_c, e_c, dsss=dsss_c)
        for s in FORCE_INTS.get((cfg['exe'], name), []):
            assert oimg[ohdr + s] == 0xCD, (name, hex(s))
            if s not in ints_o:
                ints_o.append(s)
        # calls into cand-side trivial stubs (overlay routines, isr/dos
        # helpers) get patched to the stub's observable answer — oracle
        # must not run code the candidate does not implement.
        csites = find_calls(oimg, ohdr, f_o, e_o, oentries, stubs, offstubs)
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
        for off, psz in sorted(o_pull.items()):
            psz = min(psz, odata_end - off)
            if psz > 0 and off not in sent['o']:
                patch.append((off, None, psz))
        for off, psz in sorted(c_pull.items()):
            psz = min(psz, cdata_end - off)
            if psz > 0 and off not in sent['c']:
                patch.append((None, off, psz))
        patch += [(o, c, sz, 'a5' * sz) for o, c, sz in obs]
        # Cells consumed as far-pointer segments: force the scratch DS so
        # the deref stays in mapped space (image seg values point nowhere;
        # the seg half of a les/lds dword is otherwise left unpatched).
        dsle = struct.pack('<H', int(ds, 0)).hex()
        for off in o_seg:
            patch.append((off, None, 2, dsle))
        for off in c_seg:
            patch.append((None, off, 2, dsle))
        # Resident-driver callees (fillRect/putpixel family, reached via
        # lcall into the resident table) do rep-stosb writes to the video
        # page globals — natural values like 0xA000/0x3A00 land in
        # unmapped guest space.  Naming a segment in any patch maps its
        # whole 64K window (see real16_guest._segment_pages); one byte at
        # each video base turns those faults into real execution.
        patch.append({'seg': '0xa000', 'off': '0x0', 'bytes': '00'})
        patch.append({'seg': '0x3a00', 'off': '0x0', 'bytes': '00'})

        rt = defs[name]
        regs = (['ax', 'dx'] if rt in ('int32', 'uint32', 'long')
                else [] if rt == 'void' else ['ax'])
        if not regs and not obs:
            vacuous.append(name)    # void + no paired stores = nothing checks

        # Far-pointer args: fixed probe vectors fill arg words with scalars
        # whose seg half lands in unmapped space (unmapped_access aborts
        # before any real code runs).  far_arg_sites flags pair STARTS;
        # which adjacent word pair is the real ptr is ambiguous (push-run
        # order is callee-dependent), so every flagged WORD gets a
        # distinct 0x800x value — any seg:off reading then resolves to a
        # seg in 0x8000-0x8fff, all inside one mapped/seeded window.
        vecs = VECTORS
        fa = set(far_arg_sites(oimg, ohdr, f_o, e_o))
        fa |= set(far_arg_sites(cimg, chdr, f_c, e_c))
        words = sorted({w for i in fa for w in (i, i + 1)})
        if words:
            hi = words[-1]
            # any pair (0x8000+4s : 0x8000+4o) resolves to linear
            # 0x88000+0x40s+0x4o — seed the 0x400 window it lands in.
            seed = (b'PROBE-EDGE-SEED-0123456789-ABCDEF-0123456789-'
                    b'abcdef' + b'\0' * 0x80)[:0x100] * 4
            vecs = []
            for v in VECTORS:
                a = list(v) + [0] * max(0, hi + 1 - len(v))
                ps = [{'seg': '0x8000', 'off': '0x8000',
                       'bytes': seed.hex()}]
                for w in words:
                    a[w] = 0x8000 + w * 4
                vecs.append({'args': a, 'patch': ps})

        case = {'fn': name, 'exe': cfg['exe'], 'mod': cfg['mod'],
                'ds': ds, 'patch': patch, 'vectors': vecs}
        if obs:
            case['obs'] = obs
        if regs != ['ax']:
            case['regs'] = regs
        # Stub every oracle driver slot to retf unconditionally: near-BFS
        # can't see lcalls hidden behind indirect dispatch (printObjective
        # escapes at a slot with no static 0x9A in its call graph), and the
        # slotstub fixture is inert for routines that never reach one —
        # it changes only driver-ABI bytes inside DGROUP that no sane
        # routine reads as data.
        case['stub_slots'] = True
        if ints_o:
            case['int_stub'] = ints_o
        if ints_c:
            case['int_stub_c'] = ints_c
        if dsss_o:
            case['dsss_stub'] = dsss_o
        if dsss_c:
            case['dsss_stub_c'] = dsss_c
        if csites:
            case['call_stub'] = csites
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
