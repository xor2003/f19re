#!/usr/bin/env python3
"""Generate a byte-exact MASM skeleton .asm for a normal MZ executable
(SU.EXE / START.EXE / END.EXE / EGAME.EXE) from its binary + routine map.

Sibling of drv2asm.py (which handles the OvlHeader driver overlay format).
Differences:

- segments come from the map (CODE/DATA/STACK paragraphs), emitted in
  paragraph order with LINK-compatible attributes;
- routines live in ANY segment (mostly seg000, some in dseg);
- `end start` terminates the module; the entry label is emitted at the
  MZ header's initial CS:IP image offset;
- stack/bss space beyond the load image is emitted as `dup(?)` reserves.

Usage: exe2asm.py PROG.EXE map/x.map out.asm [--lst ada.lst]

With --lst, code is emitted as real mnemonics from an Ada Script listing
instead of db+comment lines (same rules as drv2asm --lst).
"""
import os
import re
import struct
import sys

sys.path.insert(0, os.path.dirname(__file__))
from compare_exe import load_image
from drv2asm import (IDENT_RE, BRANCH_MN, NAME_RE, parse_ada_lst,
                     rewrite_insn, emit_db, emit_db_l, emit_code,
                     emit_code_lst, parse_map)


def rewrite_insn_mz(text, code_defs, data_defs, name_addr, seg_at,
                    seg_img_base, para_to_name, entry_by_key, self_seg):
    """MZ variant of drv2asm.rewrite_insn.

    seg_at(off) -> map segment name containing image off (or None).
    seg_img_base[name] -> image offset of that segment's base.
    para_to_name[para] -> map segment name at image para.
    entry_by_key[(para,off)] -> routine/label name for far calls.
    self_seg -> map name of the segment currently emitted.
    """
    if re.match(r'\s*(j\w+|loop\w*|jcxz)\s+near', text):
        return None                      # 386+ near jcc
    if re.match(r'\s*(align|org|even)\b', text):
        return None
    branch = bool(BRANCH_MN.match(text))
    m = re.search(r'\b(call|jmp)\s+far\s+ptr\s+([0-9A-Fa-f]+)h:([0-9A-Fa-f]+)h',
                  text)
    if m:
        tgt = entry_by_key.get((int(m.group(2), 16), int(m.group(3), 16)))
        if not tgt:
            return None                  # far ptr with no routine at target
        text = (text[:m.start()] + m.group(1) + ' far ptr ' + tgt +
                text[m.end():])
    out, pos = [], 0
    for i, tm in enumerate(IDENT_RE.finditer(text)):
        nm = tm.group(1)
        if i == 0 or nm.lower() in ASM_WORDS_MZ or nm in seg_img_base:
            continue                      # mnemonic / keyword / segment name
        after = text[tm.end():]
        add = re.match(r'\s*([+-]\s*[0-9A-Fa-f]+h)\s*', after)
        addend = add.group(1) if add else ''
        tail = after[add.end():] if add else after
        bracketed = tail.lstrip().startswith('[')

        def num_ref(imgoff):
            """image offset -> operand text in containing segment's frame."""
            sname = seg_at(imgoff)
            if sname is None:
                return None
            v = (imgoff - seg_img_base[sname]) & 0xFFFF
            if bracketed:
                return f'0{v:X}h'
            if addend:
                return f'[0{v:X}h{addend}]'
            return f'[0{v:X}h]' if not branch else None

        if nm in data_defs or nm in code_defs:
            if branch and not bracketed and not addend and \
                    nm in data_defs and nm not in code_defs:
                return None              # bare branch to a data label
            rep = nm
        elif nm in name_addr:
            rep = num_ref(name_addr[nm])
            if rep is None:
                return None
        elif re.fullmatch(r'(byte|word|dword|unk|off|loc|sub|proc)_'
                          r'[0-9A-Fa-f]+', nm):
            v = (int(nm.rsplit('_', 1)[1], 16) - 0x10000) & 0xFFFFF
            rep = num_ref(v)
            if rep is None:
                return None
        else:
            return None                  # unknown identifier -> db
        out.append(text[pos:tm.start()])
        out.append(rep)
        pos = tm.end() + (add.end() if add and not bracketed else 0)
    if not out:
        return text
    return ''.join(out) + text[pos:]


ASM_WORDS_MZ = None  # built in main()


