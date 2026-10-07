#!/usr/bin/env python3
"""Trace both guests for one near16 vector; print tail steps + ax history."""
import sys
sys.path.insert(0, '/home/xor/vextest')
sys.path.insert(0, '/home/xor/vextest/angr_platforms')
import json
import unicorn
from pathlib import Path
from tools.dosunit.reporting.real16_replay_cli import _image, _manifest
from tools.dosunit.runtime.real16_guest import _initialize_guest
from tools.dosunit.runtime.real16_replay_model import Real16ReplayPolicy
from tools.dosunit.reporting.real16_replay_manifest import parse_manifest

VEC = sys.argv[1] if len(sys.argv) > 1 else '/home/xor/games/f19ru/F19/dosunit/probe_egame_1.g1.vectors.json'
OEXE = sys.argv[2] if len(sys.argv) > 2 else '/home/xor/games/f19ru/F19/build/EGAME-ints-5f1cf5d8.EXE'
CEXE = sys.argv[3] if len(sys.argv) > 3 else '/home/xor/games/f19ru/F19/build/EGFRAME-ints-4cbc75f4.EXE'
VID = sys.argv[4] if len(sys.argv) > 4 else 'fireGroundThreat#0'

manifest = parse_manifest(json.loads(open(VEC).read()))
sel = next(v for v in manifest.vectors if v.vector_id == VID)

def load(path, ranges):
    data = Path(path).read_bytes()
    return data, _image(data, Path(path), manifest.oracle_load_segment,
                        ranges)

o_raw, o_img = load(OEXE, manifest.oracle_code_ranges)
c_raw, c_img = load(CEXE, manifest.candidate_code_ranges)
policy = Real16ReplayPolicy()

UC_AX = unicorn.x86_const.UC_X86_REG_AX
UC_CS = unicorn.x86_const.UC_X86_REG_CS
UC_IP = unicorn.x86_const.UC_X86_REG_IP

def run(image, entry):
    guest = _initialize_guest(image, entry, sel.vector, policy)
    trace = []
    writes = []
    def hook_code(uc, address, size, user):
        trace.append((address, uc.reg_read(UC_AX)))
    def hook_write(uc, access, address, size, value, user):
        writes.append((address, value, size))
    guest.hook_add(unicorn.UC_HOOK_CODE, hook_code)
    guest.hook_add(unicorn.UC_HOOK_MEM_WRITE, hook_write)
    try:
        guest.emu_start(entry.linear(), sel.vector.frame.target.linear(), count=2000)
    except Exception as e:
        trace.append(('ERR', str(e)))
    return trace, writes

o_trace, o_writes = run(o_img, sel.oracle_entry)
c_trace, c_writes = run(c_img, sel.candidate_entry)
o_base = sel.oracle_entry.linear()
c_base = sel.candidate_entry.linear()
print('O entry', hex(o_base), 'C entry', hex(c_base))
print('--- O tail (addr, ax) ---')
for a, x in o_trace[-20:]:
    print(f'  {a:#x} ax={x:#x}' if a != 'ERR' else f'  ERR {x}')
print('--- C tail (addr, ax) ---')
for a, x in c_trace[-20:]:
    print(f'  {a:#x} ax={x:#x}' if a != 'ERR' else f'  ERR {x}')
print('O len', len(o_trace), 'C len', len(c_trace))
