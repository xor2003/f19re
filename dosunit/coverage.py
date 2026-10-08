#!/usr/bin/env python3
"""F-19 differential coverage inventory.

For every ported routine (real definitions in src_en/src_start/src_end/src_su
that exist in the oracle map) reports one of:

  dedicated   - named in a hand-authored dosunit spec (dosunit/*.json)
  probe-agree - probe replay AGREE on >=1 vector
  probe-mix   - some vectors agree, some incomplete (covered, partial)
  artifact    - all DIFFs traceable to probe mechanics (listed in ARTIFACTS)
  diff        - unresolved divergences
  incomplete  - all vectors INCOMPLETE (reasons listed)
  skipped     - deliberately excluded from probes (SKIP set)
  uncovered   - no dedicated spec and no probe case

Usage: python3 dosunit/coverage.py [--verbose]
"""
import glob, json, os, re, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, 'tools'))
from duspec import EXE, cand_dgrp  # noqa: E402

SKIP_MODS = ('stubs', '_stub')

# Probe-mechanics artifacts: diffs explained by pointer/image overlap or
# unported callees — NOT port bugs.  name -> reason
ARTIFACTS = {
    'commFetch': 'movedata dst seg pulled from image overlaps obs cell',
    'bufReadBytes': 'cand obs cell inside dst=arg0 write stream',
    'bufReadFile': 'cand obs cell inside dst=arg0 write stream',
    'loadWorldStrings': 'stores runtime pointer values (binary-relative)',
    'sub_10E84': 'calls unported stub sub_11C4E (returns 0)',
    'memAppend': 'movedata dst seg pulled from image overlaps obs cell',
    # slot-stub era: routine runs to completion; residual diffs are cells
    # holding own-image pointers or init-table-derived values — binary
    # provenance, not port behavior.  A dedicated spec could add scalar
    # signal later, but nothing here indicates a port bug.
    'drawRiskPanel': 'esTabBase stores own esTable address (pointer)',
    'processDebriefInput': 'colorTablePtr stores own colorStyleTable address',
    'sub_15D1B': 'popupXY from flightRecords init-table + mapToScreen*',
    'drawViewportLine': 'clip coords via g_vpParms own-image struct',
    'updateThreatSites': 'store pattern driven by planeTable init data',
    # paired-store cross-wire: oracle flag cells are ADJACENT (0x5524/0x5526)
    # while cand mirrors are far apart (0x64c2/0x1a90); positional pairing
    # swapped them.  Mirrored stores DO match — both sides test+store ffff
    # to their first flag cell in the same order; oracle's extra 0x551e
    # clear and cand's 0x4e7e/0x4e8a clears are init-table-driven.
    'renderFrame': 'paired obs cross-wired (adjacent o cells vs split c cells)',
    # oracle init flag array at shared-DS 0x5524-0x553e lands on the cand's
    # g_projectiles BSS cells; cand flag check reads oracle's 0x01 and runs
    # a loop the oracle (BSS-zero) skips.  Same-offset/different-role pull
    # collision, not a port diff.
    'updateThreatTargeting': 'oracle init flags collide with cand g_projectiles',
    # both sides run to the same overlay/slot dispatch; cand follows its
    # slot-stub thunk while oracle enters the slot-stub path — streams
    # realign after; observed cells hold binary-relative overlay tables.
    'renderHudFrame': 'overlay slot thunk vs slotstub, binary-relative cells',
    # START satellite: draw work lives in unported reg-ABI callee sub_141A3
    # (skeleton in STGEN); clip shim returns 0 — unported-callee artifact.
    'drawLine': 'satellite draw callee sub_141A3 is skeleton',
    # MATCH-verified (stobj.c /Ot).  resFileOpen + record loop store
    # through own-image far pointers — `ffff 3000 0000` triples carry the
    # scratch-DS seg as data and land at layout-relative offsets; oracle
    # union-obs cells (DS:0x1716 LUT clears, 0x152a int21-success flag)
    # and cand mirrors (DS:0x784c...) therefore read back asymmetric.
    # Scratch bx/cx return values are the same provenance.  The ds:=ss
    # write inside resFileOpen's int21 helper (mov bx,ss; mov ds,bx at
    # 0x483a) is already nopped by the dsss fixture — that much is fixed.
    'sub_15460': 'record stores via own-image far ptrs; ds:=ss normalized',
}
SKIP = {  # entry points / unsynthesizable — kept in sync with gen_probe.SKIP
    'installCBreakHandler', 'setInt9Handler',
    'openFile', 'closeFile', 'picBlit', 'load15Flt3d3',
    'fillSpanRect', 'projectSceneObject',
}
# Former skips that now probe cleanly under call_stub/int_stub/dsss:
# gfxInit/openBlitClosePic 5/5 AGREE, main/waitForKeyPress 5/5 AGREE-FAULT
# (symmetric key-poll/main-loop budget burn), runGameSession incomplete
# (oracle reaches device io in the session loop — sandbox limit).
SUITES = {
    'src_en':   ('map/egame_en.map', 'probe_egame'),
    'src_start': ('map/start_en.map', 'probe_start'),
    'src_end':  ('map/end_en.map',   'probe_end'),
    'src_su':   ('map/su_en.map',    'probe_su'),
}


