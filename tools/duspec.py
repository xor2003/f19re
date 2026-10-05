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