def emit_db_l_mz(f, img, a, b, dlabels, code_seg):
    """emit_db + label definitions for labels STRICTLY inside (a,b).

    Labels in CODE segments emit as plain `name:` (usable as near-branch
    targets); in DATA/STACK segs as `name label byte|word` typed labels."""
    pos = a
    for off in sorted(k for k in dlabels if a < k < b):
        emit_db(f, img[pos:off])
        for nm in dlabels[off]:
            if code_seg:
                f.write(f'{nm}:\n')
            else:
                t = ('word' if nm.startswith(('word_', 'off_')) else
                     'dword' if nm.startswith(('dword_',)) else 'byte')
                f.write(f'{nm} label {t}\n')
        pos = off
    if pos < b:
        emit_db(f, img[pos:b])


def emit_code_lst_mz(f, img, lo, hi, instr, labels, code_defs, data_defs,
                     name_addr, seg_at, seg_img_base, para_to_name,
                     entry_by_key, self_seg, self_name=None):
    """emit_code_lst variant using rewrite_insn_mz (multi-segment)."""
    addrs = sorted(a for a in instr if lo <= a <= hi)
    off = lo
    for a in addrs:
        if a < off:
            continue
        if a > off:
            emit_db(f, img[off:a])
        for nm in labels.get(a, []):
            if nm != self_name:
                f.write(f'{nm}:\n')
        nxt = min([x for x in addrs if x > a], default=hi + 1)
        nxt = min(nxt, hi + 1)
        raw = img[a:nxt]
        text = rewrite_insn_mz(instr[a], code_defs, data_defs, name_addr,
                               seg_at, seg_img_base, para_to_name,
                               entry_by_key, self_seg)
        if text:
            f.write(f'        {text}        ; @{a:X}:{len(raw)}\n')
        else:
            f.write('        db ' + ','.join(f'0{b:02X}h' for b in raw)
                    + f'        ; {instr[a]}\n')
        off = nxt
    if off <= hi:
        emit_db(f, img[off:hi + 1])


