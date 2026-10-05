#!/usr/bin/env python3
"""Trace both guests for one near16 vector; print first execution divergence."""
import sys
sys.path.insert(0, '/home/xor/vextest')
sys.path.insert(0, '/home/xor/vextest/angr_platforms')
import json
import unicorn
from pathlib import Path
from tools.dosunit.real16_replay_cli import _image, _manifest
from tools.dosunit.real16_guest import _initialize_guest
from tools.dosunit.real16_replay_model import Real16ReplayPolicy
from tools.dosunit.real16_replay_manifest import parse_manifest

VEC = '/home/xor/games/f19ru/F19/dosunit/probe_egame_1.g1.vectors.json'
OEXE = '/home/xor/games/f19ru/F19/build/EGAME-ints-5f1cf5d8.EXE'
CEXE = '/home/xor/games/f19ru/F19/build/EGFRAME-ints-4cbc75f4.EXE'
VID = 'fireGroundThreat#0'

manifest = parse_manifest(json.loads(open(VEC).read()))
sel = next(v for v in manifest.vectors if v.vector_id == VID)

def load(path, ranges):
    data = Path(path).read_bytes()
    return data, _image(data, Path(path), manifest.oracle_load_segment,
                        ranges)

o_raw, o_img = load(OEXE, manifest.oracle_code_ranges)
c_raw, c_img = load(CEXE, manifest.candidate_code_ranges)
policy = Real16ReplayPolicy()

def run(image, entry, tag):
    guest = _initialize_guest(image, entry, sel.vector, policy)
    trace = []
    writes = []
    def hook_code(uc, address, size, user):
        cs = uc.reg_read(0x1016000A)  # UC_X86_REG_CS
        ip = address - cs * 16
        trace.append((cs, ip))
    def hook_write(uc, access, address, size, value, user):
        writes.append((address, value, size))
    guest.hook_add(unicorn.UC_HOOK_CODE, hook_code)
    guest.hook_add(unicorn.UC_HOOK_MEM_WRITE, hook_write)
    try:
        guest.emu_start(entry.linear(), sel.vector.frame.target.linear(), count=2000)
    except Exception as e:
        trace.append(('ERR', str(e)))
    return trace, writes

o_trace, o_writes = run(o_img, sel.oracle_entry, 'O')
c_trace, c_writes = run(c_img, sel.candidate_entry, 'C')
print('trace lens:', len(o_trace), len(c_trace))
for i, (a, b) in enumerate(zip(o_trace, c_trace)):
    if a != b:
        print('first divergence at step', i)
        for j in range(max(0, i - 6), min(len(o_trace), i + 10)):
            oo = o_trace[j] if j < len(o_trace) else '-'
            cc = c_trace[j] if j < len(c_trace) else '-'
            print(f'  {j}: O={oo} C={cc}')
        break
else:
    print('no divergence in common prefix')
print('O writes:', o_writes)
print('C writes:', c_writes)
