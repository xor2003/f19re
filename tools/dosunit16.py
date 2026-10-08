#!/usr/bin/env python3
"""Replay F-19 routines under dosunit real16: original EXE vs MSC port test-EXE.

Builds a replay-real16 manifest from mzmap/link-map routine addresses, runs
~/vextest's dosunit, and reports the per-vector oracle result plus a
register/observation verdict of our own (the tool's strict verdict also
compares raw writes, which always diverge on pushed code addresses).

Geometry: both images load at the same paragraph L=0x1000 and the entry
segment is the image base itself (CS=L) so `cs:[X]` reads image offset X
exactly as real DOS does — jump-table switches and cs-relative literal
reads depend on this.  The frame declares `sp_guard`: RETURNED is
recognized by a ret-family fetch while SP still points at the synthetic
caller frame, so the trap target CS:0 (image offset 0) is only the pushed
return value and is never required to be fetchable — an in-image trap
would be refused for images >=64K that span every in-CS offset.

  oracle_entry = {L, F_o}   candidate_entry = {L, F_c}

DS globals: both sides share one scratch DS segment; each side reads whatever
cells its own code addresses point at, so every referenced cell is patched
at the *effective* linear each side reaches:
  oracle cell  ds:o_off -> linear DS*16 + o_off  (patch with oracle bytes)
  cand cell    ds:c_off -> linear DS*16 + c_off  (patch with cand bytes)
Usually DS = a free segment so neither image is disturbed; when the oracle
should read its real DGROUP data instead, set ds=None to use the oracle's
natural DGROUP paragraph (patching then only applies to candidate cells).

Args are cdecl words at ss:sp+2, sp+4, ... (memory order = arg order).
"""

import json
import os
import re
import struct
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
VEXTEST = os.environ.get('VEXTEST', os.path.expanduser('~/vextest'))
VPY = os.path.join(VEXTEST, '.venv', 'bin', 'python')
F19EN = os.environ.get('F19EN', '/home/xor/games/f19/F19')

LOAD = 0x1000          # both images at linear 0x10000; entry CS = LOAD so
                       # cs:[X] reads image offset X like a real DOS load
SS = 0x4000            # scratch stack segment (its 64KB window is mapped)
DS = 0x6000            # scratch data segment
SP = 0xFFE0            # near window top — chkstk guards (e.g. STGEN's
                       # 0xef60 limit cell) trip when the frame leaves the
                       # callee < ~0xa0 of stack; real DOS gives ~64K
TRAP_OFF = 0x0000      # pushed return offset (CS:0 = img 0); under sp_guard
                       # the ret fetch is intercepted before this is fetched


def load_image(path):
    """MZ load image: file bytes after the header (as linked offsets)."""
    data = open(path, 'rb').read()
    return data[struct.unpack('<H', data[8:10])[0] * 16:]


def routine_extent(name, map_path):
    """mzmap routine -> image offset (lo) using its declared extent."""
    segs = {}
    with open(map_path, errors='replace') as f:
        for line in f:
            m = re.match(r'([\w.$]+) (CODE|DATA|STACK) ([0-9a-fA-F]+)', line.strip())
            if m:
                segs[m.group(1)] = int(m.group(3), 16) * 16
                continue
            m = re.match(r'([\w.$]+): ([\w.$]+) (NEAR|FAR) ([0-9a-fA-F]+)-([0-9a-fA-F]+)', line.strip())
            if m and m.group(1) == name:
                return segs[m.group(2)] + int(m.group(4), 16)
    return None


def public_offset(name, linkmap_path):
    """_name -> image offset from an MSC LINK .MAP publics table."""
    pat = re.compile(r'([0-9A-Fa-f]+):([0-9A-Fa-f]+)\s+_' + re.escape(name) + r'\s*$')
    with open(linkmap_path, errors='replace') as f:
        for line in f:
            m = pat.search(line)
            if m:
                return int(m.group(1), 16) * 16 + int(m.group(2), 16)
    return None


def map_dgrp(linkmap_path):
    """image offset of DGROUP from an MSC LINK .MAP, or None (routine maps)."""
    try:
        with open(linkmap_path, errors='replace') as f:
            for line in f:
                m = re.match(r'\s*([0-9A-Fa-f]+):0\s+DGROUP', line)
                if m:
                    return int(m.group(1), 16) * 16
    except OSError:
        pass
    return None


