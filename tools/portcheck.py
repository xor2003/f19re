#!/usr/bin/env python3
"""Compile-check a ported F19 routine against the original binary.

Usage:
    python3 tools/portcheck.py <src.c> <routine> [--exe egame|start|end|su] [clflags...]

Example:
    python3 tools/portcheck.py src/egparse.c replaceExtension

Steps:
  1. compile <src.c> with MSC 5.1 under kvikdos (default /AS /Os /Gs)
  2. link a test exe with mlibce.lib
  3. locate _<routine> in the link map
  4. mzdiff the original routine's extent (from map/<exe>.map) against it
"""
import os
import re
import struct
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
# vendored mzretools lives inside the repo; override via env for a dev tree
MZDIFF = os.environ.get('MZDIFF', os.path.join(ROOT, 'mzretools', 'build', 'mzdiff'))
KVIKDOS = os.environ.get('KVIKDOS',
        os.path.join(ROOT, 'mzretools', 'tools', 'emulators', 'kvikdos', 'kvikdos'))
MSC = os.path.join(ROOT, 'dos', 'msc510')
BUILD = os.path.join(ROOT, 'build')

# Per-module compiler flags, determined empirically: each original module was
# built with a fixed set (as in f15se2's Makefile).  /Os vs /Ot matters on
# routines with early returns (shared vs inlined epilogue); /Oa (assume no
# aliasing) is required where a global stays in a register across a pointer
# store (drawTargetView's `push bx`).  Modules whose routines all match under
# either flag default to /Ot.
MODULE_FLAGS = {
    'eg3dload.c': ['/AS', '/Gs', '/Os'],
    'eg3dmap.c':  ['/AS', '/Gs', '/Ot'],
    'eg3dview.c': ['/AS', '/Gs', '/Ot'],
    'egrender.c': ['/AS', '/Gs', '/Ot', '/Oa'],
    'egcombat.c': ['/AS', '/Gs', '/Os', '/Oa'],
    'egflight.c': ['/AS', '/Gs', '/Os', '/Oa'],
    'egframe.c':  ['/AS', '/Gs', '/Os', '/Oa'],
    'egkeys.c':   ['/AS', '/Gs', '/Ot'],
    'egmath.c':   ['/AS', '/Gs', '/Os'],
    'egtacmap.c': ['/AS', '/Gs', '/Os', '/Oa'],
    'egtarget.c': ['/AS', '/Gs', '/Os', '/Oa'],
    'egui.c':     ['/AS', '/Gs', '/Os', '/Oa'],
    'stparse.c':  ['/AS', '/Gs', '/Ot'],
}
DEFAULT_FLAGS = ['/AS', '/Gs', '/Ot']


def module_flags(mod):
    return MODULE_FLAGS.get(os.path.basename(mod).lower(), DEFAULT_FLAGS)


def load_image(path):
    data = open(path, 'rb').read()
    return data[struct.unpack('<H', data[8:10])[0] * 16:]


def routine_extents(name, map_path):
    """name -> (seg_base_linear, start_off, end_off) using segment table."""
    segs = {}
    with open(map_path, errors='replace') as f:
        for line in f:
            line = line.strip()
            m = re.match(r'([\w.$]+) (CODE|DATA|STACK) ([0-9a-fA-F]+)', line)
            if m:
                segs[m.group(1)] = int(m.group(3), 16) * 16
                continue
            m = re.match(r'([\w.$]+): ([\w.$]+) (NEAR|FAR) ([0-9a-fA-F]+)-([0-9a-fA-F]+)', line)
            if m and m.group(1) == name:
                seg = segs[m.group(2)]
                return seg + int(m.group(4), 16), seg + int(m.group(5), 16)
    return None


def public_offset(name, linkmap_path, image_base):
    """_name -> linear image offset of the public in the linked exe."""
    with open(linkmap_path, errors='replace') as f:
        for line in f:
            m = re.search(r'([0-9a-fA-F]+):([0-9a-fA-F]+)\s+_' + re.escape(name) + r'\s*$', line)
            if m:
                return int(m.group(1), 16) * 16 + int(m.group(2), 16) - image_base
    return None


def kvikdos(args):
    cmd = [KVIKDOS,
           '--mount=C:%s/' % MSC,
           '--mount=D:%s/' % os.path.join(ROOT, 'src'),
           '--mount=E:%s/' % BUILD,
           '--env=LIB=C:\\lib', '--env=INCLUDE=C:\\INCLUDE',
           '--env=PATH=C:\\bin;C:\\BBIN', '--env=TMP=D:\\',
           '--drive=E'] + args
    return subprocess.run(cmd)


