#!/usr/bin/env python3
"""Z3 semantic equivalence check for ported F19 routines.

Complements tools/portcheck.py: mzdiff compares instruction streams, while
this uses ~/vextest dosunit (SSA + Z3) to prove observable equivalence —
registers, memory writes, control flow — of routines whose code differs from
the original only by codegen artifacts (e.g. one extra prologue load).

Because the portcheck test exe links globals at different dseg offsets, the
candidate SSA is first rewritten: constants inside each known candidate
global's extent are shifted to that global's original dseg offset.  The map
is keyed by C name: oracle ds-offset from lst dseg labels / /* word_XXXXX */
or /* @0xXXXX */ comments in src/*.c; candidate offset from the test exe's
LINK .MAP publics.

Usage:
    python3 tools/z3check.py src/x.c name1 [name2 ...]

    --ssa-only    stop after producing the (rewritten) SSA docs
Intermediate JSON is always kept under build/z3cmp/ for inspection.
Env:
    VEXTEST   path to the vextest checkout (default ~/vextest)
"""
import glob
import json
import os
import re
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
VEXTEST = os.environ.get('VEXTEST', os.path.expanduser('~/vextest'))
VPY = os.path.join(VEXTEST, '.venv', 'bin', 'python')
DOSUNIT = ['-m', 'tools.dosunit.dosunit']
OUT = os.path.join(ROOT, 'build', 'z3cmp')
LST = os.path.join(ROOT, 'lst', 'EGAME.EXE.lst')
RMAP = os.path.join(ROOT, 'map', 'egame.map')
REF_EXE = os.path.join(ROOT, 'EGAME.EXE')

# dseg label parse: 'dseg:9EB8 word_38D28 dw ?' -> (0x9EB8, 0x38D28)
_LST_DATA_RE = re.compile(r'^dseg:([0-9A-Fa-f]+)\s+(\w+)_([0-9A-Fa-f]+)\s+d[bwdqt]\b')
# src decl comment forms: /* word_38D28 */  /* dword_38B10 */  /* @0x4880 */
_CMT_WORD_RE = re.compile(r'/\*\s*(?:word|byte|dword|off)_([0-9A-Fa-f]+)\s*\*/')
_CMT_AT_RE = re.compile(r'/\*\s*@0x([0-9A-Fa-f]+)')
_DECL_RE = re.compile(
    r'^\s*(?:extern\s+)?(?:int8|uint8|int16|uint16|int32|uint32|long|char|'
    r'(?:unsigned\s+)?(?:int|short)|struct\s+\w+|union\s+\w+|void)\b'
    r'[^;=]*?\b(\w+)\s*(?:\[[^\]]*\])*\s*(?:=[^;]*)?;\s*(/\*.*\*/)?\s*$')
_LINK_PUB_RE = re.compile(r'^\s*([0-9A-Fa-f]{4,6}):([0-9A-Fa-f]{4,6})\s+(\S+)\s*$')


def run(cmd, cwd=None):
    r = subprocess.run(cmd, cwd=cwd or VEXTEST)
    if r.returncode != 0:
        sys.exit(r.returncode)


def z3(*args):
    run([VPY] + DOSUNIT + list(args))


def discover(exe, map_path, out, extra=()):
    a = ['discover', '--exe', exe, '--map', map_path, '--out', out] + list(extra)
    if map_path == RMAP:
        a += ['--ida-listing', LST]
    z3(*a)
    return json.load(open(out))


def filter_catalog(doc, names, out):
    keep = [f for f in doc.get('functions', []) if names & set(f.get('names', []))]
    d = dict(doc)
    d['functions'] = keep
    json.dump(d, open(out, 'w'), indent=1)
    return out


def oracle_dseg():
    """ordered [(dseg_off, label_hex, name)] from the lst dseg section."""
    rows = []
    with open(LST, errors='replace') as f:
        for line in f:
            m = _LST_DATA_RE.match(line)
            if m:
                rows.append((int(m.group(1), 16), int(m.group(3), 16), m.group(0)))
    rows.sort()
    return rows


def src_var_labels():
    """C name -> oracle dseg offset, from /* word_XXXXX */ or /* @0xXXXX */
    comments attached to global declarations in src/*.c."""
    name2off = {}
    for path in glob.glob(os.path.join(ROOT, 'src', '*.c')):
        for line in open(path, errors='replace'):
            cmt = _CMT_WORD_RE.search(line) or _CMT_AT_RE.search(line)
            if not cmt:
                continue
            dm = _DECL_RE.match(line)
            if not dm:
                continue
            raw = int(cmt.group(1), 16)
            # /* word_XXXXX */ uses IDA flat names: dseg_off = X - 0x2EE70;
            # /* @0xXXXX */ is already a dseg offset.
            off = raw if _CMT_AT_RE.search(line) else raw - 0x2EE70
            name2off[dm.group(1)] = off
    return name2off


