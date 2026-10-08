#!/usr/bin/env python3
"""Generate dosunit16 spec JSON from a compact case table.

Per case: fn, exe ('EGAME'|'START'|'SU'|'END'), mod (candidate module exe),
vectors (word args), optional:
  patch:  [(oracle_off, cand_sym_or_off, size, bytes_hex_or_None)] - state pokes;
          bytes None -> pull each side's natural image bytes (needs dgrp keys)
  obs:    same shape -> memory compared after the call
  regs:   ['ax'] default; ['ax','dx'] for int32 returns; [] to skip regs
  ds:     scratch DS (default 0x6000)
  cof:    cand_off override for functions not exported as _fn
  oof:    oracle_off override when the map name differs
"""
import json, os, re, struct, subprocess, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
F19EN = '/home/xor/games/f19/F19'
# oracle dgrp = image offset of the DATA segment (from map/<x>_en.map's
# DATA decl) — used to pull oracle image bytes for size-patches and to
# default the oracle code window over every code segment.
EXE = {'EGAME': (F19EN + '/EGAME.EXE', 'map/egame_en.map', 126192),
       'START': (F19EN + '/START.EXE', 'map/start_en.map',  65536),
       'END':   (F19EN + '/END.EXE',   'map/end_en.map',    40336),
       'SU':    (F19EN + '/SU.EXE',    'map/su_en.map',     12224)}


def cand_off(map_path, sym):
    """_sym -> dseg offset from an MSC LINK .MAP publics table."""
    pat = re.compile(r'([0-9A-Fa-f]+):([0-9A-Fa-f]+)\s+_' + re.escape(sym)
                     + r'\s*$')
    for line in open(map_path, errors='replace'):
        m = pat.search(line)
        if m:
            return int(m.group(2), 16)
    return None


def cand_dgrp(map_path):
    """image offset of DGROUP (para*16) from an MSC LINK .MAP, or None."""
    for line in open(map_path, errors='replace'):
        m = re.match(r'\s*([0-9A-Fa-f]+):0\s+DGROUP', line)
        if m:
            return int(m.group(1), 16) * 16
    return None


def _disasm_cand(mod, fn, span=0x140):
    """Disassemble routine fn inside build/<mod>.EXE (capstone, 16-bit)."""
    import capstone
    data = open(os.path.join(ROOT, 'build/%s.EXE' % mod), 'rb').read()
    hdr = struct.unpack('<H', data[8:10])[0] * 16
    off = cand_off(os.path.join(ROOT, 'build/%s.MAP' % mod), fn)
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_16)
    return list(md.disasm(data[hdr + off:hdr + off + span], off))


def rand_seed_cell(mod):
    """DS offset of the MSC CRT _rand 32-bit state — module-dependent
    (DGROUP layout differs per test exe), so derive it: _rand pushes
    [seed_hi] then [seed_lo] before its 32-bit multiply."""
    pushes = [int(re.search(r'0x[0-9a-f]+', i.op_str).group(0), 16)
              for i in _disasm_cand(mod, 'rand', 48)
              if i.mnemonic == 'push' and 'word ptr [' in i.op_str]
    return pushes[1]


def arg_lit_off(mod, fn, callee, nth=0, argn=0):
    """DS offset of the `mov ax,IMM; push ax` argument feeding the nth
    `call <callee>` inside fn — tracks string-literal offsets that move
    across relinks without editing the spec.  argn counts pushed args
    back from the call site: 0 = last push (only arg of a 1-arg call,
    or dst of strcpy/strcat), 1 = the push before it (src literal)."""
    ins = _disasm_cand(mod, fn)
    tgt = cand_off(os.path.join(ROOT, 'build/%s.MAP' % mod), callee)
    hits = []
    for i, insn in enumerate(ins):
        if (insn.mnemonic == 'call'
                and int(insn.op_str, 16) & 0xFFFF == tgt & 0xFFFF):
            vals = []
            for j in range(i - 1, max(0, i - 9), -1):
                if (ins[j].mnemonic == 'push' and ins[j].op_str == 'ax'
                        and ins[j - 1].mnemonic == 'mov'
                        and ins[j - 1].op_str.startswith('ax,')):
                    vals.append(int(ins[j - 1].op_str.split(',')[1], 16))
            if len(vals) > argn:
                hits.append(vals[argn])
    return hits[nth]


