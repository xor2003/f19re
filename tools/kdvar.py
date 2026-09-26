#!/usr/bin/env python3
"""keyDispatch switch-tail variant tester.

Patches src/egkeys.c break/goto forms for the shared-tail case groups,
compiles ONLY egkeys.c with MSC 5.1, extracts _keyDispatch code from the
.OBJ, and reports the tail-merge topology:

  - which case hosts each shared `push ax; call` block
  - where the shared `add sp,2` cleanup block lands (anchor)
  - each case tail's continuation (inline add-sp vs jmp <anchor>)

Usage:
    python3 tools/kdvar.py <variant-spec>
    spec:  cm=BBB,GGB..  s5=B|G  dp=BBBBBB  g4400=B|G
    e.g.   python3 tools/kdvar.py cm=BGG dp=BBBBBB s5=B g4400=G

Order for dp slots: 342E(scanApply) 3F00 4000 4100 4200 4400
The source file is restored after each run unless --keep is passed.
"""
import os, re, struct, subprocess, sys, tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = os.path.join(ROOT, 'src', 'egkeys.c')
MSC = os.path.join(ROOT, 'dos', 'msc510')
KVIKDOS = os.path.join(ROOT, 'mzretools', 'tools', 'emulators', 'kvikdos', 'kvikdos')
NDISASM = 'ndisasm'

CM_CASES = ['0x231', '0x332', '0x635']
CM_CALL = 'countermeasures'
DP_CASES = [('0x342E', '0x13'), ('0x3F00', '0x15'), ('0x4000', '0x16'),
            ('0x4100', '0x14'), ('0x4200', '0x14'), ('0x4400', '0x18')]
DP_CALL = 'drawPanelModeText'

def tail_for(form):
    return 'break;' if form == 'B' else 'goto cleanup;'

def patch_source(spec):
    src = open(SRC).read()
    orig = src
    # --- countermeasures group ---
    cm = spec.get('cm')
    if cm:
        for i, case in enumerate(CM_CASES):
            pat = re.compile(
                r'(case %s:[^\n]*\n\s*%s\(%d\);\n)\s*(break;|goto cleanup;)'
                % (re.escape(case), CM_CALL, i + 1))
            src, n = pat.subn(r'\1        ' + tail_for(cm[i]), src)
            assert n == 1, 'cm case %s not found' % case
    # --- drawPanelModeText group ---
    dp = spec.get('dp')
    if dp:
        for i, (case, arg) in enumerate(DP_CASES):
            pat = re.compile(
                r'(%s\(%s\);\n)\s*(break;|goto cleanup;)'
                % (DP_CALL, re.escape(arg)))
            # for duplicated 0x14 args disambiguate by occurrence index
            matches = list(pat.finditer(src))
            if arg == '0x14':
                idx = 0 if case == '0x4100' else 1
                m = matches[idx]
                src = src[:m.start()] + m.group(1) + '        ' + tail_for(dp[i]) + src[m.end():]
            else:
                assert len(matches) == 1, 'dp %s: %d matches' % (case, len(matches))
                m = matches[0]
                src = src[:m.start()] + m.group(1) + '        ' + tail_for(dp[i]) + src[m.end():]
    # --- 0x5200 hudMessage tail ---
    s5 = spec.get('s5')
    if s5:
        # the 0x5200 case is the one whose hudMessage is last stmt before break
        pat = re.compile(
            r"(case 0x5200:[^\n]*\n(?:[^\n]*\n)*?\s*hudMessage\(strBuf\);\n)\s*(break;|goto cleanup;)")
        src, n = pat.subn(r'\1        ' + tail_for(s5), src)
        assert n == 1, '0x5200 block not found'
    return src, orig

def compile_obj():
    for f in ('EGKEYS.OBJ',):
        p = os.path.join(MSC, 'bin', f)
        if os.path.exists(p):
            os.remove(p)
    r = subprocess.run([KVIKDOS,
        '--mount=C:%s/' % MSC,
        '--mount=D:%s/' % os.path.join(ROOT, 'src'),
        '--env=LIB=C:\\lib', '--env=INCLUDE=C:\\INCLUDE',
        '--env=PATH=C:\\bin', '--env=TMP=D:\\',
        'C:\\bin\\CL.EXE', '/AS', '/Gs', '/Ot', '/c', 'D:\\EGKEYS.C'],
        capture_output=True)
    obj = os.path.join(MSC, 'bin', 'EGKEYS.OBJ')
    if not os.path.exists(obj):
        print('compile failed:\n' + r.stdout.decode('ascii', 'replace')
              + r.stderr.decode('ascii', 'replace'))
        sys.exit(1)
    return obj

def extract_code(objpath):
    """Return (pubdef_offsets, code_bytes) — concat LEDATA of code seg + PUBDEFs."""
    data = open(objpath, 'rb').read()
    i = 0
    pubs = {}       # name -> frame offset
    seg_names = []  # LNAMES order
    code_idx = None
    segdata = []    # (segidx, offset, bytes)
    cur_seg = None
    while i < len(data) - 3:
        rec = data[i]
        ln = data[i+1] | (data[i+2] << 8)
        body = data[i+3:i+3+ln-1]
        if rec == 0x96:   # LNAMES
            j = 0
            while j < len(body):
                sl = body[j]; j += 1
                seg_names.append(body[j:j+sl]); j += sl
        elif rec in (0x90, 0x91):  # PUBDEF
            # body: grpidx, segidx, then name/offset/type
            seg = body[1]
            j = 2
            if seg == 0:
                j += 2
            while j < len(body):
                nl = body[j]; j += 1
                nm = body[j:j+nl].decode('ascii', 'replace'); j += nl
                off = body[j] | (body[j+1] << 8); j += 2
                j += 1  # type idx
                pubs[nm] = (seg, off)
        elif rec in (0xA0, 0xA1):  # LEDATA
            segidx = body[0]
            off = body[1] | (body[2] << 8)
            segdata.append((segidx, off, body[3:]))
            seg_names_idx = segidx
        i += 3 + ln
    # find the code segment index used by _keyDispatch
    seg, off = pubs.get('_keyDispatch', (None, None))
    if seg is None:
        print('no _keyDispatch PUBDEF'); sys.exit(1)
    # gather code bytes of that segment
    chunks = [d for (s, o, d) in segdata if s == seg]
    code = b''.join(
        (b'\x00' * (o - sum(len(c) for c in chunks[:k])) if False else d)
        for k, (s, o, d) in enumerate([(s, o, d) for (s, o, d) in segdata if s == seg]))
    # simpler: concatenate in record order (single LEDATA typical)
    code = b''.join(d for (s, o, d) in segdata if s == seg)
    return pubs, code, off