def main():
    global ASM_WORDS_MZ
    exe, mapfile, outasm = sys.argv[1], sys.argv[2], sys.argv[3]
    lst = None
    if '--lst' in sys.argv:
        lst = sys.argv[sys.argv.index('--lst') + 1]

    exefile = open(exe, 'rb').read()
    img = load_image(exe)
    # MZ entry: CS:IP at file 0x14/0x16, relative to load-module base
    eip, ecs = struct.unpack_from('<HH', exefile, 0x14)
    entry_img = ecs * 16 + eip
    hdr_paras = struct.unpack_from('<H', exefile, 8)[0]
    exefile = None

    segs, routines = parse_map(mapfile)
    # order segments by paragraph; record kinds for attributes
    seg_lines = []
    for line in open(mapfile):
        m = re.match(r'^(\w+) (CODE|DATA|STACK) ([0-9a-f]+)', line)
        if m:
            seg_lines.append((m.group(1), m.group(2), int(m.group(3), 16)))
    seg_lines.sort(key=lambda t: t[2])
    seg_names = [n for n, _, _ in seg_lines]
    seg_kind = {n: k for n, k, _ in seg_lines}
    seg_para = {n: p for n, _, p in seg_lines}
    seg_img_base = {n: p * 16 for n, _, p in seg_lines}
    para_to_name = {p: n for n, _, p in seg_lines}
    data_seg = next((n for n, k, _ in seg_lines if k == 'DATA'), None)

    def seg_at(imgoff):
        """map segment name containing image offset imgoff."""
        name = None
        for n, _, p in seg_lines:
            if p * 16 <= imgoff:
                name = n
        return name

    # ASM words += map segment names so rewrite keeps `seg dseg` etc.
    from drv2asm import ASM_WORDS
    ASM_WORDS_MZ = ASM_WORDS | set(seg_names)

    # lst parse: ada names its segments seg000 (all leading code merged),
    # dseg, seg_stack. Map segs may be finer-grained (Code1..CodeN), so
    # align by role: the lst's first code seg covers image base 0.
    instr = labels = None
    name_addr = {}
    seg_base = {n: seg_img_base[n] for n in seg_names}
    data_seg_para = seg_para.get(data_seg)
    stack_names = [n for n, k, _ in seg_lines if k == 'STACK']
    seg_base['seg000'] = 0
    if data_seg_para is not None:
        seg_base['dseg'] = seg_img_base[data_seg]
    if stack_names:
        seg_base['seg_stack'] = seg_img_base[stack_names[0]]
    if lst:
        instr, labels = parse_ada_lst(lst, seg_base)
        for a, nms in labels.items():
            for nm in nms:
                name_addr[nm] = a

    # code-name / data-name partitioning
    rt_by_seg = {}
    for r in routines:
        rt_by_seg.setdefault(r['seg'], []).append(r)
    code_extent = [(seg_img_base[r['seg']] + r['lo'],
                    seg_img_base[r['seg']] + r['hi'])
                   for r in routines]
    entry_by_key = {}            # (para, off) -> routine/label name
    for r in routines:
        entry_by_key[(seg_para[r['seg']], r['lo'])] = r['name']

    code_defs = {r['name'] for r in routines}
    data_labels = {}             # img off -> [names]
    data_defs = set()
    if lst:
        for a, nms in labels.items():
            if a in instr and any(lo <= a <= hi for lo, hi in code_extent):
                code_defs.update(nms)
        refset = {tm.group(1) for t in instr.values()
                  for tm in IDENT_RE.finditer(t)}
        for a, nms in labels.items():
            if not any(lo <= a <= hi for lo, hi in code_extent):
                keep = [nm for nm in nms if nm in refset]
                if keep:
                    data_labels.setdefault(a, []).extend(keep)
                    data_defs.update(keep)
        # ada labels inside code extents are also far-call targets
        for a, nms in labels.items():
            sn = seg_at(a)
            if sn is None:
                continue
            for nm in nms:
                entry_by_key.setdefault(
                    (seg_para[sn], (a - seg_img_base[sn]) & 0xFFFF), nm)

    # ensure an entrypoint label exists at the image entry offset
    if 'start' not in name_addr:
        labels = labels or {}
        labels.setdefault(entry_img, [])
        if 'start' not in labels[entry_img]:
            labels[entry_img].append('start')
        name_addr.setdefault('start', entry_img)
        code_defs.add('start')
    entry_name = 'start'

    with open(outasm, 'w') as f:
        f.write('; auto-generated MZ skeleton: ' + os.path.basename(exe) + '\n')
        f.write('; generated by tools/exe2asm.py from ' + mapfile + '\n\n')
        f.write('.8086\n\n')

        for si, (sname, kind, para) in enumerate(seg_lines):
            base = para * 16
            nxt = (seg_lines[si + 1][2] * 16
                   if si + 1 < len(seg_lines) else len(img))
            in_img_end = min(nxt, len(img))
            align = 'byte' if kind == 'CODE' else 'para'
            comb = "stack" if kind == 'STACK' else 'public'
            f.write(f"{sname} segment {align} {comb} '{kind}' use16\n")
            acs = f'cs:{sname}' if kind == 'CODE' else 'cs:nothing'
            ads = f'ds:{data_seg}' if data_seg else 'ds:nothing'
            f.write(f'        assume {acs}, {ads}, es:nothing, ss:nothing\n')

            srt = sorted(rt_by_seg.get(sname, []), key=lambda r: r['lo'])
            pos = base
            if kind == 'STACK':
                stored = max(0, in_img_end - base)
                emit_db(f, img[base:base + stored])
                rest = nxt - max(in_img_end, base)
                if rest > 0:
                    f.write(f'        db 0{rest:X}h dup(?)\n')
            else:
                for r in srt:
                    rlo, rhi = base + r['lo'], base + r['hi']
                    # extents may overhang the segment para boundary (a
                    # routine's trailing chunk physically lands in the next
                    # segment's space) -- emit only what lies in this seg
                    rhi = min(rhi, nxt - 1)
                    if rlo > pos:
                        emit_db_l_mz(f, img, pos, rlo, data_labels,
                                     kind == 'CODE')
                    if rlo < pos or rlo >= nxt:
                        continue
                    f.write(f"{r['name']} proc {r['type'].lower()}\n")
                    if lst:
                        emit_code_lst_mz(f, img, rlo, rhi, instr, labels,
                                         code_defs, data_defs, name_addr,
                                         seg_at, seg_img_base, para_to_name,
                                         entry_by_key, sname,
                                         self_name=r['name'])
                    else:
                        emit_code(f, img[rlo:rhi + 1], r['lo'])
                    f.write(f"{r['name']} endp\n")
                    pos = rhi + 1
                if pos < in_img_end:
                    emit_db_l_mz(f, img, pos, in_img_end, data_labels,
                                 kind == 'CODE')
            f.write(f'{sname} ends\n\n')
            # BSS gap before the next segment (uninitialized memory between
            # the stored image end and e.g. the stack): reserve as a 'BSS'
            # class segment so LINK does not write it to the file.
            gap = nxt - in_img_end
            if gap > 0 and si + 1 < len(seg_lines):
                f.write("bss segment para public 'BSS'\n")
                f.write(f'        db 0{gap:X}h dup(?)\n')
                f.write('bss ends\n\n')
        f.write(f'        end {entry_name}\n')
    print(f'wrote {outasm}')


if __name__ == '__main__':
    main()