_SLOT_CACHE = {}


def overlay_slots(exe, odgrp):
    """DSEG offsets of the runtime overlay jump table: stride-5 runs of
    `EA` (jmp far) entries the drivers patch at install time.  Returns the
    byte offset of each slot — patching slot[0] to 0xCB turns it into a
    retf, matching the candidate build's compiled empty-thunk stubs."""
    key = (exe, odgrp)
    if key not in _SLOT_CACHE:
        data = open(exe, 'rb').read()
        hdr = struct.unpack('<H', data[8:10])[0] * 16
        slots, i, end = [], odgrp, len(data) - hdr
        while i < end - 5:
            if data[hdr + i] == 0xEA:
                j = i
                while j + 5 <= end and data[hdr + j] == 0xEA:
                    j += 5
                if (j - i) // 5 >= 3:
                    slots.extend(range(i, j, 5))
                i = j
            else:
                i += 1
        _SLOT_CACHE[key] = slots
    return _SLOT_CACHE[key]


def slot_stub_exe(exe, odgrp):
    """Copy of the oracle exe with every runtime driver slot patched to
    `xor ax,ax; retf` (31 c0 cb 90 90) — the load-image equivalent of the
    candidate's compiled empty-thunk stubs, which return ax=0.  Patches
    can't do this (dosunit refuses memory writes over declared code); the
    bytes must ride in via the image.  Cached under build/."""
    out = os.path.join(ROOT,
                       'build/%s-slotstub.EXE' % os.path.basename(exe)[:-4])
    if not os.path.exists(out):
        data = bytearray(open(exe, 'rb').read())
        hdr = struct.unpack('<H', data[8:10])[0] * 16
        slots = overlay_slots(exe, odgrp)
        for s in slots:
            data[hdr + s:hdr + s + 5] = b'\x31\xc0\xcb\x90\x90'
        _drop_relocs_overlapping(data, [(s, 5) for s in slots])
        open(out, 'wb').write(bytes(data))
    return out


def int_stub_exe(exe, base, sites):
    """Copy of the oracle exe with each `int NN` (CD xx) site the routine's
    near-call graph reaches patched to `xor ax,ax` (31 c0) — a bounded
    "service succeeded, ax=0" answer matching the cand build's
    zero-returning int21/file helpers.  Fixture (not a vector patch)
    because declared instruction bytes are immutable.  `base` is an
    already-derived fixture path (e.g. slotstub) or the original exe.
    Cached per unique (base content, site list) pair — the test-exe
    candidates relink on every source change, so the tag must cover the
    input bytes or a stale image would be replayed with shifted data
    offsets."""
    import hashlib
    src = open(base, 'rb').read()
    tag = hashlib.md5(src + b''.join(s.to_bytes(2, 'little')
                                     for s in sites)).hexdigest()[:8]
    out = os.path.join(ROOT, 'build/%s-ints-%s.EXE'
                       % (os.path.basename(exe)[:-4], tag))
    if not os.path.exists(out):
        data = bytearray(src)
        hdr = struct.unpack('<H', data[8:10])[0] * 16
        for s in sites:
            assert data[hdr + s] == 0xCD, \
                '%s@%#x byte=%#x' % (base, s, data[hdr + s])
            data[hdr + s:hdr + s + 2] = b'\x31\xc0'
        _drop_relocs_overlapping(data, [(s, 2) for s in sites])
        open(out, 'wb').write(bytes(data))
    return out


def dsss_stub_exe(exe, base, sites):
    """Copy of `base` with reachable `ds := ss` writes nopped out —
    `push ss;pop ds` and `mov r16,ss; mov ds,r16` are DGROUP-restore
    no-ops under real DOS (DS==SS), but under the probe DS is the scratch
    segment while SS is the stack window, so surviving writes land in the
    wrong space.  Nopping keeps DS on DGROUP, matching the intent.
    (site, bytes) pairs, cached per (base content, site list)."""
    import hashlib
    src = open(base, 'rb').read()
    tag = hashlib.md5(src + b'dsss' + b''.join(
        s.to_bytes(2, 'little') + p for s, p in sites)).hexdigest()[:8]
    out = os.path.join(ROOT, 'build/%s-dsss-%s.EXE'
                       % (os.path.basename(exe)[:-4], tag))
    if not os.path.exists(out):
        data = bytearray(src)
        hdr = struct.unpack('<H', data[8:10])[0] * 16
        for s, p in sites:
            assert data[hdr + s] in (0x16, 0x8E), \
                '%s@%#x byte=%#x' % (base, s, data[hdr + s])
            data[hdr + s:hdr + s + len(p)] = p
        _drop_relocs_overlapping(data, [(s, len(p)) for s, p in sites])
        open(out, 'wb').write(bytes(data))
    return out