def omap_names(path):
    out = set()
    for line in open(path, errors='replace'):
        m = re.match(r'(\w+): \w+ (?:NEAR|FAR) [0-9a-f]+-', line.strip())
        if m:
            out.add(m.group(1))
    return out


def ported_defs(srcdir, omap_set):
    """Routine names with real bodies (prototypes end in ';' not '{')."""
    defs = set()
    for f in glob.glob(os.path.join(ROOT, srcdir, '*.c')):
        if any(s in os.path.basename(f).lower() for s in SKIP_MODS):
            continue
        txt = open(f, errors='replace').read()
        for m in re.finditer(r'\b([A-Za-z_]\w*)\s*\([^;{}()]*\)\s*\{', txt):
            if m.group(1) in omap_set:
                defs.add(m.group(1))
    return defs


def dedicated():
    spec = set()
    for f in glob.glob(os.path.join(ROOT, 'dosunit', '*.json')):
        b = os.path.basename(f)
        if b.startswith('probe_') or '.out.' in b or '.vectors.' in b:
            continue
        try:
            data = json.load(open(f))
        except Exception:
            continue
        cs = data if isinstance(data, list) else data.get('inputs', [])
        for c in cs:
            if isinstance(c, dict) and 'fn' in c:
                spec.add(c['fn'])
    return spec


def pairs_of(case):
    n = 0
    pairs = []
    for o in case.get('observe', []):
        if 'off' in o:
            n += 1
            pairs.append((n - 1, n - 1))
        else:
            n += 2
            pairs.append((n - 2, n - 1))
    return pairs