def u16hex(v):
    return hex(v & 0xFFFF)


def img_slice(path, img_off, size):
    if not hasattr(img_slice, '_c'):
        img_slice._c = {}
    if path not in img_slice._c:
        img_slice._c[path] = load_image(path)
    return img_slice._c[path][img_off:img_off + size]


def build_vector(vid, f_o, f_c, args, case, ds_seg, flags='0x0202'):
    """One near16 vector: entries at each side's own image offset.

    Patches: {'seg'? -> ds, 'off'|'o_off'|'c_off', 'bytes' hex | 'size' N}
    With 'size' and no 'bytes', bytes are pulled from that side's own image
    at (dgrp_img_off + off) — replays the side's natural dseg content.

    Shared-DS collisions: an oracle cell and a cand cell may carry the same
    numeric offset while intending different content.  Small (explicit
    scalar) writes apply after large (span) writes so explicit seeds win;
    bytes contested by conflicting patch intents are removed from the
    paired-observation contract (the seed can't satisfy both sides there).
    """
    writes = []          # (seg, off, bytes, pulled)
    sp = SP
    for i, w in enumerate(args):
        writes.append((SS, sp + 2 + 2 * i,
                       struct.pack('<H', w & 0xFFFF), False))
    for p in case.get('patches', []):
        seg = int(str(p.get('seg', ds_seg)), 0)
        offs = []
        if 'off' in p:
            offs.append((int(str(p['off']), 0), None))
        else:
            if 'o_off' in p:
                offs.append((int(str(p['o_off']), 0), 'oracle'))
            if 'c_off' in p:
                offs.append((int(str(p['c_off']), 0), 'candidate'))
        for off, side in offs:
            b = p.get('bytes')
            pulled = b is None
            if pulled:
                exe = case['oracle_exe'] if side == 'oracle' \
                    else case['cand_exe']
                dgrp = case['oracle_dgrp'] if side == 'oracle' \
                    else case['cand_dgrp']
                b = img_slice(exe, dgrp + off, int(str(p['size']), 0)).hex()
            writes.append((seg, off, bytes.fromhex(b), pulled))
    # natural-image pulls first, then explicit seeds big->small: explicit
    # scalar cells outrank span bleed; listed order keeps ties stable
    order = sorted(range(len(writes)),
                   key=lambda i: (not writes[i][3], -len(writes[i][2])))
    last = {}            # linear -> final byte value
    contested = set()    # linears where patch intents disagreed
    for i in order:
        seg, off, b, _ = writes[i]
        for k, by in enumerate(b):
            lin = seg * 16 + off + k
            if lin in last and last[lin] != by:
                contested.add(lin)
            last[lin] = by
    mem = [{'segment': u16hex(writes[i][0]),
            'offset': u16hex(writes[i][1]),
            'bytes': writes[i][2].hex()} for i in order]

    obs = []
    pairs = []          # (idx_in_oracle, idx_in_candidate) to compare

    def emit_obs(oseg, ooff, cseg, coff, size):
        """Paired obs split around contested bytes (either side)."""
        run = 0
        for k in range(size + 1):
            bad = k < size and (oseg * 16 + ooff + k in contested
                                or cseg * 16 + coff + k in contested)
            if k == size or bad:
                if run:
                    obs.append({'segment': u16hex(oseg),
                                'offset': u16hex(ooff + k - run),
                                'size': run})
                    obs.append({'segment': u16hex(cseg),
                                'offset': u16hex(coff + k - run),
                                'size': run})
                    pairs.append((len(obs) - 2, len(obs) - 1))
                run = 0
            else:
                run += 1

    for o in case.get('observe', []):
        if 'off' in o:                      # same seg:off on both sides
            emit_obs(int(str(o.get('seg', ds_seg)), 0),
                     int(str(o['off']), 0),
                     int(str(o.get('seg', ds_seg)), 0),
                     int(str(o['off']), 0), o['size'])
        else:                               # per-side cells: o_off vs c_off
            seg = int(str(o.get('seg', ds_seg)), 0)
            emit_obs(seg, int(str(o['o_off']), 0),
                     seg, int(str(o['c_off']), 0), o['size'])
    vec = {
        'id': vid,
        'oracle_entry': {'segment': u16hex(LOAD), 'offset': u16hex(f_o)},
        'candidate_entry': {'segment': u16hex(LOAD), 'offset': u16hex(f_c)},
        'registers': {'sp': u16hex(sp), 'bp': '0', 'si': '0', 'di': '0',
                      'flags': flags},
        'segments': {'ds': u16hex(ds_seg), 'es': u16hex(ds_seg),
                     'ss': u16hex(SS)},
        'frame': {'kind': 'near16',
                  'target': {'segment': u16hex(LOAD),
                             'offset': u16hex(TRAP_OFF)},
                  'sp_guard': True},
        'memory': mem,
        'observations': obs,
        'obs_pairs': pairs,   # emitted-obs indices to compare (frag-split aware)
        'flags_mask': '0x08d5',
    }
    return vec, pairs