def _drop_relocs_overlapping(data, spans):
    """Remove MZ relocation entries whose 2-byte target word intersects a
    patched span — the loader still adds the load segment to that word, so
    fixture bytes under a reloc site would be rewritten at load time and
    desync the instruction stream (e.g. an `lcall`'s seg field nop'd to
    90 90 loads as 90 a0, and `a0` swallows the next two bytes)."""
    nrel = struct.unpack('<H', data[6:8])[0]
    rp = struct.unpack('<H', data[24:26])[0]
    keep = []
    for i in range(nrel):
        off, seg = struct.unpack('<HH', data[rp + 4 * i:rp + 4 * i + 4])
        lin = seg * 16 + off            # image offset of the reloc'd word
        if any(s - 1 <= lin <= s + n - 1 for s, n in spans):
            continue
        keep.append((off, seg))
    if len(keep) != nrel:
        for i, (off, seg) in enumerate(keep):
            struct.pack_into('<HH', data, rp + 4 * i, off, seg)
        struct.pack_into('<H', data, 6, len(keep))


def call_stub_exe(exe, base, sites):
    """Copy of `base` with each `call rel16`/`lcall seg:off` site whose
    callee the candidate test exe supplies as a trivial stub patched to
    the stub's observable answer — nop fill for empty bodies, `xor ax,ax`
    for `return 0`, `mov ax,imm` for constant returns (sites carry their
    precomputed replacement bytes).  Symmetric with int_stub_exe: the
    probe must not run oracle code the candidate does not have.  Fixture
    (not a vector patch) because declared instruction bytes are
    immutable.  Cached per unique (base content, site list) pair."""
    import hashlib
    src = open(base, 'rb').read()
    tag = hashlib.md5(src + b'v2' + b''.join(
        s.to_bytes(2, 'little') + p for s, p in sites)).hexdigest()[:8]
    out = os.path.join(ROOT, 'build/%s-calls-%s.EXE'
                       % (os.path.basename(exe)[:-4], tag))
    if not os.path.exists(out):
        data = bytearray(src)
        hdr = struct.unpack('<H', data[8:10])[0] * 16
        for s, p in sites:
            want = 5 if data[hdr + s] == 0x9A else 3
            assert data[hdr + s] in (0xE8, 0x9A), \
                '%s@%#x byte=%#x' % (base, s, data[hdr + s])
            assert len(p) == want, '%s@%#x patch %d != %d' % (base, s, len(p), want)
            data[hdr + s:hdr + s + want] = p
        _drop_relocs_overlapping(
            data, [(s, len(p)) for s, p in sites])
        open(out, 'wb').write(bytes(data))
    return out


