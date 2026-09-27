#!/usr/bin/env python3
"""Generate a byte-exact MASM skeleton .asm for a MicroProse driver overlay
(*GRAPHIC.EXE / *SOUND.EXE) from its binary + mzmap routine map.

The drivers have no call-reachable exports (entries come from the OvlHeader
jump table), so the skeleton is intentionally simple:

- seg000 (data): OvlHeader emitted as asm (`dw seg seg001`, `dw offset
  <handler>`) so LINK reproduces the original stored values AND relocation
  entries; remaining bytes emitted verbatim.
- seg001 (code): each map routine becomes `name proc far` + `db` bytes with
  the ndisasm decoding as a comment, `endp` -- byte-exact by construction.
  Bytes not claimed by any routine are emitted as data.

Usage: drv2asm.py DRIVER.EXE map/x.map out.asm [--lst ada.lst]

With --lst, code is emitted as real mnemonics from an Ada Script
(vextest ada.py) listing instead of db+comment lines. A mnemonic is kept
only when every referenced label resolves in the emitted file; otherwise
the instruction falls back to `db` bytes (still byte-exact).
"""
import re
import struct
import subprocess
import sys
import os

sys.path.insert(0, os.path.dirname(__file__))
from compare_exe import load_image

NAME_RE = re.compile(r'\b(loc|sub|byte|word|dword|unk|off|proc)_([0-9A-Fa-f]{4,})\b')


def parse_ada_lst(path, seg_base=None):
    """ada .lst -> {image off: instr text}, {image off: [label names]}.

    segNNN:off fields are segment-relative; seg_base maps the lst segment
    name to its image-offset base (map paragraph * 16). Without seg_base
    (or for unknown segments) only flat seg000 lines at base 0 are kept --
    the pre-IDC listing layout. Ada name suffixes encode LINEAR addresses
    (image base 0x10000)."""
    instr = {}        # image off -> text
    labels = {}       # image off -> [names]
    for line in open(path, encoding='utf-8', errors='replace'):
        m = re.match(r'seg(\w+):([0-9A-F]+)\s+(.*)', line)
        if not m:
            continue
        segname = 'seg' + m.group(1)
        if seg_base is None:
            base = 0 if m.group(1) == '000' else None
        else:
            base = seg_base.get(segname)
        if base is None:                 # unknown segment (e.g. seg_stack)
            continue
        off, rest = base + int(m.group(2), 16), m.group(3).strip()
        if not rest or rest.startswith(';'):
            continue
        pm = re.match(r'([\w$]+)\s+proc\s+(near|far)', rest)
        if pm:
            labels.setdefault(off, []).append(pm.group(1))
            continue
        if re.match(r'[\w$]+\s+endp', rest):
            continue
        lm = re.match(r'([\w$]+):', rest)
        if lm:
            labels.setdefault(off, []).append(lm.group(1))
            continue
        if ' segment ' in rest or ' equ ' in rest or rest.endswith(' ends'):
            continue
        instr[off] = rest.split(';')[0].rstrip()
    return instr, labels


IDENT_RE = re.compile(r'(?<![0-9A-Za-z_$.])([A-Za-z_$][\w$]*)')
ASM_WORDS = {'ptr', 'short', 'near', 'far', 'offset', 'seg', 'byte', 'word',
             'dword', 'qword', 'tbyte', 'fword', 'low', 'high', 'opattr',
             'seg000', 'seg001', 'seg002', 'seg003', 'seg004', 'seg005',
             'db', 'dw', 'dd', 'dp', 'dq', 'dt', 'dup', 'align', 'org',
             'even', 'type', 'this', 'label',
             'al', 'ah', 'ax', 'eax', 'bl', 'bh', 'bx', 'ebx', 'cl', 'ch',
             'cx', 'ecx', 'dl', 'dh', 'dx', 'edx', 'si', 'esi', 'di', 'edi',
             'bp', 'ebp', 'sp', 'esp', 'cs', 'ds', 'es', 'ss', 'fs', 'gs',
             'rep', 'repe', 'repne', 'repz', 'repnz', 'lock', 'stos', 'lods',
             'movs', 'scas', 'cmps', 'stosb', 'stosw', 'lodsb', 'lodsw',
             'movsb', 'movsw', 'scasb', 'scasw', 'cmpsb', 'cmpsw'}


