#!/usr/bin/env python3
# Generates dosunit/egame_state.json - EGAME state-mutation / UI cluster.
#
# Same conventions as gen_egame_tac.py:
#   EN dseg offset = IDA flat - 0x2ECF0; patches are little-endian bytes.
#   Oracle and candidate cell offsets differ -> asymmetric (o_off, c_off).
#   obs entries compare per-side cell bytes after the call.
#   Scratch DS is zero-filled, so only nonzero inputs need patches.
import sys, os, struct
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))) + '/tools')
from duspec import emit, cand_off

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
EG = 'EGAME'

def c(mod, sym):
    v = cand_off(os.path.join(ROOT, 'build/%s.MAP' % mod), sym)
    assert v is not None, (mod, sym)
    return v

TC = 'EGTACMAP'; CB = 'EGCOMBAT'; FR = 'EGFRAME'
KY = 'EGKEYS';   UI = 'EGUI';     MA = 'EGMATH'; LD = 'EG3DLOAD'

def w16(v): return struct.pack('<H', v & 0xFFFF).hex()
def b8(v):  return struct.pack('<B', v & 0xFF).hex()

# EN oracle dseg cells (verified against image disasm)
O = {
    'viewMode': 0x94E0,   # byte
    'extCamDist': 0x5530, 'mapMode': 0x9676, 'zoomLvl': 0x584E,
    'radarRange': 0x5850, 'panelOn': 0x4EFA,
    'viewX': 0x94EE, 'viewY': 0x94FE, 'ourHead': 0x46DA,
    'planeFlags': 0x684C, # word; byte 0x684D gets |=0x20/0x40
    'tgtSlots': 0x8794,   # stride 18, +0 = state
    'wpIndex': 0x486A, 'strBuf': 0x65C6,
    'grndCnt': 0x9670, 'simObj': 0x8852, 'trkIdx': 0x551E,
    'gunHits': 0x4ECE, 'bombMask': 0x95BE, 'stores4': 0x4EDC,
    'gunAmmo': 0x4EEC, 'fuel': 0x4ED0,
    'frate': 0x4EFC, 'accelMode': 0x553A, 'syncWait': 0x5C68,
    'bulletTrk': 0x855E, 'threatInit': 0x86FC, 'dispTtl': 0x6EAA,
    'waypoints': 0x485A,  # word pairs, stride 4
    'stores': 0x80AA,     # stride 16: coordX+2 coordY+4
    'projDepth': 0x9642, 'vprojX': 0x10A8, 'vprojY': 0x128C,
    'rotMat': 0x8098,     # 3x3 int16
    'projTab': 0x53FC,    # stride 24: mapX+0 mapY+2 speed+6 worldX+8 ttl+0xE
    'acqAimY': 0x633A, 'acqRange': 0x6338,
    'detailLvl': 0x662C,  # byte
    'lodBlk': 0x9EC,      # 6 lod words + near 0x9F8 + far 0x9FA + 0x9FC
                        # (lodDistBase 0x9F4 / Scale 0x9F6 sit inside the
                        # block the loop just wrote on the oracle side)
}

def pp(mod, sym): return c(mod, sym)      # cand offset

def P(o, m, s, v, size=2):
    return (o, c(m, s), size, w16(v) if size == 2 else b8(v))

def OBS(o, m, s, size=2):
    return (o, c(m, s), size)

def obsv(o, coff, size): return (o, coff, size)

def sbytes(txt):                # NUL-terminated string literal for DS scratch
    return (txt + '\0').encode().hex()

DST, SRC = 0x7000, 0x7080       # scratch pointer-arg cells (same DS both sides)