def emit(cases, out_path):
    spec = []
    for c in cases:
        exe, omap, odgrp = EXE[c['exe']]
        mod = c['mod']
        cmap = 'build/%s.MAP' % mod
        # candidate dgrp = its image's DGROUP para*16 - resolve from MAP
        cd = c.get('cand_dgrp')
        if cd is None:
            cd = cand_dgrp(os.path.join(ROOT, cmap))
        # code window covers everything below DGROUP — tracks relinks
        code_c = c.get('code_c', [[0, cd]]) if cd is not None \
            else c.get('code_c', [[0, 62912]])
        case = {'fn': c['fn'], 'oracle_exe': exe, 'oracle_map': omap,
                'cand_exe': 'build/%s.EXE' % mod, 'cand_map': cmap,
                'ds': c.get('ds', '0x6000'),
                'code_o': c.get('code_o', [[0, odgrp or 64896]]),
                'code_c': code_c}
        if odgrp is not None:
            case['oracle_dgrp'] = odgrp
        if cd is not None:
            case['cand_dgrp'] = cd
        if 'oof' in c:
            case['oracle_off'] = c['oof']
        if 'cof' in c:
            case['cand_off'] = c['cof']
        if 'regs' in c:
            case['check_regs'] = c['regs']

        def cells(lst):
            out = []
            for e in lst:
                if isinstance(e, dict):   # raw {'seg','off','bytes'} escape
                    out.append(e)
                    continue
                o_off, csym, size = e[0], e[1], e[2]
                c_off = None
                if csym is not None:
                    c_off = csym if isinstance(csym, int) else \
                        cand_off(os.path.join(ROOT, cmap), csym)
                    if c_off is None:
                        print('MISS %s.%s: no _%s in %s' % (mod, c['fn'], csym, cmap))
                        c_off = 0xFFFF
                d = {}
                if o_off is not None:
                    d['o_off'] = hex(o_off)
                if c_off is not None:
                    d['c_off'] = hex(c_off)
                if len(e) > 3 and e[3] is not None:
                    d['bytes'] = e[3]
                else:
                    d['size'] = size
                out.append(d)
            return out

        if 'patch' in c:
            case['patches'] = cells(c['patch'])
        if c.get('stub_slots'):
            # runtime driver slots -> retf via a pre-patched oracle image
            # (the candidate's thunk stubs already no-op).  code_o must
            # also cover the slot table or the lcall still trips
            # fetch_outside_declared_code.
            slots = overlay_slots(exe, odgrp)
            case['oracle_exe'] = slot_stub_exe(exe, odgrp)
            case['code_o'] = [[0, odgrp],
                              [min(slots), max(slots) + 5 - min(slots)]]
        if c.get('int_stub'):
            # reachable `int NN` sites -> xor ax,ax via a prepatched oracle
            # image (bounded "service ok, ax=0" — cand helpers already
            # return 0).  Sites are image offsets; all in-extent so the
            # existing code_o window already covers them.
            try:
                case['oracle_exe'] = int_stub_exe(
                    exe, case.get('oracle_exe', exe), c['int_stub'])
            except AssertionError as e:
                raise AssertionError('%s: %s' % (c['fn'], e))
        if c.get('int_stub_c'):
            # symmetric stub for the candidate test exe: its linked
            # skeleton/CRT helpers carry their own int NN sites.
            cexe = os.path.join(ROOT, 'build/%s.EXE' % mod)
            try:
                case['cand_exe'] = int_stub_exe(
                    cexe, case.get('cand_exe', cexe), c['int_stub_c'])
            except AssertionError as e:
                raise AssertionError('%s(c): %s' % (c['fn'], e))
        if c.get('call_stub'):
            # calls into cand-side trivial stubs -> precomputed nop/xor/mov
            # answers (unimplemented overlay routines, isr installers, dos
            # helpers the cand build leaves empty).  (site, bytes) pairs.
            try:
                case['oracle_exe'] = call_stub_exe(
                    exe, case.get('oracle_exe', exe), c['call_stub'])
            except AssertionError as e:
                raise AssertionError('%s: %s' % (c['fn'], e))
        if c.get('dsss_stub'):
            # `ds := ss` writes in the oracle graph -> nops: DS==SS only
            # holds in real DOS; under the split scratch-DS/stack sandbox
            # the write clobbers DGROUP with the SS window.
            try:
                case['oracle_exe'] = dsss_stub_exe(
                    exe, case.get('oracle_exe', exe), c['dsss_stub'])
            except AssertionError as e:
                raise AssertionError('%s: %s' % (c['fn'], e))
        if c.get('dsss_stub_c'):
            cexe = os.path.join(ROOT, 'build/%s.EXE' % mod)
            try:
                case['cand_exe'] = dsss_stub_exe(
                    cexe, case.get('cand_exe', cexe), c['dsss_stub_c'])
            except AssertionError as e:
                raise AssertionError('%s(c): %s' % (c['fn'], e))
        if 'obs' in c:
            case['observe'] = cells(c['obs'])
        vecs = []
        for v in c['vectors']:
            if isinstance(v, dict):
                nv = {'args': v['args']}
                if 'patch' in v:
                    nv['patches'] = cells(v['patch'])
                if 'obs' in v:
                    nv['observe'] = cells(v['obs'])
                vecs.append(nv)
            else:
                vecs.append(v)
        case['vectors'] = vecs
        spec.append(case)
    json.dump(spec, open(out_path, 'w'), indent=1)
    print('wrote %s (%d cases)' % (out_path, len(spec)))