def rewrite_insn(text, code_defs, data_defs, name_addr, code_lin0,
                 seg_para, entry_by_off):
    """Rewrite one ada instruction line for uasm. Returns text or None -> db.

    - names emitted as labels (data_defs in seg000, code_defs in seg001)
      stay symbolic so MASM emits relocatable mod=10 disp16 operands, matching
      the original assembler's output.
    - ada names at image offsets we don't label decode to their numeric
      image offset; bare ones wrap in [..]; bare ones under a branch -> db.
    - `call/jmp far ptr NUM:NUM` with NUM == code para -> `call far ptr name`.
    """
    if re.match(r'\s*(j\w+|loop\w*|jcxz)\s+near', text):
        return None                      # 386+ near jcc
    if re.match(r'\s*(align|org|even)\b', text):
        return None
    branch = bool(BRANCH_MN.match(text))
    m = re.search(r'\b(call|jmp)\s+far\s+ptr\s+([0-9A-Fa-f]+)h:([0-9A-Fa-f]+)h',
                  text)
    if m:
        if int(m.group(2), 16) != seg_para:
            return None                  # far ptr to another segment
        tgt = entry_by_off.get(int(m.group(3), 16))
        if not tgt:
            return None                  # no routine at that offset
        text = (text[:m.start()] + m.group(1) + ' ' + tgt +
                text[m.end():])
    out, pos = [], 0
    for i, tm in enumerate(IDENT_RE.finditer(text)):
        nm = tm.group(1)
        if i == 0 or nm.lower() in ASM_WORDS:
            continue                      # mnemonic / keyword / segment name
        after = text[tm.end():]
        add = re.match(r'\s*([+-]\s*[0-9A-Fa-f]+h)\s*', after)
        addend = add.group(1) if add else ''
        tail = after[add.end():] if add else after
        # already-bracketed: `name[reg]` / `name+Kh[reg]` -> bare numeric
        bracketed = tail.lstrip().startswith('[')

        def num_ref(v):
            # bracketed keeps the +Kh tail in the source text
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
            v = name_addr[nm]
            if v >= code_lin0:
                return None              # code-region name we don't emit
            rep = num_ref(v & 0xFFFF)
            if rep is None:
                return None
        elif re.fullmatch(r'(byte|word|dword|unk|off|loc|sub|proc)_'
                          r'[0-9A-Fa-f]+', nm):
            # suffixed name absent from labels: decode linear suffix
            v = (int(nm.rsplit('_', 1)[1], 16) - 0x10000) & 0xFFFF
            if code_lin0 <= v < 0xFFF0:
                return None              # would-be code ref we can't emit
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


def emit_db_l(f, img, a, b, dlabels):
    """emit_db + `name label T` definitions for labels STRICTLY inside (a,b).
    Labels at `a` are the caller's responsibility (mark processing)."""
    pos = a
    for off in sorted(k for k in dlabels if a < k < b):
        emit_db(f, img[pos:off])
        for nm in dlabels[off]:
            t = 'word' if nm.startswith(('word_', 'off_')) else 'byte'
            f.write(f'{nm} label {t}\n')
        pos = off
    if pos < b:
        emit_db(f, img[pos:b])


def parse_map(path):
    segs, routines = {}, []
    for line in open(path):
        m = re.match(r'^(\w+) (CODE|DATA|STACK) ([0-9a-f]+)', line)
        if m:
            segs[m.group(1)] = int(m.group(3), 16)
            continue
        m = re.match(r'^([\w.]+): (\w+) (NEAR|FAR) ([0-9a-f]+)-([0-9a-f]+)', line)
        if m:
            routines.append(dict(name=m.group(1), seg=m.group(2), type=m.group(3),
                                 lo=int(m.group(4), 16), hi=int(m.group(5), 16)))
    return segs, routines


def disasm_span(data, org):
    """ndisasm a span -> [(bytes, text)] roughly aligned per instruction."""
    p = subprocess.run(['ndisasm', '-b16', '-o', hex(org), '/dev/stdin'],
                       input=data, capture_output=True)
    out = []
    for line in p.stdout.decode().splitlines():
        m = re.match(r'([0-9A-F]+)\s+([0-9A-F]+)\s+(.*)', line)
        if m:
            out.append((bytes.fromhex(m.group(2)), m.group(3)))
    return out


