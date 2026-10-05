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
}
SKIP = {  # entry points / unsynthesizable — kept in sync with gen_probe.SKIP
    'main', 'gfxInit', 'installCBreakHandler', 'setInt9Handler',
    'openFile', 'closeFile', 'picBlit', 'openBlitClosePic', 'load15Flt3d3',
    'waitForKeyPress', 'runGameSession', 'fillSpanRect', 'projectSceneObject',
}
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
    for sf in sorted(glob.glob(os.path.join(du, 'probe_*.json'))):
        if '.out.' in sf or '.vectors.' in sf:
            continue
        try:
            data = json.load(open(sf))
        except Exception:
            continue
        if not isinstance(data, list):
            continue
        cases = {c['fn']: c for c in data}
        for of in sorted(glob.glob(sf[:-5] + '.g*.out.json')):
            try:
                res = json.load(open(of))['results']
            except Exception:
                continue
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
                    for i, j in pairs_of(case))
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