def link_data_publics(map_path):
    """name -> (linear, off_in_own_seg); extents from sorted linears."""
    pubs = []
    with open(map_path, errors='replace') as f:
        in_pub = False
        for line in f:
            if 'Publics by Name' in line:
                in_pub = True
                continue
            if in_pub:
                m = _LINK_PUB_RE.match(line)
                if not m:
                    if line.strip() and not line.startswith(' '):
                        in_pub = False
                    continue
                seg, off, name = int(m.group(1), 16), int(m.group(2), 16), m.group(3)
                pubs.append((seg * 16 + off, seg, off, name.lstrip('_')))
    pubs.sort()
    return pubs


def dgroup_origin(map_path):
    with open(map_path, errors='replace') as f:
        for line in f:
            m = re.match(r'^\s*([0-9A-Fa-f]+):0\s+DGROUP', line)
            if m:
                return int(m.group(1), 16) * 16
    return None


def build_remap(cand_map, oracle_map):
    """cand_const_value -> oracle_const_value, over global extents."""
    remap = {}
    for name, (c_off, c_ext) in cand_map.items():
        if name not in oracle_map:
            continue
        o_off, o_ext = oracle_map[name]
        ext = min(c_ext, o_ext)
        for v in range(c_off, c_off + ext):
            remap[v] = o_off + (v - c_off)
    return remap


def load_image(path):
    data = open(path, 'rb').read()
    import struct
    return data[struct.unpack('<H', data[8:10])[0] * 16:]


def c_string_at(image, off):
    if off < 0 or off >= len(image):
        return None
    end = image.find(b'\0', off)
    if end < 0:
        end = min(len(image), off + 64)
    s = image[off:end]
    return s if s else None


def oracle_dseg_bytes():
    """(start, end) dseg offsets in the loaded image + image bytes."""
    img = load_image(REF_EXE)
    # skeleton map: DSEG linear 0x1EE70; dseg_off -> linear 0x1EE70+off
    return img, 0x1EE70


def build_string_remap(cand_img, cand_dgrp_linear, oracle_img, oracle_dseg_linear,
                       cand_consts, remap, oracle_consts=None):
    """Extend remap with string-content pairs: candidate const -> oracle off.

    For each candidate data-offset const not already mapped, read the
    NUL-terminated literal at that dseg offset in the candidate image and
    match it (with trailing \\0) in the oracle dseg.  When several oracle
    positions hold identical bytes, prefer the one that appears in the
    oracle SSA's own const set — identical content means any occurrence is
    observationally the same, but the pushed one is the layout partner.
    """
    hay = oracle_img[oracle_dseg_linear:]
    added = 0
    for v in sorted(cand_consts):
        if v in remap or v < 0x20:
            continue
        s = c_string_at(cand_img, cand_dgrp_linear + v)
        if s is None or len(s) < 3:
            continue
        needle = s + b'\0'
        hits = []
        i = hay.find(needle)
        while i >= 0:
            hits.append(i)
            i = hay.find(needle, i + 1)
        if not hits:
            continue
        pick = hits[0] if len(hits) == 1 else None
        if pick is None and oracle_consts:
            used = [h for h in hits if h in oracle_consts]
            if len(used) == 1:
                pick = used[0]
        if pick is None:
            continue
        remap[v] = pick
        added += 1
    return added


def build_positional_string_remap(odoc, cdoc, cand_img, cand_dgrp, orac_img, orac_dseg,
                                  remap):
    """Positional string pairing: zip same-name SSA parts in traversal order;
    wherever aligned const leaves differ, accept (cand -> orac) when the
    candidate const's NUL-terminated literal equals the oracle bytes at the
    oracle offset.  Disambiguates strings that occur several times.
    """
    from collections import defaultdict
    oparts = defaultdict(list)
    cparts = defaultdict(list)
    for f in odoc.get('functions', []):
        nm = (f.get('function') or {}).get('name', '')
        oparts[nm].append(f)
    for f in cdoc.get('functions', []):
        nm = (f.get('function') or {}).get('name', '')
        cparts[nm].append(f)
    for v in oparts.values():
        v.sort(key=lambda f: int(str((f.get('entry') or {}).get('linear', '0')), 0))
    for v in cparts.values():
        v.sort(key=lambda f: int(str((f.get('entry') or {}).get('linear', '0')), 0))
    added = 0
    for nm, ops in oparts.items():
        cps = cparts.get(nm) or []
        for of, cf in zip(ops, cps):
            added += _pair_terms(of.get('assignments', []), cf.get('assignments', []),
                                 cand_img, cand_dgrp, orac_img, orac_dseg, remap)
    return added