def emit_db(f, data, comment=''):
    for i in range(0, len(data), 12):
        chunk = data[i:i + 12]
        f.write('        db ' + ','.join(f'0{b:02X}h' if b > 9 else str(b)
                                         for b in chunk))
        f.write('\n')


def emit_code(f, data, org):
    """emit instructions: bytes as db + disasm text as comments"""
    pos = 0
    for insn, text in disasm_span(data, org):
        f.write(f'        db ' + ','.join(f'0{b:02X}h' for b in insn)
                + f'        ; {text}\n')
        pos += len(insn)
    if pos < len(data):  # tail bytes ndisasm couldn't decode
        emit_db(f, data[pos:])


BRANCH_MN = re.compile(
    r'^(call|jmp|ret[nf]?|iret|int|into|j\w+|loop\w*|jcxz)\b')


def emit_code_lst(f, img, lo, hi, instr, labels, code_defs, data_defs,
                  name_addr, code_lin0, seg_para, entry_by_off,
                  self_name=None):
    """Emit a routine body from the ada listing; lo/hi are image offsets.

    Keeps ada mnemonics after rewrite_insn name resolution; instructions we
    cannot represent (out-of-segment refs, missing labels, 386+ encodings)
    fall back to db bytes. Emitted lines carry '; @OFF:LEN' tags so a failed
    assemble can be repaired by turning the line into its raw bytes.
    """
    addrs = sorted(a for a in instr if lo <= a <= hi)
    off = lo
    for a in addrs:
        if a < off:
            continue
        if a > off:  # unlisted gap inside the extent
            emit_db(f, img[off:a])
        for nm in labels.get(a, []):
            if nm != self_name:          # proc header already defines it
                f.write(f'{nm}:\n')
        nxt = min([x for x in addrs if x > a], default=hi + 1)
        nxt = min(nxt, hi + 1)
        raw = img[a:nxt]
        text = rewrite_insn(instr[a], code_defs, data_defs, name_addr,
                            code_lin0, seg_para, entry_by_off)
        if text:
            f.write(f'        {text}        ; @{a:X}:{len(raw)}\n')
        else:
            f.write('        db ' + ','.join(f'0{b:02X}h' for b in raw)
                    + f'        ; {instr[a]}\n')
        off = nxt
    if off <= hi:
        emit_db(f, img[off:hi + 1])