def probe_verdicts():
    """fn -> Counter(agree/diff/inc) replicated from dosunit16 verdict logic."""
    from collections import Counter, defaultdict
    agg = defaultdict(Counter)
    du = os.path.join(ROOT, 'dosunit')
    # Cases index globally across specs: shard boundaries shift between
    # regens, orphaning out.json results whose group moved files — the
    # fn name in each result id is authoritative.
    cases = {}
    outs = []
    for sf in sorted(glob.glob(os.path.join(du, 'probe_*.json'))):
        if '.out.' in sf or '.vectors.' in sf:
            continue
        try:
            data = json.load(open(sf))
        except Exception:
            continue
        if not isinstance(data, list):
            continue
        for c in data:
            cases.setdefault(c['fn'], c)
        outs.extend(sorted(glob.glob(sf[:-5] + '.g*.out.json')))
    for of in outs:
        try:
            res = json.load(open(of))['results']
        except Exception:
            continue
        vp = {}   # emitted obs_pairs survive frag-splitting; spec-derived
        try:      # pairs_of is only a fallback for pre-paired vectors
            for v in json.load(
                    open(of.replace('.out.json', '.vectors.json'))
                    )['vectors']:
                vp[v['id']] = v.get('obs_pairs')
        except Exception:
            pass
        for r in res:
            fn = r['id'].rsplit('#', 1)[0]
            case = cases.get(fn)
            if case is None:
                continue
            o, c = r['oracle'], r['candidate']
            if o['status'] != 'returned' or c['status'] != 'returned':
                if (o['status'] == c['status'] and
                        o.get('detail', '') == c.get('detail', '')):
                    agg[fn]['agree'] += 1
                else:
                    key = (o['status'], o.get('detail', '')[:28],
                           c['status'], c.get('detail', '')[:28])
                    agg[fn]['inc:' + str(key)] += 1
                continue
            want = case.get('check_regs', ['ax'])
            regs_ok = all(o['registers'].get(k) == c['registers'].get(k)
                          for k in want)
            oo = o.get('observations') or []
            co = c.get('observations') or []
            obs_ok = all(
                i < len(oo) and j < len(co) and
                oo[i].get('bytes') == co[j].get('bytes')
                for i, j in (vp.get(r['id']) or pairs_of(case)))
            agg[fn]['agree' if regs_ok and obs_ok else 'diff'] += 1
    return agg


def main():
    spec = dedicated()
    agg = probe_verdicts()
    inv = {}
    for srcdir, (omap, prefix) in SUITES.items():
        for nm in sorted(ported_defs(srcdir, omap_names(omap))):
            inv[nm] = srcdir
    rows = []
    stat = {}
    for nm, srcdir in sorted(inv.items()):
        c = agg.get(nm)
        if nm in spec:
            st = 'dedicated'
        elif nm in ARTIFACTS:
            st = 'artifact'
        elif nm in SKIP:
            st = 'skipped'
        elif c is None:
            st = 'uncovered'
        elif c.get('diff'):
            st = 'probe-mix' if c.get('agree') else 'diff'
        elif c.get('agree'):
            st = 'probe-mix' if any(k.startswith('inc:') for k in c) \
                else 'probe-agree'
        else:
            st = 'incomplete'
        stat.setdefault(st, []).append(nm)
        rows.append((nm, srcdir, st, dict(c) if c else {}))
    for st in ('dedicated', 'probe-agree', 'probe-mix', 'artifact',
               'incomplete', 'skipped', 'diff', 'uncovered'):
        v = stat.get(st, [])
        print('%-12s %4d  %s' % (st, len(v), ' '.join(v[:18]) +
              (' ...' if len(v) > 18 else '')))
    print('total ported routines:', len(inv))
    # bucket the all-incomplete routines by dominant blocker class
    REASON_CLASS = [
        ('fetch_outside_declared_code', 'overlay/xseg call'),
        ('interrupt_or_iret', 'int instruction'),
        ('device_io_or_halt', 'hardware io'),
        ('budget_exhausted', 'looping'),
        ('unmapped_access', 'wild pointer'),
        ('unmodeled_register_state', 'reg ABI'),
        ('interrupt:', 'faulting'),
    ]
    cls = {}
    for nm in stat.get('incomplete', []):
        ks = ' '.join(k for k in agg[nm] if k.startswith('inc:'))
        for tag, label in REASON_CLASS:
            if tag in ks:
                cls.setdefault(label, []).append(nm)
                break
        else:
            cls.setdefault('other', []).append(nm)
    for label, v in sorted(cls.items()):
        print('  incomplete:%-17s %3d  %s' %
              (label, len(v), ' '.join(sorted(v)[:14]) +
              (' ...' if len(v) > 14 else '')))
    if '--verbose' in sys.argv:
        for nm, sd, st, c in rows:
            print('%-28s %-9s %-11s %s' % (nm, sd, st, c if c else ''))


if __name__ == '__main__':
    main()