def main():
    args = sys.argv[1:]
    exe = 'egame'
    if '--exe' in args:
        i = args.index('--exe')
        exe = args[i + 1]
        del args[i:i + 2]
    flags = [a for a in args if a.startswith('/')]
    args = [a for a in args if not a.startswith('/')]
    if len(args) < 2:
        print(__doc__)
        sys.exit(1)
    src, names = args[0], args[1:]
    # explicit flags override; otherwise the module's recorded set is used
    if not flags:
        flags = module_flags(src)

    base = os.path.splitext(os.path.basename(src))[0].upper()[:8]
    srcdos = 'D:\\' + os.path.basename(src).upper()
    # compile every src/*.c module (except stubs.c) so cross-module
    # references resolve to real implementations; stubs fills the rest.
    import glob
    modules = [os.path.basename(p) for p in glob.glob(os.path.join(ROOT, 'src', '*.c'))
               if os.path.basename(p).lower() not in ('stubs.c', '_stub.c')]
    linkobjs = []
    for mod in sorted(modules):
        modbase = os.path.splitext(mod)[0].upper()[:8]
        modflags = flags if mod == os.path.basename(src) else module_flags(mod)
        # cl drops the obj into its own directory (C:\bin) under kvikdos
        r = kvikdos(['C:\\bin\\CL.EXE'] + modflags + ['/c', 'D:\\' + mod.upper()])
        if r.returncode != 0:
            sys.exit(r.returncode)
        if not os.path.exists(os.path.join(MSC, 'bin', modbase + '.OBJ')):
            print('compile produced no object: C:\\bin\\%s.OBJ' % modbase)
            sys.exit(1)
        linkobjs.append('C:\\bin\\%s.OBJ' % modbase)
    # ensure _main + extern-stub objects exist so CRT0 and callees link
    stub = os.path.join(MSC, 'bin', '_STUB.OBJ')
    if not os.path.exists(stub):
        kvikdos(['C:\\bin\\CL.EXE', '/AS', '/c', 'D:\\_STUB.C'])
    stubs_src = os.path.join(ROOT, 'src', 'stubs.c')
    if os.path.exists(stubs_src):
        # kvikdos writes DOS mtimes — always rebuild (cheap)
        kvikdos(['C:\\bin\\CL.EXE', '/AS', '/c', 'D:\\STUBS.C'])

    model = 'S'
    for f in flags:
        if re.match(r'/A[SHMLC]', f, re.I):
            model = f[2].upper()
    # LINK via response file — the arg list exceeds DOS's ~126-char limit
    rsp = os.path.join(BUILD, 'portlink.rsp')
    # LINK 3.65 response files: one line per prompt, lines continue via
    # trailing '+' and no line may exceed ~126 chars.
    objlines = '+\n'.join(linkobjs) + '+\nC:\\bin\\_STUB.OBJ+C:\\bin\\STUBS.OBJ,'
    with open(rsp, 'w') as f:
        f.write('/M %s\nE:\\%s.EXE,\nE:\\%s.MAP,\nC:\\lib\\%sLIBCE;\n'
                % (objlines, base, base, model))
    r = kvikdos(['C:\\bin\\LINK.EXE', '@E:\\portlink.rsp'])
    if r.returncode != 0:
        sys.exit(r.returncode)
    # tidy: move the stray objs out of the compiler dir
    for mod in modules:
        modbase = os.path.splitext(mod)[0].upper()[:8]
        os.rename(os.path.join(MSC, 'bin', modbase + '.OBJ'),
                  os.path.join(BUILD, modbase.lower() + '.obj'))

    def find_out(ext):
        for cand in os.listdir(BUILD):
            if cand.lower() == (base + ext).lower():
                return os.path.join(BUILD, cand)
        return None
    testexe = find_out('.exe')
    linkmap = find_out('.map')
    if not testexe or not linkmap:
        print('link produced no exe/map in', BUILD)
        sys.exit(1)
    map_path = os.path.join(ROOT, 'map', exe + '.map')
    ref_exe = os.path.join(ROOT, exe.upper() + '.EXE')

    rc = 0
    for name in names:
        ext = routine_extents(name, map_path)
        if not ext:
            print(f'{name}: not found in {map_path}')
            rc = 1
            continue
        tgt = public_offset(name, linkmap, 0)
        if tgt is None:
            print(f'{name}: _{name} not found in {linkmap}')
            rc = 1
            continue
        size = ext[1] - ext[0]
        spec_ref = f'{ref_exe}:0x{ext[0]:x}-0x{ext[1]:x}'
        spec_tgt = f'{testexe}:0x{tgt:x}-0x{tgt + size:x}'
        cmd = [MZDIFF, spec_ref, spec_tgt, '--nocall', '--loose',
               '--map', map_path]
        r = subprocess.run(cmd, capture_output=True, text=True)
        ok = r.returncode == 0
        if not ok:
            # a mismatch preceded by "data segment offset mapping conflict" only
            # means the global lives at a different offset in the test exe —
            # the instruction stream itself matched. All mismatching pairs must
            # be data-offset conflicts for this to count as a match.
            lines = r.stdout.splitlines()
            errors = 0
            allowed = 0
            prev_conflict = False
            for ln in lines:
                if 'ERROR:' in ln:
                    errors += 1
                    # tolerated: data-offset remap conflicts, and stray 'nop'
                    # pads (MSC aligns jump targets to even; pad placement
                    # depends on the function's offset in the final exe)
                    if prev_conflict or 'nop' in ln:
                        allowed += 1
                prev_conflict = ('data segment offset mapping conflict' in ln)
            if errors > 0 and errors == allowed:
                ok = True
        print(f'{name}: {"MATCH" if ok else "MISMATCH"}'
              f'  ({os.path.basename(src)} @0x{tgt:x} vs ref 0x{ext[0]:x}-0x{ext[1]:x})')
        if not ok:
            print('  ' + ' '.join(cmd))
            print('\n'.join('  ' + l for l in r.stdout.splitlines()[:25]))
            rc = 1
    sys.exit(rc)


if __name__ == '__main__':
    main()