def run_case(case, vectors, srcs):
    """Emit vectors for one function across its arg sets."""
    f_o = case['oracle_off']
    f_c = case['cand_off']
    if f_o > 0xFFFF or f_c > 0xFFFF:
        return [f"{case['fn']}: entry out of CS window (F_o={f_o:x} F_c={f_c:x})"]
    ds_seg = int(str(case.get('ds', DS)), 0)
    out = []
    for i, spec in enumerate(case['vectors']):
        vid = f"{case['fn']}#{i}"
        # a vector is bare [args..] or {'args':[..], 'patches':[...], ...}
        vcase = dict(case)
        args = spec
        if isinstance(spec, dict):
            args = spec['args']
            vcase['patches'] = case.get('patches', []) + spec.get('patches', [])
            if 'observe' in spec:
                vcase['observe'] = spec['observe']
            if 'check_regs' in spec:
                vcase['check_regs'] = spec['check_regs']
        vec, pairs = build_vector(vid, f_o, f_c, args, vcase, ds_seg)
        vectors.append(vec)
        srcs[vid] = (vcase, pairs)
    return out


def main():
    # args: spec json (list of cases) -> writes manifest, runs dosunit, reports.
    os.chdir(ROOT)
    spec_path = os.path.abspath(sys.argv[1])
    cases = json.load(open(spec_path))
    srcs = {}
    problems = []
    # replay-real16 takes a single --oracle-exe/--candidate-exe pair per run,
    # so group cases by exe pair and emit one manifest per group — otherwise
    # every vector would execute against the last case's candidate binary.
    groups = {}          # (oracle_exe, cand_exe) -> manifest
    order = []
    for case in cases:
        key = (case['oracle_exe'], case['cand_exe'])
        if key not in groups:
            groups[key] = {'oracle_load_segment': u16hex(LOAD),
                           'candidate_load_segment': u16hex(LOAD),
                           'vectors': [], 'code_o': set(), 'code_c': set()}
            order.append(key)
        grp = groups[key]
        f_o = routine_extent(case['fn'], case['oracle_map']) \
            if case.get('oracle_off') is None else case['oracle_off']
        if f_o is None:
            problems.append(f"{case['fn']}: not in {case['oracle_map']}")
            continue
        f_c = public_offset(case['fn'], case['cand_map']) \
            if case.get('cand_off') is None else case['cand_off']
        if f_c is None:
            problems.append(f"{case['fn']}: _{case['fn']} not in {case['cand_map']}")
            continue
        # For test-exe candidates the LINK map is ground truth: refresh the
        # dgrp base (used by size-pull patches) and the code window — spec
        # copies of these values go stale on every test-exe relink.
        cd = map_dgrp(os.path.join(ROOT, case['cand_map']))
        if cd is not None:
            case['cand_dgrp'] = cd
            case['code_c'] = [[0, cd]]
        case['oracle_off'], case['cand_off'] = f_o, f_c
        problems += run_case(case, grp['vectors'], srcs)
        for rng in case.get('code_o', []):
            grp['code_o'].add(
                (LOAD * 16 + int(str(rng[0]), 0), int(str(rng[1]), 0)))
        for rng in case.get('code_c', []):
            grp['code_c'].add(
                (LOAD * 16 + int(str(rng[0]), 0), int(str(rng[1]), 0)))
    for p in problems:
        print('SKIP', p)

    stem = spec_path.replace('.json', '')
    emit_only = '--emit-only' in sys.argv
    results = []
    for gi, key in enumerate(order):
        manifest = {'oracle_load_segment': u16hex(LOAD),
                    'candidate_load_segment': u16hex(LOAD),
                    'vectors': groups[key]['vectors']}
        if groups[key]['code_o']:
            manifest['oracle_code_ranges'] = [
                {'address': a, 'size': s}
                for a, s in sorted(groups[key]['code_o'])]
        if groups[key]['code_c']:
            manifest['candidate_code_ranges'] = [
                {'address': a, 'size': s}
                for a, s in sorted(groups[key]['code_c'])]
        vec_path = f'{stem}.g{gi}.vectors.json'
        out_path = f'{stem}.g{gi}.out.json'
        oracle_exe, cand_exe = key
        if not oracle_exe.startswith('/'):
            oracle_exe = os.path.join(ROOT, oracle_exe)
        if not cand_exe.startswith('/'):
            cand_exe = os.path.join(ROOT, cand_exe)
        json.dump(manifest, open(vec_path, 'w'), indent=1)
        if emit_only:
            continue            # vectors refreshed without touching out files
        if os.path.exists(out_path):
            os.remove(out_path)     # a failed replay must not surface stale results
        cmd = [VPY, '-m', 'tools.dosunit.dosunit', 'replay-real16',
               '--oracle-exe', oracle_exe, '--candidate-exe', cand_exe,
               '--vectors', vec_path, '--out', out_path,
               '--instruction-limit',
               os.environ.get('DOSUNIT_ILIMIT', '200000')]
        r = subprocess.run(cmd, cwd=VEXTEST, capture_output=True, text=True,
                           timeout=int(os.environ.get('DOSUNIT_TIMEOUT',
                                                      '1800')))
        if r.returncode not in (0, 1, 2) or not os.path.exists(out_path):
            print('dosunit failed:', r.stdout[-500:], r.stderr[-500:])
            return 1
        results += json.load(open(out_path))['results']
    if emit_only:
        return 0
    rep = {'results': results}
    print(f"{'vector':34s} {'oracle':>24s} {'cand':>24s} verdict")
    n_agree = n_diff = n_inc = 0
    for res in rep['results']:
        vid = res['id']
        case, pairs = srcs[vid]
        o, c = res['oracle'], res['candidate']
        want = case.get('check_regs', ['ax'])
        o_regs = ' '.join(f"{k}={o['registers'].get(k)}" for k in want)
        c_regs = ' '.join(f"{k}={c['registers'].get(k)}" for k in want)
        if o['status'] != 'returned' or c['status'] != 'returned':
            detail = (o['status'], o.get('detail', ''),
                      c['status'], c.get('detail', ''))
            if (o['status'] == c['status']
                    and o.get('detail', '') == c.get('detail', '')):
                # both sides fault identically (e.g. signedRatio16 int 0 on a
                # zero divisor) — faithful behavior, count as agreement
                verdict = 'AGREE-FAULT'
                n_agree += 1
            else:
                verdict = 'INCOMPLETE'
                n_inc += 1
        else:
            regs_ok = all(o['registers'].get(k) == c['registers'].get(k)
                          for k in want)
            o_obs = o.get('observations') or []
            c_obs = c.get('observations') or []
            obs_ok = all(
                i < len(o_obs) and j < len(c_obs) and
                (o_obs[i].get('bytes') if isinstance(o_obs[i], dict)
                 else o_obs[i]) ==
                (c_obs[j].get('bytes') if isinstance(c_obs[j], dict)
                 else c_obs[j])
                for i, j in pairs)
            verdict = 'AGREE' if (regs_ok and obs_ok) else 'DIFF'
            detail = ''
            n_agree += regs_ok and obs_ok
            n_diff += not (regs_ok and obs_ok)
        print(f"{vid:34s} {o_regs:>24s} {c_regs:>24s} {verdict} {detail}")
    print(f"-- {n_agree} agree, {n_diff} diverge, {n_inc} incomplete")
    return 0


if __name__ == '__main__':
    sys.exit(main())