def disasm(code, base):
    with open('/tmp/kdv.bin', 'wb') as f:
        f.write(code)
    out = subprocess.run([NDISASM, '-b16', '-o0x%x' % base, '/tmp/kdv.bin'],
                         capture_output=True, text=True).stdout
    ins = []
    for line in out.splitlines():
        m = re.match(r'([0-9A-F]{8})\s+([0-9A-F]+)\s+(.*)', line)
        if m:
            ins.append((int(m.group(1), 16), m.group(3)))
    return ins

def jmp_tgts(ins, lo, hi):
    tg = {}
    for a, t in ins:
        if lo <= a < hi:
            m = re.match(r'jmp (?:short )?(0x[0-9a-f]+)', t)
            if m:
                v = int(m.group(1), 16)
                tg[v] = tg.get(v, 0) + 1
    return tg

def describe_group(ins, args, lo, hi, name):
    """For a group of `mov ax,arg; [jmp|push]` stubs + shared push;call,
    report each arg's role (HOST/STUB) and tail continuation."""
    text = {a: t for a, t in ins}
    addrs = [a for a, t in ins]
    def nxt(a, k=1):
        try:
            i = addrs.index(a)
            return addrs[i+k] if i + k < len(addrs) else None
        except ValueError:
            return None
    # find `push ax;call` sites in range
    pushsites = [a for a, t in ins
                 if lo <= a < hi and t == 'push ax'
                 and nxt(a) is not None and text.get(nxt(a), '').startswith('call')]
    for p in pushsites:
        # preceding non-nop instruction
        b = addrs.index(p) - 1
        while b >= 0 and text[addrs[b]] == 'nop':
            b -= 1
        arg = text.get(addrs[b], '?') if b >= 0 else '?'
        c = nxt(p)          # call
        c1 = nxt(c)         # after call
        cont = text.get(c1, '?') if c1 else '?'
        # is this push ax a merge target (jumps into it)?
        refs = [a for a, t in ins
                if lo <= a < hi
                and re.match(r'jmp (?:short )?0x%x$' % p, t)]
        tag = '  <- merge host(%d stubs)' % len(refs) if refs else ''
        print('  %s %05x push;call  prev=[%s]  cont=[%s]%s'
              % (name, p, arg, cont, tag))
    # mov ax,N stubs into any push ax site
    for a, t in ins:
        if lo <= a < hi and re.match(r'mov ax,0x[0-9a-f]+$', t):
            n = nxt(a)
            if n is not None and re.match(r'jmp (?:short )?0x', text.get(n, '')):
                print('  %s %05x %s ; %s' % (name, a, t, text[n]))

def analyze(ins, kd_off, kd_size):
    lo, hi = kd_off, kd_off + kd_size
    tg = jmp_tgts(ins, lo, hi)
    text = {a: t for a, t in ins}
    # add-sp,2 sites that are jump targets = dedupe anchors
    anchors = sorted(a for a, t in ins
                     if lo <= a < hi and t == 'add sp,byte +0x2' and a in tg)
    print('anchors(add sp,2 with jmp refs): %s'
          % ', '.join('%x(r%d)' % (a, tg[a]) for a in anchors))
    # add-sp,2 sites total
    allsp = [a for a, t in ins if lo <= a < hi and t == 'add sp,byte +0x2']
    print('all add sp,2 sites: ' + ' '.join('%x' % a for a in allsp))
    # countermeasures group: args 1,2,3 — cluster near case bodies
    print('--- countermeasures group (args 1/2/3) ---')
    describe_group(ins, [1, 2, 3], lo, hi, 'cm')
    print('--- drawPanelModeText group ---')
    describe_group(ins, [0x13, 0x15, 0x16, 0x14, 0x18], lo, hi, 'dp')
    # updatePanelMode tail: find ternary then push ax;call
    for a, t in ins:
        if lo <= a < hi and t == 'push ax':
            # upMode: preceded by sub ax,ax or mov ax,1 ternary block
            pass


def main():
    keep = '--keep' in sys.argv
    args = [a for a in sys.argv[1:] if a != '--keep' and not a.startswith('-')]
    spec = {}
    for a in args:
        k, v = a.split('=')
        spec[k] = v
    src, orig = patch_source(spec)
    open(SRC, 'w').write(src)
    try:
        obj = compile_obj()
        pubs, code, koff = extract_code(obj)
        # routine extent: from PUBDEF _keyDispatch to next pub
        same = sorted(o for (s, o) in pubs.values() if s == pubs['_keyDispatch'][0] and o >= koff)
        end = same[1] if len(same) > 1 else len(code)
        ins = disasm(code, 0)
        analyze(ins, koff, end - koff)
    finally:
        if not keep:
            open(SRC, 'w').write(orig)

if __name__ == '__main__':
    main()