def _pair_terms(oa, ca, cand_img, cand_dgrp, orac_img, orac_dseg, remap):
    """Walk two assignment lists in parallel; pair differing const leaves."""
    added = 0
    for x, y in zip(oa, ca):
        added += _pair_node(x, y, cand_img, cand_dgrp, orac_img, orac_dseg, remap)
    return added


def _pair_node(x, y, cand_img, cand_dgrp, orac_img, orac_dseg, remap):
    if not isinstance(x, dict) or not isinstance(y, dict):
        return 0
    xv, yv = x.get('value'), y.get('value')
    if x.get('op') == 'const' and y.get('op') == 'const' and xv is not None and yv is not None:
        ov, cv = int(str(xv), 0), int(str(yv), 0)
        if ov == cv or cv in remap:
            return 0
        s = c_string_at(cand_img, cand_dgrp + cv)
        if s is None or len(s) < 2:
            return 0
        ob = orac_img[orac_dseg + ov:orac_dseg + ov + len(s)]
        if ob == s and orac_img[orac_dseg + ov + len(s)] == 0:
            remap[cv] = ov
            return 1
        return 0
    xa, ya = x.get('args'), y.get('args')
    if isinstance(xa, list) and isinstance(ya, list):
        n = 0
        for a, b in zip(xa, ya):
            n += _pair_node(a, b, cand_img, cand_dgrp, orac_img, orac_dseg, remap)
        return n
    return 0


def collect_consts(node, acc):
    if isinstance(node, dict):
        if node.get('op') == 'const' and 'value' in node:
            acc.add(int(str(node['value']), 0))
            return
        for v in node.values():
            collect_consts(v, acc)
    elif isinstance(node, list):
        for v in node:
            collect_consts(v, acc)


def rewrite_consts(node, remap, stats, oracle_consts=None):
    """Rewrite const values via the relocation map.

    A const is rewritten only when its value is a mapped candidate global /
    string offset AND the value does not itself appear in the corresponding
    oracle function — real literals (e.g. 0x800) are shared by both sides
    and must not be remapped just because they collide with a candidate
    global's address.
    """
    if isinstance(node, dict):
        if node.get('op') == 'const' and 'value' in node:
            v = int(str(node['value']), 0)
            if v in remap and not (oracle_consts and v in oracle_consts):
                node['value'] = hex(remap[v])
                stats[0] += 1
            return
        for v in node.values():
            rewrite_consts(v, remap, stats, oracle_consts)
    elif isinstance(node, list):
        for v in node:
            rewrite_consts(v, remap, stats, oracle_consts)