def main():
    exe, mapfile, outasm = sys.argv[1], sys.argv[2], sys.argv[3]
    lst = None
    if '--lst' in sys.argv:
        lst = sys.argv[sys.argv.index('--lst') + 1]
    img = load_image(exe)
    segs, routines = parse_map(mapfile)
    instr = labels = None
    name_addr = {}
    # lst segment names (seg000/seg001 by para order) -> image-offset base
    seg_lstnames = {name: f'seg{i:03d}' for i, (name, para) in
                    enumerate(sorted(segs.items(), key=lambda kv: kv[1]))}
    seg_base = {seg_lstnames[n]: p * 16 for n, p in segs.items()}
    if lst:
        instr, labels = parse_ada_lst(lst, seg_base)
        # every ada-defined name -> its image offset
        for a, nms in labels.items():
            for nm in nms:
                name_addr[nm] = a

    # seg order by paragraph; seg000 = para 0 (header/data), seg001 = code
    seg_names = {name: f'seg{i:03d}' for i, (name, para) in
                 enumerate(sorted(segs.items(), key=lambda kv: kv[1]))}
    seg_para = {name: para for name, para in segs.items()}
    code_seg_name = max(seg_para, key=seg_para.get)  # last seg = code (seg001)
    data_seg_name = min(seg_para, key=seg_para.get)

    # header fields
    code_para, base_para, first, size1, size2, count = struct.unpack_from('<6H', img, 0x18)
    slots = struct.unpack_from(f'<{count}H', img, 0x24)
    slot_off = {w: i + first for i, w in enumerate(slots)}
    entry_by_off = {r['lo']: r['name'] for r in routines if r['seg'] == code_seg_name}

    code_lin0 = seg_para[code_seg_name] * 16
    data_end = code_lin0

    # names that will exist as code labels in the emitted file: ada labels at
    # instruction boundaries inside emitted routine extents + my routine names
    code_defs = set(entry_by_off.values())
    data_labels = {}        # image off (< code_lin0) -> [names] to emit
    data_defs = set()
    if lst:
        covered = [(code_lin0 + r['lo'], code_lin0 + r['hi'])
                   for r in routines if r['seg'] == code_seg_name]
        for a, nms in labels.items():
            if a in instr and any(lo <= a <= hi for lo, hi in covered):
                code_defs.update(nms)
        # ada names referenced by any instruction
        refset = {tm.group(1) for t in instr.values()
                  for tm in IDENT_RE.finditer(t)}
        for a, nms in labels.items():
            if a < code_lin0:
                keep = [nm for nm in nms if nm in refset]
                if keep:
                    data_labels[a] = keep
                    data_defs.update(keep)

    with open(outasm, 'w') as f:
        f.write('; auto-generated driver skeleton: ' + os.path.basename(exe) + '\n')
        f.write('; generated by tools/drv2asm.py from ' + mapfile + '\n\n')
        # ---- data segment (seg000): header + tables, verbatim ----
        f.write(f'{seg_names[data_seg_name]} segment byte public \'CODE\'\n')
        f.write('        assume cs:nothing, ds:nothing, es:nothing, ss:nothing\n')
        fields = {0x18: f'dw seg {seg_names[code_seg_name]}        ; code_segment',
                  0x1A: f'dw seg {seg_names[data_seg_name]}        ; base_segment',
                  0x1C: f'dw {first}        ; first_slot',
                  0x1E: f'dw 0{size1:X}h        ; size1',
                  0x20: f'dw 0{size2:X}h        ; size2',
                  0x22: f'dw {count}        ; jump_count'}
        slotat = {0x24 + 2 * k: w for k, w in enumerate(slots)}
        marks = sorted(set(fields) | set(slotat) | set(data_labels)
                       | {0, data_end})
        pos = 0
        for m in marks:
            if m > pos:
                emit_db_l(f, img, pos, m, data_labels)
                pos = m
            for nm in data_labels.get(m, ()):
                t = 'word' if nm.startswith(('word_', 'off_')) else 'byte'
                f.write(f'{nm} label {t}\n')
            if m in fields:
                f.write(f'        {fields[m]}\n')
                pos += 2
            elif m in slotat:
                w = slotat[m]
                tgt = entry_by_off.get(w)
                if tgt:
                    f.write(f'        dw offset {tgt} '
                            f'; slot {m // 2 - 0x12:#04x}\n')
                else:
                    f.write(f'        dw 0{w:X}h                '
                            f'; slot {m // 2 - 0x12:#04x} (mid-proc)\n')
                pos += 2
        if pos < data_end:
            emit_db_l(f, img, pos, data_end, data_labels)
        f.write(f'{seg_names[data_seg_name]} ends\n\n')

        # ---- code segment (seg001) ----
        f.write(f'{seg_names[code_seg_name]} segment byte public \'CODE\'\n')
        f.write(f'        assume cs:{seg_names[code_seg_name]}, ds:{seg_names[data_seg_name]}, es:nothing, ss:nothing\n')
        pos = 0
        for r in sorted(routines, key=lambda r: r['lo']):
            if r['seg'] != code_seg_name:
                continue
            if r['lo'] > pos:  # unclaimed gap
                emit_db(f, img[code_lin0 + pos: code_lin0 + r['lo']])
            if r['lo'] < pos:
                continue  # routine overlaps an earlier extent; skip dup bytes
            body = img[code_lin0 + r['lo']: code_lin0 + r['hi'] + 1]
            f.write(f"{r['name']} proc {r['type'].lower()}\n")
            if lst:
                emit_code_lst(f, img, code_lin0 + r['lo'],
                              code_lin0 + r['hi'], instr, labels,
                              code_defs, data_defs, name_addr, code_lin0,
                              seg_para[code_seg_name], entry_by_off,
                              self_name=r['name'])
            else:
                emit_code(f, body, r['lo'])
            f.write(f"{r['name']} endp\n")
            pos = r['hi'] + 1
        if pos < len(img) - code_lin0:
            emit_db(f, img[code_lin0 + pos:])
        f.write(f'{seg_names[code_seg_name]} ends\n')
        f.write('        end\n')
    print(f'wrote {outasm}')


if __name__ == '__main__':
    main()
