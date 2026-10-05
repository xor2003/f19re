#!/usr/bin/env python3
# Generates dosunit/egame_state2.json - EGAME combat/frame/map-state cluster.
#
# Same conventions as gen_egame_tac.py / gen_egame_state.py:
#   EN dseg offset = IDA flat - 0x2ECF0; patches are little-endian bytes.
#   Oracle and candidate cell offsets differ -> asymmetric (o_off, c_off).
#   Far-pointer cells are patched so far reads land in the shared DS scratch.
import sys, os, struct, re
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))) + '/tools')
from duspec import emit, cand_off

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
EG = 'EGAME'
FR = 'EGFRAME'; CB = 'EGCOMBAT'; DM = 'EG3DMAP'; FL = 'EGFLIGHT'

def c(mod, sym):
    v = cand_off(os.path.join(ROOT, 'build/%s.MAP' % mod), sym)
    assert v is not None, (mod, sym)
    return v

def w16(v): return struct.pack('<H', v & 0xFFFF).hex()
def b8(v):  return struct.pack('<B', v & 0xFF).hex()

# EN oracle dseg cells (verified against image disasm)
O = {
    'recCount': 0x632E, 'replayLog': 0x8E46, 'missionTick': 0x6630,
    'viewX': 0x94EE, 'viewY': 0x94FE, 'viewZ': 0x46E0, 'ourHead': 0x46DA,
    'setupSlots': 0x8794,        # int16[] stride 2 (not the 0x12 table!)
    'frameTick': 0x5520, 'nightMode': 0x4EF4, 'unusedFrameVal': 0x65C0,
    'rngState': 0x63A0, 'seedDone': 0x4EF6,
    'storeDefs': 0x80AA,         # stride 16: subIdx+0 coordX+2 coordY+4 nameIdx+0xE
    'nameTab': 0x9678, 'nameBuf': 0x65C6,
    'flagFtN': 0x963C, 'farPtr': 0x6330,   # off/seg pair at 0x6330/0x6332
    'commData': 0x9C82,          # far ptr off/seg at 0x9C82/0x9C84
    'smokeSrc': 0x5528, 'smokeSlot': 0x527A, 'particles': 0x523A,  # stride 8
    'threatInit': 0x86FC, 'threatActive': 0x5522,
    'mapEvents': 0x520A,         # 12B recs: mapX+0 mapY+2 ttl+8
    'threatRefX': 0x8B24, 'threatRefY': 0x8B2E, 'threatRefZ': 0x8B34,
    'threatRefHead': 0x663C, 'unusedHist0': 0x9502,
    'planeScanCnt': 0x9C78, 'missionStatus': 0x4EF0, 'diffTier': 0x4EF2,
    'planeTable': 0x80AA,        # MapTarget 16B stride: active+6 alertLevel+0xA
    'samSpecs': 0x486E,          # stride 14: name+0 lethality+8 dangerTier+0xA flags+0xC
    'mapCellFlags': 0x85FC, 'startRange': 0x7F94, 'threatScope': 0x551C,
    'projTab': 0x53FC, 'samSpeed': 0x4E6C, 'samRange': 0x4E6A,
    'frate': 0x4EFC, 'strBuf': 0x65C6,
    'modelStream': 0xA7C,        # far ptr off/seg
    'edgeCount': 0xAF0, 'wideVtx': 0xAF4, 'maskLo': 0xAF6, 'maskHi': 0xAF8,
    'orientMat': 0x4680,         # 9 int16 (word_33370..80)
    'aaHead': 0x46DA, 'aaPitch': 0x46DC, 'aaRoll': 0x46DE,
    'orientDirty': 0x46EF, 'rollWasNz': 0x46EC,
    # runtime-filled LUTs live in oracle BSS: sine interp table word_32466,
    # asin table word_32668; the cand uses the single const g_angleLut for both
    'sinLut': 0x3776, 'asinLut': 0x3978,
}

# stubs.c's const g_angleLut (Q15 sine, 260 entries) - the cand's table data;
# patched into the oracle's BSS LUTs so both sides see identical contents.
def _angle_lut_hex():
    src = open(os.path.join(ROOT, 'src/stubs.c')).read()
    body = src.split('g_angleLut[260] = {', 1)[1].split('};', 1)[0]
    vals = [int(t, 16) for t in re.findall(r'0x[0-9a-fA-F]+', body)]
    return struct.pack('<%dH' % len(vals), *vals).hex()