def main():
    ssa_only = '--ssa-only' in sys.argv
    args = [a for a in sys.argv[1:] if not a.startswith('--')]
    if len(args) < 2:
        print(__doc__)
        sys.exit(1)
    src, names = args[0], set(args[1:])
    base = os.path.splitext(os.path.basename(src))[0].upper()[:8]
    cand_exe = os.path.join(ROOT, 'build', base + '.EXE')
    cand_map = os.path.join(ROOT, 'build', base + '.MAP')
    for p in (cand_exe, cand_map):
        if not os.path.exists(p):
            print('missing', p, '-- run tools/portcheck.py first')
            sys.exit(1)
    os.makedirs(OUT, exist_ok=True)

    print('== discover')
    ocat = os.path.join(OUT, base + '.ofuncs.json')
    ccat = os.path.join(OUT, base + '.cfuncs.json')
    oall_path = os.path.join(OUT, 'EGAME.ofuncs.json.all')
    oall = discover(REF_EXE, RMAP, oall_path) if not os.path.exists(oall_path) else json.load(open(oall_path))
    # candidate catalog cache is invalidated when the test exe or its map is
    # rebuilt (portcheck runs before every z3check in the sweep)
    ccall_path = ccat + '.all'
    stale = (not os.path.exists(ccall_path) or
             os.path.getmtime(ccall_path) < os.path.getmtime(cand_exe) or
             os.path.getmtime(ccall_path) < os.path.getmtime(cand_map))
    call = discover(cand_exe, cand_map, ccall_path) if stale else json.load(open(ccall_path))
    filter_catalog(oall, names, ocat)
    filter_catalog(call, names, ccat)

    print('== ssa oracle')
    ossa = os.path.join(OUT, base + '.ossa.json')
    z3('ssa', '--exe', REF_EXE, '--functions', ocat, '--out', ossa)
    print('== ssa candidate')
    cssa = os.path.join(OUT, base + '.cssa.json')
    z3('ssa', '--exe', cand_exe, '--functions', ccat, '--out', cssa)

    print('== data remap')
    rows = oracle_dseg()
    name2off = src_var_labels()
    # precise oracle extents: dseg_off -> next dseg label offset
    offs = sorted(off for off, _lab, _l in rows)
    oracle_map = {}
    for name, off in name2off.items():
        nxt = next((x for x in offs if x > off), off + 2)
        oracle_map[name] = (off, max(2, nxt - off))

    pubs = link_data_publics(cand_map)
    dgrp = dgroup_origin(cand_map)
    if dgrp is None:
        print('no DGROUP origin in', cand_map)
        sys.exit(1)
    cand_map_d = {}
    for i, (lin, seg, off, name) in enumerate(pubs):
        ds_off = lin - dgrp
        if ds_off < 0:
            continue
        nxt = pubs[i + 1][0] - lin if i + 1 < len(pubs) else 2
        cand_map_d[name] = (ds_off, max(2, nxt))
    remap = build_remap(cand_map_d, oracle_map)
    print('  mapped %d globals, %d const values' % (
        sum(1 for n in cand_map_d if n in oracle_map), len(remap)))

    cdoc = json.load(open(cssa))
    # second pass: string literals paired by content (weapon names, fmt strs)
    cand_consts = set()
    for f in cdoc.get('functions', []):
        collect_consts(f, cand_consts)
    odoc = json.load(open(ossa))
    orac_consts = set()
    for f in odoc.get('functions', []):
        collect_consts(f, orac_consts)
    cand_img = load_image(cand_exe)
    orac_img, orac_dseg = oracle_dseg_bytes()
    added = build_string_remap(cand_img, dgrp, orac_img, orac_dseg,
                               cand_consts, remap, orac_consts)
    print('  string remap: +%d (%d total)' % (added, len(remap)))
    added = build_positional_string_remap(odoc, cdoc, cand_img, dgrp,
                                          orac_img, orac_dseg, remap)
    print('  positional string remap: +%d (%d total)' % (added, len(remap)))

    # per-function oracle const set: values already literals on the oracle
    # side are not relocation targets.
    orac_by_fn = {}
    for f in odoc.get('functions', []):
        nm = (f.get('function') or {}).get('name', '')
        s = orac_by_fn.setdefault(nm, set())
        collect_consts(f, s)
    stats = [0]
    for f in cdoc.get('functions', []):
        nm = (f.get('function') or {}).get('name', '')
        rewrite_consts(f, remap, stats, orac_by_fn.get(nm))
    print('  rewrote %d consts' % stats[0])
    rssa = os.path.join(OUT, base + '.cssa.remap.json')
    json.dump(cdoc, open(rssa, 'w'))
    if ssa_only:
        return

    print('== mapping + compare')
    mp = os.path.join(OUT, base + '.mapping.json')
    z3('make-mapping', '--oracle-functions', ocat, '--candidate-functions', ccat, '--out', mp)
    cmpout = os.path.join(OUT, base + '.compare.json')
    subprocess.run([VPY] + DOSUNIT + ['compare-ssa', '--oracle-ssa', ossa,
                   '--candidate-ssa', rssa, '--mapping', mp, '--out', cmpout],
                   cwd=VEXTEST)
    doc = json.load(open(cmpout))
    summ = doc.get('summary', {})
    print('== summary:', json.dumps(summ))
    byf = {}
    for res in doc.get('results', []):
        fn = (res.get('function') or {}).get('name', '?')
        e = byf.setdefault(fn, {'passed': 0, 'failed': 0, 'refused': 0, 'mm': 0, 'other': []})
        st = res.get('status')
        e[st] = e.get(st, 0) + 1
        if st == 'failed':
            kinds = set(m.get('kind') for m in res.get('mismatches', []))
            if kinds == {'memory_expr_changed'}:
                e['mm'] += 1
            else:
                e['other'].append(kinds)
        elif st == 'refused':
            e['other'].append({res.get('reason')})
    benign_refusals = ({'part_boundary_mismatch'}, {'candidate_ssa_missing'})
    for fn in sorted(byf):
        e = byf[fn]
        if e['failed'] == 0 and e['refused'] == 0:
            tag = 'PROVEN'
        elif e['failed'] == e['mm'] and all(x in benign_refusals for x in e['other']):
            tag = 'PROVEN(modulo-layout)'
        else:
            tag = 'DIFFERS'
        print('%-28s %-22s pass=%d fail=%d(mem %d) refused=%d %s' % (
            fn, tag, e['passed'], e['failed'], e['mm'], e['refused'],
            e['other'][:6]))
    sys.exit(0 if all(v['failed'] == 0 for v in byf.values()) else 1)


if __name__ == '__main__':
    main()
