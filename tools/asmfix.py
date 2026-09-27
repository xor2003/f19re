#!/usr/bin/env python3
"""Repair a lst-driven driver skeleton until it assembles byte-exact.

drv2asm.py --lst emits ada mnemonics tagged '; @OFF:LEN' (image offset +
length). uasm may reject a form (error pass) or re-encode it differently
from the original assembler. Instead of comparing the linked image (where
one wrong-length instruction desyncs everything after it), we read uasm's
own -Fl listing, which carries the emitted bytes for every source line.

Passes:
  asm errors  -> db every tagged line uasm rejected
  length diff -> db lines whose emitted byte count != LEN (desync culprits)
  byte diff   -> once aligned, db lines whose emitted bytes != img[OFF:OFF+LEN]
  link+image  -> final safety net; db the line containing the first diff

Usage: asmfix.py DRIVER.EXE map/x.map out.asm --lst ada.lst
"""
import os
import re
import subprocess
import sys

sys.path.insert(0, os.path.dirname(__file__))
from compare_exe import load_image

TAG_RE = re.compile(r';\s*@([0-9A-Fa-f]+):([0-9]+)\s*$')
ERR_RE = re.compile(r'\((\d+)\)')
LST_ROW = re.compile(r'^([0-9A-F]+)\s+((?:[0-9A-F]{2} ?)+)\s{2,}(.*)$')
LST_CONT = re.compile(r'^([0-9A-F]+)\s+((?:[0-9A-F]{2} ?)+)\s*$')


def try_asm(asm, obj, lstpath):
    r = subprocess.run(
        ['uasm', '-q', '-0', '-Zm', f'-Fo{obj}', f'-Fl{lstpath}', asm],
        capture_output=True, text=True)
    errs = set()
    for ln in (r.stdout + r.stderr).splitlines():
        m = ERR_RE.search(ln)
        if 'Error' in ln and m:
            errs.add(int(m.group(1)))
    return errs


def parse_lst(lstpath):
    """-> {image_off: emitted_bytes} keyed by source-tag offset, plus
    emitted offset map rows: list of (emitted_off, bytes, tag_off|None)."""
    rows = []          # (emitted_off, bytes, tagoff)
    cur = None         # accumulating row
    for ln in open(lstpath, encoding='utf-8', errors='replace'):
        m = LST_ROW.match(ln)
        if m:
            if cur:
                rows.append(cur)
            off = int(m.group(1), 16)
            byts = bytes(int(p, 16) for p in
                         re.findall(r'[0-9A-F]{2}', m.group(2)))
            txt = m.group(3)
            tm = TAG_RE.search(txt)
            cur = [off, byts, int(tm.group(1), 16) if tm else None]
        elif (m := LST_CONT.match(ln)) and cur:
            cur[1] += bytes(int(p, 16) for p in
                            re.findall(r'[0-9A-F]{2}', m.group(2)))
    if cur:
        rows.append(cur)
    return rows


def db_line(lines, lno, img):
    m = TAG_RE.search(lines[lno])
    if not m:
        return False
    off, ln = int(m.group(1), 16), int(m.group(2))
    old = lines[lno].split(';')[0].rstrip()
    lines[lno] = ('        db ' + ','.join(f'0{b:02X}h' for b in
                                          img[off:off + ln])
                  + f'        ; {old.strip()}')
    return True


def main():
    exe, mapfile, asm = sys.argv[1], sys.argv[2], sys.argv[3]
    lst = sys.argv[sys.argv.index('--lst') + 1] if '--lst' in sys.argv else None
    build = 'build'
    if '--build' in sys.argv:
        build = sys.argv[sys.argv.index('--build') + 1]
    stem = os.path.splitext(os.path.basename(asm))[0]
    obj = os.path.join(build, stem + '.obj')
    ulst = os.path.join(build, stem + '.lst')
    outexe = os.path.join(build, stem + '.exe')
    img = load_image(exe)
    root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

    subprocess.check_call(
        [sys.executable, os.path.join(root, 'tools', 'drv2asm.py'),
         exe, mapfile, asm] + (['--lst', lst] if lst else []))
    for it in range(200):
        lines = open(asm).read().splitlines()
        errs = try_asm(asm, obj, ulst)
        if errs:
            n = sum(0 < l <= len(lines) and db_line(lines, l - 1, img)
                    for l in errs)
            if not n:
                print(f'cannot fix asm errors at {sorted(errs)[:10]}')
                return 1
            open(asm, 'w').write('\n'.join(lines) + '\n')
            print(f'asm pass {it}: db\'d {n} error lines')
            continue
        # stage 1: length mismatches (desync culprits)
        rows = parse_lst(ulst)
        bad_len = []
        tagged = {}      # tag off -> src line index
        for i, ln in enumerate(lines):
            m = TAG_RE.search(ln)
            if m:
                tagged[int(m.group(1), 16)] = i
        row_by_tag = {tag: (off, byts) for off, byts, tag in rows if tag}
        for off, (eoff, byts) in row_by_tag.items():
            mlen = int(TAG_RE.search(lines[tagged[off]]).group(2))
            if len(byts) != mlen:
                bad_len.append(off)
        if bad_len:
            for off in bad_len:
                db_line(lines, tagged[off], img)
            open(asm, 'w').write('\n'.join(lines) + '\n')
            print(f'len pass {it}: db\'d {len(bad_len)} length mismatches')
            continue
        # stage 2: content mismatches (stream now aligned)
        bad = [off for off, (eoff, byts) in row_by_tag.items()
               if byts != img[off:off + len(byts)]]
        if bad:
            for off in bad:
                db_line(lines, tagged[off], img)
            open(asm, 'w').write('\n'.join(lines) + '\n')
            print(f'byte pass {it}: db\'d {len(bad)} content mismatches')
            continue
        # stage 3: link + whole-image compare
        r = subprocess.run(
            ['mzretools/tools/dosbuild.sh', 'link', 'msc510',
             '-i', obj, '-o', outexe, '-f', '/M /I'],
            cwd=root, env={**os.environ, 'DOSBOX': 'dosbox'},
            capture_output=True, text=True)
        if not os.path.exists(outexe):
            print(r.stdout[-1500:])
            print('link failed')
            return 1
        a, b = load_image(exe), load_image(outexe)
        diffs = [i for i in range(min(len(a), len(b))) if a[i] != b[i]]
        if not diffs and len(a) == len(b):
            print(f'byte-exact after {it} passes: {outexe}')
            return 0
        if not diffs:
            print(f'content matches but size differs: {len(a)} vs {len(b)}')
            return 1
        d = diffs[0]
        for i, ln in enumerate(lines):
            m = TAG_RE.search(ln)
            if m:
                off, ln2 = int(m.group(1), 16), int(m.group(2))
                if off <= d < off + ln2 and db_line(lines, i, img):
                    open(asm, 'w').write('\n'.join(lines) + '\n')
                    print(f'img pass {it}: db\'d line covering {d:#x}')
                    break
        else:
            print(f'unfixable diff at {d:#x} ({len(diffs)} diffs)')
            return 1
    print('gave up')
    return 1


if __name__ == '__main__':
    sys.exit(main())