ANGLE_LUT = _angle_lut_hex()

DS = 0x6000
SC1, SC2, SC3 = 0x7000, 0x7200, 0x7400   # scratch cells inside DS

def P(o, m, s, v, size=2):
    return (o, c(m, s), size, w16(v) if size == 2 else b8(v))
def OBS(o, m, s, size=2):
    return (o, c(m, s), size)
def obsv(o, coff, size): return (o, coff, size)

cases = [
    # ---- recordFrame(a,b): replay-log 6B records; count>=0xFF -> no-op
    dict(fn='recordFrame', exe=EG, mod=FR, regs=[],
         vectors=[
             {'args': [a, b], 'patch':
                 [P(O['recCount'], FR, 'g_replayCount', cnt),
                  P(O['missionTick'], FR, 'g_missionTick', 0x111),
                  P(O['viewX'], FR, 'g_viewX_', 0x4020),
                  P(O['viewY'], FR, 'g_viewY_', 0x2091)],
              'obs': [OBS(O['recCount'], FR, 'g_replayCount'),
                      obsv(O['replayLog'] + 6 * cnt,
                           c(FR, 'g_replayLog') + 6 * cnt, 12)]}
             for cnt in [0, 1, 0xFE, 0xFF]
             for a, b in [(0, 0), (0x80, 0x41), (0xFF, 0xFF)]]),

    # initFrameRandom is intentionally absent: it calls clearStatusPanel ->
    # drawPanelText -> text renderer outside the declared code window
    # (fetch_outside_declared_code), so no vector can return.

    # ---- buildStoreName(i): nameTab[subIdx&0x7F] + optional nameTab[subIdx]
    dict(fn='buildStoreName', exe=EG, mod=FR, regs=[],
         obs=[obsv(O['nameBuf'], c(FR, 'g_nameBuf'), 32)],
         vectors=[
             {'args': [i], 'patch':
                 [(O['storeDefs'] + 16 * i, c(FR, 'g_storeDefs') + 16 * i,
                   2, w16(sub)),
                  (O['storeDefs'] + 16 * i + 0xE, c(FR, 'g_storeDefs') + 16 * i
                   + 0xE, 2, w16(ni)),
                  (O['nameTab'] + 2 * (ni & 0x7F),
                   c(FR, 'g_nameTab') + 2 * (ni & 0x7F), 2, w16(SC1)),
                  (O['nameTab'] + 2 * sub, c(FR, 'g_nameTab') + 2 * sub,
                   2, w16(SC2)),
                  (SC1, SC1, 8, s1.encode().hex()),
                  (SC2, SC2, 8, (s2.encode() or b'\0').hex())]}
             for i in [0, 3]
             for ni, s1 in [(2, 'MIG-29\0'), (0x85, 'A-10\0')]
             for sub, s2 in [(0, ''), (5, ' sq.\0'), (7, 'x' * 30 + '\0')]]),

    # ---- moveNearFar(nearOff,count): movedata far<->near + farPointer advance
    dict(fn='moveNearFar', exe=EG, mod=FR, regs=[],
         vectors=[
             {'args': [SC2, cnt], 'patch':
                 [P(O['flagFtN'], FR, 'flagFarToNear', fl),
                  (O['farPtr'], c(FR, 'farPointer'), 2, w16(SC3)),
                  (O['farPtr'] + 2, c(FR, 'farPointer') + 2, 2, w16(DS)),
                  (SC3, SC3, 16, 'A0B1C2D3E4F5060708090A0B0C0D0E0F'[:32]),
                  (SC2, SC2, 16, '11223344556677889900AABBCCDDEEFF'[:32])],
              'obs': [(SC2, SC2, 16), (SC3, SC3, 16),
                      (O['farPtr'], c(FR, 'farPointer'), 2)]}
             for fl in [0, 1]
             for cnt in [1, 4, 8]]),

    # ---- setCommWorldbufPtr(): farPointer = commData + 0x7A
    dict(fn='setCommWorldbufPtr', exe=EG, mod=FR, oof=0x4ed6, regs=['ax'],
         obs=[obsv(O['farPtr'], c(FR, 'farPointer'), 4)],
         vectors=[
             {'args': [], 'patch':
                 [(O['commData'], c(FR, 'commData'), 4, w16(off) + w16(seg))]}
             for off, seg in [(0, 0), (0x100, 0x5555), (0xFF00, 0x7FFF)]]),

    # ---- updateTracerParticles(): 8 particles + smoke-slot spawn
    dict(fn='updateTracerParticles', exe=EG, mod=FR, regs=[],
         vectors=[
             {'args': [], 'patch':
                 [P(O['smokeSrc'], FR, 'g_smokeSourceIdx', src),
                  P(O['frameTick'], FR, 'frameTick', tick),
                  (O['particles'], c(FR, 'g_particles'), 64,
                   (b''.join(struct.pack('<hhhh', 10 * i, 20 * i,
                                         0x200 + i, 0x3030 + i)
                             for i in range(8))).hex()),
                  (O['storeDefs'] + 16 * (src & 0x3F) + 2,
                   c(FR, 'g_storeDefs') + 16 * (src & 0x3F) + 2,
                   4, w16(0x333) + w16(0x444)),
                  P(O['rngState'], FR, 'g_rngSeed', 0x4321)],
              'obs': [obsv(O['particles'], c(FR, 'g_particles'), 64),
                      OBS(O['smokeSlot'], FR, 'g_smokeParticleSlot')]}
             for src in [-1, 0, 3]
             for tick in [0x10, 0x11, 0xF0]]),

    # ---- updateThreatAlert(): threatRef snapshot + alertLevel clamp loop
    dict(fn='updateThreatAlert', exe=EG, mod=CB, regs=[],
         vectors=[
             {'args': [], 'patch':
                 [P(O['threatInit'], CB, 'g_threatTimerInit', 0x250),
                  (O['mapEvents'] + 8, c(CB, 'mapEvents') + 8, 2, w16(ttl)),
                  (O['mapEvents'], c(CB, 'mapEvents'), 4, w16(mx) + w16(my)),
                  P(O['viewX'], CB, 'g_viewX_', 0x1111),
                  P(O['viewY'], CB, 'g_viewY_', 0x2222),
                  P(O['viewZ'], CB, 'g_viewZ', 0x333),
                  P(O['ourHead'], CB, 'g_ourHead', 0x4444),
                  P(O['planeScanCnt'], CB, 'g_planeScanCount', 3),
                  P(O['missionStatus'], CB, 'g_missionStatus', ms),
                  P(O['diffTier'], CB, 'g_difficultyTier', dt)]
                 + [(O['planeTable'] + 16 * i + 6, c(CB, 'g_planeTable')
                     + 16 * i + 6, 2, w16(act))
                    + (O['planeTable'] + 16 * i + 0xA, c(CB, 'g_planeTable')
                       + 16 * i + 0xA, 2, w16(al + i * 0x20))
                    for i, (act, al) in enumerate([(1, 0x50), (0, 0x80),
                                                   (1, 0x200)])],
              'obs': [OBS(O['threatActive'], CB, 'g_threatActiveTimer'),
                      OBS(O['threatRefX'], CB, 'g_threatRefX'),
                      OBS(O['threatRefY'], CB, 'g_threatRefY'),
                      OBS(O['threatRefZ'], CB, 'g_threatRefZ'),
                      OBS(O['threatRefHead'], CB, 'g_threatRefHead'),
                      OBS(O['unusedHist0'], CB, 'g_unusedEventHist0')]
                 + [(O['planeTable'] + 16 * i + 0xA,
                     c(CB, 'g_planeTable') + 16 * i + 0xA, 2)
                    for i in range(3)]}
             for ttl, mx, my in [(0, 0, 0), (5, 0x777, 0x888)]
             for ms, dt in [(0, 0), (2, 3)]]),

    # ---- computeThreatRangeBearing(tx,ty,alt,type,brgP,rngP)
    #      grid cell = (viewY>>11)*16 + (viewX>>11) = 0x12 for view 0x1000/0x800
    dict(fn='computeThreatRangeBearing', exe=EG, mod=CB, regs=['ax'],
         obs=[(SC2, SC2, 4)],
         vectors=[
             {'args': [tx, ty, alt, ty2, SC2, SC2 + 2], 'patch':
                 [P(O['viewX'], CB, 'g_viewX_', 0x1000),
                  P(O['viewY'], CB, 'g_viewY_', 0x0800),
                  P(O['viewZ'], CB, 'g_viewZ', vz),
                  P(O['ourHead'], CB, 'g_ourHead', hd),
                  P(O['missionStatus'], CB, 'g_missionStatus', ms),
                  P(O['startRange'], CB, 'g_startRange', 0x800),
                  P(O['threatScope'], CB, 'g_threatScopeRange', 0x20),
                  (O['mapCellFlags'] + cell, c(CB, 'g_mapCellFlags') + cell,
                   1, b8(fl)),
                  (O['samSpecs'] + 14 * ty2 + 8, c(CB, 'g_samSpecs')
                   + 14 * ty2 + 8, 6, w16(le) + w16(dgt) + w16(flg)),
                  (SC2, SC2, 4, 'ADDEADDE')]}
             for tx, ty, alt in [(0x800, 0x600, 0x300), (0x1400, 0x1000, 0x400),
                                 (0x3000, -0x800, -0x100)]
             for ty2 in [0, -1, 1, 3]
             for vz, hd, ms in [(0x300, 0, 0), (0x900, 0x2000, 2)]
             for cell, fl in [(0x12, 0xC), (0x12, 0)]
             for le, dgt, flg in [(0x600, 5, 1), (0x600, 5, 0),
                                  (0x60, 2, 1)]]),

    # ---- spawnSamThreat(): guards then a 24B projectile write + strBuf
    dict(fn='spawnSamThreat', exe=EG, mod=CB, regs=[],
         vectors=[
             {'args': [], 'patch':
                 [P(O['missionTick'], CB, 'g_missionTick', tick),
                  P(O['viewX'], CB, 'g_viewX_', 0x1000),
                  P(O['viewY'], CB, 'g_viewY_', 0x0800),
                  P(O['nightMode'], CB, 'g_nightMode', nm),
                  P(O['missionStatus'], CB, 'g_missionStatus', 1),
                  P(O['frate'], CB, 'g_frameRateScaling', 5),
                  P(O['samSpeed'], CB, 'g_samSpeed', 0x400),
                  P(O['samRange'], CB, 'g_samRange', 0x60),
                  (O['mapCellFlags'] + cell, c(CB, 'g_mapCellFlags') + cell,
                   1, b8(fl)),
                  (O['projTab'] + 24 * ((tick >> 4) & 7) + 0xE,
                   c(CB, 'g_projectiles') + 24 * ((tick >> 4) & 7) + 0xE,
                   2, w16(ttl)),
                  P(O['rngState'], CB, 'g_rngSeed', 0x99),
                  P(O['threatInit'], CB, 'g_threatTimerInit', 0x111),
                  P(O['viewZ'], CB, 'g_viewZ', 0x40),
                  P(O['ourHead'], CB, 'g_ourHead', 0x100)],
              'obs': [obsv(O['projTab'] + 24 * ((tick >> 4) & 7),
                           c(CB, 'g_projectiles') + 24 * ((tick >> 4) & 7), 24),
                      obsv(O['strBuf'], c(CB, 'strBuf'), 16)]}
             for tick in [0x10, 0x11, 0x50]
             for nm in [0, 1]
             for cell, fl in [(0x12, 0x10), (0x12, 0)]
             for ttl in [0, 5]]),

    # ---- buildVertexSignMask(): far stream ptr -> edge count + sign masks
    dict(fn='buildVertexSignMask', exe=EG, mod=DM, regs=[],
         obs=[OBS(O['edgeCount'], DM, 'g_modelEdgeCount'),
              OBS(O['wideVtx'], DM, 'g_modelWideVtxFlag'),
              OBS(O['maskLo'], DM, 'g_vtxSignMaskLo'),
              OBS(O['maskHi'], DM, 'g_vtxSignMaskHi'),
              obsv(O['modelStream'], c(DM, 'g_modelStreamPtr'), 4)],
         vectors=[
             {'args': [], 'patch':
                 [(O['modelStream'], c(DM, 'g_modelStreamPtr'), 2, w16(SC1)),
                  (O['modelStream'] + 2, c(DM, 'g_modelStreamPtr') + 2,
                   2, w16(DS)),
                  (SC1, SC1, 1 + 6 * 8,
                   (bytes([cnt & 0x1F]) + b''.join(
                       b'\x11\x22\x33\x44' + struct.pack('<h', w)
                       + b'\x55\x66' for w in edges)).hex())]}
             for cnt, edges in [(3, [-1, 0x100, -1]),
                                (5, [0, -1, -0x8000, 1, -1]),
                                (0x1F, [-1 if i % 2 else 1
                                        for i in range(31)]),
                                (0x11, [1] * 17)]]),

    # ---- computeAttitudeAngles(): orientMatrix -> ourPitch/Head/Roll
    dict(fn='computeAttitudeAngles', exe=EG, mod=FL, regs=[],
         obs=[OBS(O['aaPitch'], FL, 'g_ourPitch'),
              OBS(O['aaHead'], FL, 'g_ourHead'),
              OBS(O['aaRoll'], FL, 'g_ourRoll'),
              OBS(O['orientDirty'], FL, 'g_orientationDirty', 1)],
         vectors=[
             {'args': [], 'patch':
                 [(O['orientMat'], c(FL, 'g_orientMatrix'), 18, m.hex()),
                  (O['rollWasNz'], c(FL, 'g_rollWasNonzero'), 1, b8(rw)),
                  (O['orientDirty'], c(FL, 'g_orientationDirty'), 1, b8(0)),
                  # oracle BSS LUTs + cand BSS table -> identical contents
                  (O['sinLut'], None, 520, ANGLE_LUT),
                  (O['asinLut'], None, 520, ANGLE_LUT),
                  (None, c(FL, 'g_angleLut'), 520, ANGLE_LUT)]}
             for m, rw in [
                 # level: pitch=0 cos=0x7FFF; basic ratios, roll negated (m3<0,m4>0)
                 (struct.pack('<9h', 0, 0, 0x1000, -0x2000, 0x6000, 0, 0, 0, 0x7000), 0),
                 # head hi += 0x80 (m2<0,m8<0); roll = 0x8000-roll (m3>0,m4<0)
                 (struct.pack('<9h', 0, 0, -0x1000, 0x1000, -0x4000, 0, 0, 0, -0x5000), 0),
                 # head = -head (m2<0,m8>0); roll = -roll (m3<0,m4>0)
                 (struct.pack('<9h', 0, 0, -0x2000, -0x1000, 0x4000, 0, 0, 0, 0x5000), 0),
                 # m2==0 with m8<0 hits the <= test; m3==0,m4<0 likewise
                 (struct.pack('<9h', 0, 0, 0, 0, -0x3000, 0, 0, 0, -0x3000), 0),
                 # pitch ~+0x2000 (cos ~0x6EDA): |m2|,|m3| >= 0x5a81 -> else arm
                 (struct.pack('<9h', 0, 0, 0x5A90, 0x5A90, 0x2000, -0x4000, 0, 0, 0x3000), 0),
                 # m5=0x8000 -> pitch=0xC000 -> cosPitch=0 -> else path
                 # (head=valueToAngle(m1), roll=0); pitch in (0xbfff,0xc71d)
                 # sets orientDirty; m3<0,m4<0 -> head hi += 0x80
                 (struct.pack('<9h', 0, 0x1000, 0, -1, -1, -0x8000, 0, 0, 0), 0),
                 # pitch ~0x3EC4 in (0x38e3,0x4001) -> dirty; tiny cos -> zero nums
                 (struct.pack('<9h', 0, 0, 0, 0, 1, -0x7ff0, 0, 0, 1), 0),
                 # rollWasNonzero=1 and roll lands 0 -> dirty
                 (struct.pack('<9h', 0, 0, 0x1000, 0, 0x7000, 0, 0, 0, 0x7000), 1)]]),
]

emit(cases, os.path.join(os.path.dirname(os.path.abspath(__file__)),
                         'egame_state2.json'))