cases = [
    # ---- zoomIn/zoomOut: viewMode&0x80 -> extCamDist; mapMode 0 -> zoom + a
    #      redrawTacMap call (will write to unmapped video -> typed fault);
    #      mapMode 1 -> radarScopeRange++.  Observed cells only.
    dict(fn='zoomIn', exe=EG, mod=TC, regs=[],
         obs=[OBS(O['zoomLvl'], TC, 'g_mapZoomLevel'),
              OBS(O['radarRange'], TC, 'g_radarScopeRange'),
              OBS(O['extCamDist'], TC, 'g_externalCamDist')],
         vectors=[
             {'args': [], 'patch':
                 [P(O['viewMode'], TC, 'g_viewMode', vm, 1),
                  P(O['mapMode'], TC, 'g_mapMode', mm),
                  P(O['zoomLvl'], TC, 'g_mapZoomLevel', zl),
                  P(O['radarRange'], TC, 'g_radarScopeRange', rr),
                  P(O['extCamDist'], TC, 'g_externalCamDist', ed)]}
             for vm in [0x00, 0x80]
             for mm, zl, rr, ed in [(0, 5, 0, 0x100), (1, 5, 3, 0x100),
                                    (0, 9, 0, 0x100), (0, 0, 0, 0x100),
                                    (2, 5, 1, 0x100)]]),
    dict(fn='zoomOut', exe=EG, mod=TC, regs=[],
         obs=[OBS(O['zoomLvl'], TC, 'g_mapZoomLevel'),
              OBS(O['radarRange'], TC, 'g_radarScopeRange'),
              OBS(O['extCamDist'], TC, 'g_externalCamDist')],
         vectors=[
             {'args': [], 'patch':
                 [P(O['viewMode'], TC, 'g_viewMode', vm, 1),
                  P(O['mapMode'], TC, 'g_mapMode', mm),
                  P(O['zoomLvl'], TC, 'g_mapZoomLevel', zl),
                  P(O['radarRange'], TC, 'g_radarScopeRange', rr),
                  P(O['extCamDist'], TC, 'g_externalCamDist', ed)]}
             for vm in [0x00, 0x80]
             for mm, zl, rr, ed in [(0, 5, 0, 0x100), (1, 5, 3, 0x100),
                                    (0, 2, 0, 0x100), (0, 9, 0, 0x100),
                                    (1, 5, 0, 0x100), (0, 5, 0, 0x8000)]]),

    # ---- updatePanelMode(mode): panelOn!=0 -> dispatch(mode) + g_mapMode=mode
    #      (mode 0/1 with panelOn leave the oracle's code window via
    #      redrawTacMap/drawPanelText -> not replayable; covered indirectly)
    dict(fn='updatePanelMode', exe=EG, mod=TC, regs=['ax'],
         obs=[OBS(O['mapMode'], TC, 'g_mapMode')],
         vectors=[
             {'args': [m], 'patch':
                 [P(O['panelOn'], TC, 'g_panelLabelOn', po_),
                  P(O['mapMode'], TC, 'g_mapMode', 0x7777)]}
             for m in [2, 7, -1]
             for po_ in [0, 1]] +
            [{'args': [m], 'patch':
                 [P(O['panelOn'], TC, 'g_panelLabelOn', 0),
                  P(O['mapMode'], TC, 'g_mapMode', 0x7777)]}
             for m in [0, 1]]),

    # ---- markTargetReached(n): mask bit -> 0; else waypointIndex writes,
    #      appendMapEvent + strcpy into strBuf; ret 1
    dict(fn='markTargetReached', exe=EG, mod=CB, regs=['ax'],
         obs=[OBS(O['wpIndex'], CB, 'waypointIndex'),
              OBS(O['planeFlags'], CB, 'g_playerPlaneFlags'),
              obsv(O['strBuf'], c(CB, 'strBuf'), 16)],
         vectors=[
             {'args': [n], 'patch':
                 [P(O['planeFlags'], CB, 'g_playerPlaneFlags', mk),
                  (O['tgtSlots'] + 18 * n, c(CB, 'g_targetSlots') + 18 * n,
                   2, w16(st))]}
             for n in [0, 1, 2, 5]
             for mk in [0, 0x4000, 0x6000, 0xFFFF]
             for st in [0, 3, 4, 5]]),

    # ---- resetSimObjectLocks: simObjects[i].terrainColor=-1 for i<grndCnt,
    #      trackedEnemyIdx=-1
    dict(fn='resetSimObjectLocks', exe=EG, mod=FR, regs=[],
         vectors=[
             {'args': [], 'patch':
                 [P(O['grndCnt'], FR, 'g_groundUnitCount', cnt)]
                 + [(O['simObj'] + 36 * i + 0x20,
                     c(FR, 'g_simObjects') + 36 * i + 0x20, 2, w16(0x5555 + i))
                    for i in range(5)],
              'obs': [OBS(O['trkIdx'], FR, 'g_trackedEnemyIdx')]
                 + [obsv(O['simObj'] + 36 * i + 0x20,
                         c(FR, 'g_simObjects') + 36 * i + 0x20, 2)
                    for i in range(5)]}
             for cnt in [0, 1, 3, 5]]),

    # ---- initWeaponLoadout: all-writes leaf
    dict(fn='initWeaponLoadout', exe=EG, mod=FR, regs=[],
         obs=[OBS(O['gunHits'], FR, 'g_gunHits'),
              OBS(O['bombMask'], FR, 'g_bombDamageMask'),
              obsv(O['stores4'], c(FR, 'g_stores'), 16),
              OBS(O['gunAmmo'], FR, 'g_gunAmmo'),
              OBS(O['fuel'], FR, 'g_fuelRemaining')],
         vectors=[{'args': [], 'patch': []}]),

    # ---- recalcTimeScale: scales frameRateScaling, writes 5 cells
    dict(fn='recalcTimeScale', exe=EG, mod=KY, regs=[],
         obs=[OBS(O['syncWait'], KY, 'g_frameSyncWait'),
              OBS(O['frate'], KY, 'g_frameRateScaling'),
              OBS(O['bulletTrk'], KY, 'g_bulletTrackCount'),
              OBS(O['threatInit'], KY, 'g_threatTimerInit'),
              OBS(O['dispTtl'], KY, 'g_threatDisplayTtl')],
         vectors=[
             {'args': [], 'patch':
                 [P(O['frate'], KY, 'g_frameRateScaling', fr),
                  P(O['accelMode'], KY, 'g_timeAccelMode', am)]}
             for fr in [0, 1, 4, 15, 16, 30, 120, 0x7FFF]
             for am in [0, 1, 2]]),

    # ---- copyStoreToWaypoint(dst, src): wp[2d]=stores[s].coordX/Y
    dict(fn='copyStoreToWaypoint', exe=EG, mod=KY, regs=[],
         obs=[(O['waypoints'], c(KY, 'waypoints'), 16)],
         vectors=[
             {'args': [d, s], 'patch': [
                 (O['stores'] + 16 * s + 2, c(KY, 'g_storeDefs') + 16 * s + 2,
                  4, w16(x) + w16(y))]}
             for d in [0, 1, 3]
             for s in [0, 2, 7]
             for x, y in [(0x111, 0x222), (-1, 0x8000)]]),

    # ---- projectMapPoint(x,y): writes projDepth/vprojX/vprojY
    dict(fn='projectMapPoint', exe=EG, mod=UI, regs=[],
         obs=[OBS(O['vprojX'], UI, 'g_vprojX'),
              OBS(O['vprojY'], UI, 'g_vprojY'),
              OBS(O['projDepth'], UI, 'g_projDepth')],
         vectors=[
             {'args': [mx, my], 'patch':
                 [P(O['radarRange'], UI, 'g_radarScopeRange', rr, 1),
                  P(O['viewX'], UI, 'g_viewX_', vx),
                  P(O['viewY'], UI, 'g_viewY_', vy),
                  P(O['ourHead'], UI, 'g_ourHead', hd)]}
             for mx, my in [(0, 0), (0x400, 0x1400), (-0x800, 0x2000),
                            (0x7FFF, -0x7FFF)]
             for rr in [0, 3, 7]
             for vx, vy, hd in [(0, 0, 0), (0x1000, -0x800, 0x4000)]]),

    # ---- matVecDotAxis(i,a,b,c): dx:ax = sum fixedMul(mat[row][i], v)
    dict(fn='matVecDotAxis', exe=EG, mod=MA, regs=['ax', 'dx'],
         vectors=[
             {'args': [i, a, b, cc], 'patch': [
                 (O['rotMat'], c(MA, 'g_rotMatrix'), 18, m.hex())]}
             for i in [0, 1, 2]
             for a, b, cc in [(0, 0, 0), (0x1000, 0x2000, 0x3000),
                              (-1, 0x7FFF, -0x8000)]
             for m in [
                 bytes(18),                                      # zero
                 struct.pack('<9h', 0x4000, 0, 0, 0, 0x4000, 0, 0, 0, 0x4000),
                 struct.pack('<9h', 1, -2, 3, -4, 5, -6, 7, -8, 0x7FFF)]]),

    # ---- samCanAcquireTarget(slot,tx,ty,alt,mode): projectiles[slot] fields
    dict(fn='samCanAcquireTarget', exe=EG, mod=CB, regs=['ax'],
         obs=[OBS(O['acqAimY'], CB, 'g_acqAimY'),
              OBS(O['acqRange'], CB, 'g_acqRange')],
         vectors=[
             {'args': [slot, tx, ty, alt, mode], 'patch': [
                 (O['projTab'] + 24 * slot, c(CB, 'g_projectiles') + 24 * slot,
                  16, w16(px) + w16(py) + w16(pa) + w16(sp)),
                 (O['projTab'] + 24 * slot + 8, c(CB, 'g_projectiles')
                  + 24 * slot + 8, 2, w16(wx)),
                 P(O['ourHead'], CB, 'g_ourHead', hd)]}
             for slot in [0, 3, 7, 9]
             for tx, ty, alt, mode in [(0x100, 0x80, 0, 0),
                                       (0x1000, 0x1000, 0, 0),
                                       (0x4000, 0, 0, 3),
                                       (-0x1000, 0x800, 100, 1)]
             for px, py, pa, sp, wx, hd in
                 [(0, 0, 0, 0, 0, 0), (0x40, -0x40, 10, 0x200, 0x800, 0x800)]]),

    # ---- strcpyFromDot(dst, src): scan dst for '.', strcpy(dst_at_dot, src)
    dict(fn='strcpyFromDot', exe=EG, mod=LD, regs=[],
         obs=[(DST, DST, 16)],
         vectors=[
             {'args': [DST, SRC], 'patch':
                 [(DST, DST, 16, (dt + '\0').encode().hex().ljust(32, '0')),
                  (SRC, SRC, 8, sbytes(st))]}
             for dt in ["A.OLD", "NOEXT", ".X", ""]
             for st in [".3D3", "X", ".1234567", ""]]),

    # ---- formatTwoDigit(v): val%=60 -> optional '0' + itoa -> strBuf append
    dict(fn='formatTwoDigit', exe=EG, mod=UI, regs=[],
         obs=[obsv(O['strBuf'], c(UI, 'g_nameBuf'), 10)],
         vectors=[
             {'args': [v], 'patch': [
                 (O['strBuf'], c(UI, 'g_nameBuf'), 4, pre)]}
             for v in [0, 1, 9, 10, 59, 60, 61, 119, 120, -1, 3599]
             for pre in ['00000000', '41410000']]),   # "" and "AA\0"

    # ---- setupLodDistances: 6 lod words + near/far from detailLevel.
    #      near/far live INSIDE the written block (lodTab[6]/[7]); the
    #      18-byte window covers everything including the +0x20 write.
    dict(fn='setupLodDistances', exe=EG, mod=FR, regs=[],
         obs=[obsv(O['lodBlk'], c(FR, 'colorLut') + 0x10, 18)],
         vectors=[
             {'args': [], 'patch': [
                 P(O['detailLvl'], FR, 'g_detailLevel', d, 1)]}
             for d in [0, 1, 3, 5, 9, 0x7F, 0xFF]]),
]

emit(cases, os.path.join(os.path.dirname(os.path.abspath(__file__)),
                         'egame_state.json'))
