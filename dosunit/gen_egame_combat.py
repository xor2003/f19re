#!/usr/bin/env python3
# Generates dosunit/egame_combat.json - EGAME combat/frame/UI state cluster.
#
# Cell offsets are EN-image DS offsets (what the oracle binary reads/writes),
# verified against image disassembly.  Oracle and candidate cell offsets
# differ -> asymmetric (o_off, c_off) patch/observe tuples.
#
# ABI limitation: routines reaching makeSound (0xDD7F) end in `call far
# 1ECF:<slot>` whose slots are `EA 00 00 00 00` (jmp far 0:0) - the driver
# is not loaded in the replay environment and code-range/memory rules make
# the slots unpatchable.  Such routines are oracle-untestable, so this
# batch only covers paths that never reach a driver call:
#   * destroySimObject / bombTarget: unconditional makeSound -> dropped.
#   * countermeasures: exhausted-timer path only (jumps to epilogue).
#   * updateBulletsAndFire: non-firing paths only (makeSound is inside the
#     fire branch).
#   * tickWeaponSlots: drawStatusItem early-rets when statusGate == 0
#     (gate == 1 would reach the draw ABI call).
import sys, os, struct, re
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))) + '/tools')
from duspec import emit, cand_off, rand_seed_cell, arg_lit_off

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
EG = 'EGAME'
FR = 'EGFRAME'; CB = 'EGCOMBAT'; TG = 'EGTARGET'; UI = 'EGUI'

def c(mod, sym):
    v = cand_off(os.path.join(ROOT, 'build/%s.MAP' % mod), sym)
    assert v is not None, (mod, sym)
    return v

def w16(v): return struct.pack('<H', v & 0xFFFF).hex()
def b8(v):  return struct.pack('<B', v & 0xFF).hex()
def s8(t):  return (t.encode() + b'\0').hex()

# --- EN oracle dseg cells (verified against EGAME.EXE image disasm) ---
O = {
    # countermeasures / fire+weapon table (aliased views of 0x520A)
    'eventTimers': 0x4ED2,      # int16[4] indexed by type
    'fireRecs':    0x520A,      # stride 0xC: viewX+0 viewY+2 type+6 timer+8
    'wpnSlots':    0x5210,      # tickWeaponSlots reads state@+6 timer@+8
    'weaponMask':  0x4ECE,      # byte test 0x40 (also bombDamageMask)
    'missionStatus': 0x4EF0, 'frate': 0x4EFC, 'ejectState': 0x944A,
    'viewX': 0x94EE, 'viewY': 0x94FE, 'viewZ': 0x46E0,
    'ourHead': 0x46DA, 'ourPitch': 0x46DC,
    'nameBuf': 0x65C6,
    'hudMsgBuf': 0x956E, 'hudMsgTtl': 0x5856,
    'soundFloor': 0x5C72,       # makeSound priority gate word_34962
    'statusGate': 0x4EFA,       # drawStatusItem enable flag
    'statusTab': 0x5684,        # stride 0xA; last-drawn value at +8
    'litZapas': 0x553E, 'litFlare': 0x554F, 'litChaff': 0x5555,
    'litKran': 0x555B, 'litWyp': 0x5561,
    # updateBulletsAndFire
    'bullets': 0x9B88,          # BulletTrack stride 0xC
    'bulletCnt': 0x855E,
    'frameTick': 0x5520,
    'gunAmmo': 0x4EEC,
    'gunFiredFlag': 0x6636,
    'inputDisabled': 0x4EF6,    # readAxisInput gate
    'axisReads': 0x5C6E,        # +2*axis
    'commData': 0x9C82,         # far ptr (off/seg)
    'sinLut': 0x3776, 'asinLut': 0x3978,
    # markTargetReached (0x7974)
    'targetSlots': 0x8794,      # stride 0x12: state@+0
    'playerFlags': 0x684C,      # high byte at 0x684D
    'waypointIndex': 0x486A,
    'strBuf': 0x65C6,
    # recordFrame (0x4B40) - called via appendMapEvent
    'recCount': 0x632E, 'replayLog': 0x8E46, 'missionTick': 0x6630,
    # commitCommSnapshot
    'commEvtFlag': 0x9E8A,      # byte
    'gunHits': 0x95BE,
    # moveStuff / initStoreData
    'flagFtN': 0x963C, 'farPtr': 0x6330,     # stream cursor off/seg
    'landTgt': 0x9652, 'waterTgt': 0x94F2, 'planeCnt': 0x9500,
    'tgtEntCnt': 0x6646, 'planeScan': 0x9C78,
    'planeTab': 0x80AA,                      # MapTarget stride 0x10
    'gndUnitCnt': 0x9670, 'shapeCat': 0x95D2, 'killTally': 0x9506,
    'simObjects': 0x8852,       # moveStuff ground-object stream target
    'strPool': 0x9746, 'mapCellFl': 0x85FC, 'unusedSaved': 0x9638,
    'padlockAc': 0x552E, 'waypoints': 0x485A,
    'nameTab': 0x9678, 'selStoreIdx': 0x8798,
    'worldX': 0x8E2A, 'worldY': 0x9446,
    # randomRange
    'rngSeed': 0x622C,          # 32-bit LCG state
    # loadColorPalette
    'palDst': 0x9DC, 'palSrc': 0x3F0,
    # buildRangeString
    'litRange': 0x5C05, 'litDot': 0x5C0C, 'litKm': 0x5C0E,
}

DS = 0x6000
SC = 0xA000          # comm stream image inside scratch DS — must sit above
                     # every stream destination (oracle strPool ends 0x9A34)
                     # or transfers overwrite their own pending input bytes

# stubs.c's const g_angleLut (Q15 sine, 260 entries) patched into the
# oracle's BSS LUTs so sinMul/cosMul see identical contents.
def _angle_lut_hex():
    src = open(os.path.join(ROOT, 'src/stubs.c')).read()
    body = src.split('g_angleLut[260] = {', 1)[1].split('};', 1)[0]
    vals = [int(t, 16) for t in re.findall(r'0x[0-9a-fA-F]+', body)]
    return struct.pack('<%dH' % len(vals), *vals).hex()

ANGLE_LUT = _angle_lut_hex()

def P(o, m, s, v, size=2):
    return (o, c(m, s), size, w16(v) if size == 2 else b8(v))
def OBS(o, m, s, size=2):
    return (o, c(m, s), size)
def obsv(o, coff, size): return (o, coff, size)
def OFFOBS(off, size): return {'off': hex(off), 'size': size}

def stream_bytes(np_, ng, sel=None):
    """Comm-buffer image for moveStuff: one record per transfer, distinct
    bytes so swapped/misrouted copies show as diffs.  The count fields
    (planeCnt at pos 2, gndUnitCnt after the plane table) must carry the
    real counts — they size the following transfers, and a 16-bit shl-4
    truncates large values into absurd copies that clobber later cells.
    sel embeds a value at targetSlots+4 — in EN that cell is
    word_37626 (g_selStoreIdx), which the last transfer overwrites, so
    initStoreData's post-moveStuff storeDef lookup reads it."""
    lens = [1, 1, 2, 2, 2, 16 * np_, 2, 36 * ng, 0x64, 0x64,
            0x2EE, 0x100, 2, 2, 0x10, 0x24]
    buf = bytearray(); pos = 0
    for ln in lens:
        for i in range(ln):
            buf.append((0x40 + pos) & 0xFF); pos += 1
    buf[2:4] = np_.to_bytes(2, 'little')
    buf[8 + 16 * np_:8 + 16 * np_ + 2] = ng.to_bytes(2, 'little')
    if sel is not None:
        ts = 10 + 16 * np_ + 36 * ng + 0x64 + 0x64 + 0x2EE + 0x100 + 2 + 2 + 0x10
        buf[ts + 4:ts + 6] = sel.to_bytes(2, 'little')
    return bytes(buf)

def _pool_nuls(np_, ng):
    """NUL bytes the initStoreData scan finds inside the streamed strPool:
    the pool sits after transfers 1-10 of the comm image."""
    pool_start = 10 + 16 * np_ + 36 * ng + 0xC8
    return sum(1 for i in range(0x2EE)
               if (0x40 + pool_start + i) & 0xFF == 0)

def stream_patch(mod, np_, ng, base=SC, init_farptr=True, sel=None):
    """DS image of the comm stream + cursor pointing at it."""
    out = []
    if init_farptr:
        out.append((O['farPtr'], c(mod, 'farPointer'), 4,
                    w16(base) + w16(DS)))
    data = stream_bytes(np_, ng, sel)
    out.append((base, base, len(data), data.hex()))
    return out

def _storedef_xy(np_, sel):
    """coordX/coordY the oracle's initStoreData will read for index sel:
    g_storeDefs aliases the streamed planeTab, so a streamed entry
    (sel < np) yields pattern bytes; anything beyond stays whatever the
    caller patched, so just pick fixed probe values there."""
    if sel < np_:
        d = stream_bytes(np_, 0)
        return (d[8 + 16 * sel + 2] | d[8 + 16 * sel + 3] << 8,
                d[8 + 16 * sel + 4] | d[8 + 16 * sel + 5] << 8)
    return 0x1234, 0x5678

def ds_src_patches(mod, np_, ng):
    """flagFtN==0 direction: fill every DS source cell with the same
    position-coded pattern the stream would hold."""
    lens = [('landTgt', 'g_landTargetId', 1),
            ('waterTgt', 'g_waterTargetId', 1),
            ('planeCnt', 'g_planeCount', 2),
            ('tgtEntCnt', 'g_targetEntityCount', 2),
            ('planeScan', 'g_planeScanCount', 2),
            ('planeTab', 'g_planeTable', 16 * np_),
            ('gndUnitCnt', 'g_groundUnitCount', 2),
            ('simObjects', 'g_simObjects', 36 * ng),
            ('shapeCat', 'g_shapeTargetCategory', 0x64),
            ('killTally', 'g_tileKillTally', 0x64),
            ('strPool', 'g_stringPool', 0x2EE),
            ('mapCellFl', 'g_mapCellFlags', 0x100),
            ('unusedSaved', 'g_unusedSavedWord', 2),
            ('padlockAc', 'g_padlockAircraft', 2),
            ('waypoints', 'waypoints', 0x10),
            ('targetSlots', 'g_targetSlots', 0x24)]
    pos = 0; out = []
    for ok, cs, ln in lens:
        if ln == 0:
            continue
        # planeCnt / gndUnitCnt must carry the real counts: they size later
        # transfers, so the pattern can't be arbitrary there.
        if ok == 'planeCnt':  data = w16(np_)
        elif ok == 'gndUnitCnt': data = w16(ng)
        else:
            data = bytes((0x40 + pos + i) & 0xFF for i in range(ln))
        if isinstance(data, str): hx = data
        else: hx = data.hex()
        out.append((O[ok], c(mod, cs), ln, hx))
        pos += ln
    return out

cases = [
    # ---- loadColorPalette(idx): memcpy(colorLut, palettes + idx*16, 16)
    dict(fn='loadColorPalette', exe=EG, mod=UI, regs=[],
         obs=[obsv(O['palDst'], c(UI, 'colorLut'), 16)],
         vectors=[
             {'args': [idx], 'patch':
                 [(O['palSrc'] + 16 * i, c(UI, 'g_colorPalettes') + 16 * i, 16,
                   bytes((i * 16 + j) & 0xFF for j in range(16)).hex())
                  for i in range(8)]}
             for idx in [0, 1, 3, 7]]),

    # ---- tickWeaponSlots(): first nonzero-timer slot only; state==3 calls
    #      drawStatusItem(7, timer ? 0xA : 0).  statusGate==0 makes
    #      drawStatusItem early-ret (gate==1 would reach the draw ABI).
    dict(fn='tickWeaponSlots', exe=EG, mod=FR, regs=[],
         vectors=[
             {'args': [], 'patch':
                 [P(O['statusGate'], FR, 'g_panelLabelOn', 0)]
                 + [(O['wpnSlots'] + 12 * i, c(FR, 'g_wpnSlots') + 12 * i,
                     4, w16(st) + w16(tm)) for i, (st, tm) in enumerate(slots)],
              'obs': [obsv(O['wpnSlots'], c(FR, 'g_wpnSlots'), 48),
                      obsv(O['statusTab'], c(FR, 'g_statCells'), 0x50)]}
             for slots in [[(0, 0)] * 4,
                           [(3, 5), (0, 0), (0, 0), (0, 0)],
                           [(1, 9), (3, 1), (3, 2), (2, 7)],
                           [(3, 0), (3, 1), (0, 0), (3, 4)]]]),

    # ---- countermeasures(type): exhausted-timer path only - all spawn
    #      paths end in makeSound (untestable).  The exhausted path does
    #      eventTimers[type]-- (old <= 0 -> reset to 0) + hudMessage(zapas).
    dict(fn='countermeasures', exe=EG, mod=FR, regs=[],
         vectors=[
             {'args': [typ], 'patch':
                 [(O['eventTimers'], c(FR, 'g_eventTimers'), 8,
                   ''.join(w16(et if i == typ else 7)
                           for i in range(4))),
                  P(O['frate'], FR, 'g_frameRateScaling', 4),
                  P(O['weaponMask'], FR, 'g_weaponMask', wm),
                  P(O['missionStatus'], FR, 'g_missionStatus', ms),
                  P(O['soundFloor'], FR, 'g_soundPriorityFloor', 0x7FFF),
                  # oracle carries English literals, cand Russian - patch
                  # both sides to identical bytes; cand literal offset
                  # moves across relinks, so derive it from the hudMessage
                  # call's push
                  (O['litZapas'], arg_lit_off(FR, 'countermeasures',
                                              'hudMessage'), 16,
                   s8('Stores empty')),
                  (O['nameBuf'], c(FR, 'g_nameBuf'), 4, '00000000'),
                  (O['fireRecs'], c(FR, 'g_fireRecs'), 48, '5a' * 48)],
              'obs': [obsv(O['eventTimers'], c(FR, 'g_eventTimers'), 8),
                      obsv(O['hudMsgBuf'], c(FR, 'g_hudMessageBuf'), 24),
                      OBS(O['hudMsgTtl'], FR, 'g_hudMsgTimer'),
                      obsv(O['nameBuf'], c(FR, 'g_nameBuf'), 8),
                      obsv(O['fireRecs'], c(FR, 'g_fireRecs'), 48)]}
             for typ in [0, 1, 2, 3]
             for et in [0, -3]
             for wm, ms in [(0, 0), (0x40, 3)]]),

    # ---- updateBulletsAndFire(): track integration on every call;
    #      fire branch (odd tick + axis + ammo + !eject) ends in
    #      makeSound -> vectors keep at least one gate closed.
    dict(fn='updateBulletsAndFire', exe=EG, mod=FR, regs=[],
         vectors=[
             {'args': [], 'patch':
                 [P(O['bulletCnt'], FR, 'g_bulletTrackCount', cnt),
                  P(O['frameTick'], FR, 'frameTick', tick),
                  P(O['inputDisabled'], FR, 'g_inputDisabled', idis),
                  P(O['gunAmmo'], FR, 'g_gunAmmo', ammo),
                  P(O['ejectState'], FR, 'g_ejectState', ej),
                  P(O['frate'], FR, 'g_frameRateScaling', 4),
                  P(O['ourHead'], FR, 'g_ourHead', 0x1000),
                  P(O['ourPitch'], FR, 'g_ourPitch', 0x0400),
                  P(O['viewX'], FR, 'g_viewX_', 0x800),
                  P(O['viewY'], FR, 'g_viewY_', 0x400),
                  P(O['viewZ'], FR, 'g_viewZ', 0x40),
                  P(O['soundFloor'], FR, 'g_soundPriorityFloor', 0x7FFF),
                  # readAxisInput: commData->setupUseJoy == 0 -> table read
                  (O['commData'], c(FR, 'commData'), 4, w16(0xB000) + w16(DS)),
                  (0xB072, 0xB072, 2, w16(0)),
                  (O['axisReads'], c(FR, 'g_axisInputAccum'), 8,
                   w16(axv) + w16(0x20) + w16(0x30) + w16(0x40)),
                  (O['sinLut'], None, 520, ANGLE_LUT),
                  (O['asinLut'], None, 520, ANGLE_LUT),
                  (None, c(FR, 'g_angleLut'), 520, ANGLE_LUT)]
                 + [(O['bullets'] + 12 * i, c(FR, 'bulletTracks') + 12 * i, 12,
                     struct.pack('<6h', px, 0x10 + i, 0x20 + i,
                                 1 + i, -1 - i, 2 + i).hex())
                    for i, px in enumerate(pxs)],
              'obs': [obsv(O['bullets'], c(FR, 'bulletTracks'),
                           12 * (cnt + 4)),
                      OBS(O['gunAmmo'], FR, 'g_gunAmmo'),
                      OBS(O['gunFiredFlag'], FR, 'g_gunFiredFlag')]}
             for cnt in [1, 4]
             for tick in [4, 5]
             for idis, axv in [(1, 0), (0, 0), (0, 1)]
             for ammo, ej in [(0, 0), (500, 0), (500, 2)]
             for pxs in [[0] * 8, [100, 0, 200, 0, 0, 0, 0, 0]]
             if not (tick & 1 and not idis and axv and ammo > 0 and not ej)]),

    # ---- markTargetReached(n): done-mask check -> appendMapEvent ->
    #      strBuf msg + waypointIndex + playerPlaneFlags bits
    dict(fn='markTargetReached', exe=EG, mod=CB, regs=['ax'],
         vectors=[
             {'args': [n], 'patch':
                 [P(O['playerFlags'], CB, 'g_playerPlaneFlags', pf),
                  (O['targetSlots'] + 0x12 * n,
                   c(CB, 'g_targetSlots') + 0x12 * n, 2, w16(st)),
                  (O['strBuf'], c(CB, 'strBuf'), 16, 'ee' * 16),
                  P(O['recCount'], CB, 'g_replayCount', rc),
                  P(O['missionTick'], CB, 'g_missionTick', 0x55),
                  P(O['viewX'], CB, 'g_viewX_', 0x4020),
                  P(O['viewY'], CB, 'g_viewY_', 0x2091)],
              'obs': [OBS(O['playerFlags'], CB, 'g_playerPlaneFlags'),
                      OBS(O['waypointIndex'], CB, 'waypointIndex'),
                      obsv(O['strBuf'], c(CB, 'strBuf'), 20),
                      OBS(O['recCount'], CB, 'g_replayCount'),
                      obsv(O['replayLog'] + 6 * rc,
                           c(CB, 'g_replayLog') + 6 * rc, 12),
                      (O['targetSlots'] + 0x12 * n,
                       c(CB, 'g_targetSlots') + 0x12 * n, 2)]}
             for n in [0, 1]
             for pf in [0, 0x4000 >> n, 0x6000 & ~(0x4000 >> n)]
             for st in [0, 3, 4]
             for rc in [2]]
         + [
             # replay-log full: appendMapEvent no-ops, rest still runs
             {'args': [0], 'patch':
                 [P(O['playerFlags'], CB, 'g_playerPlaneFlags', 0),
                  (O['targetSlots'], c(CB, 'g_targetSlots'), 2, w16(3)),
                  (O['strBuf'], c(CB, 'strBuf'), 16, 'ee' * 16),
                  P(O['recCount'], CB, 'g_replayCount', 0xFF),
                  P(O['missionTick'], CB, 'g_missionTick', 0x55),
                  P(O['viewX'], CB, 'g_viewX_', 0x4020),
                  P(O['viewY'], CB, 'g_viewY_', 0x2091)],
              'obs': [OBS(O['playerFlags'], CB, 'g_playerPlaneFlags'),
                      OBS(O['waypointIndex'], CB, 'waypointIndex'),
                      obsv(O['strBuf'], c(CB, 'strBuf'), 20),
                      OBS(O['recCount'], CB, 'g_replayCount')]}]),

    # ---- commitCommSnapshot(arg): comm snapshot + recordFrame(8,0).
    #      Early-ret iff ejectState != 0 && arg != 0.
    dict(fn='commitCommSnapshot', exe=EG, mod=FR, regs=[],
         vectors=[
             {'args': [arg], 'patch':
                 [P(O['ejectState'], FR, 'g_ejectState', ej),
                  (O['commData'], c(FR, 'commData'), 4, w16(0xB000) + w16(DS)),
                  (0xB000, 0xB000, 0x80, '00' * 0x80),
                  (O['commEvtFlag'], c(FR, 'g_commEventFlag'), 1, 'ab'),
                  P(O['viewX'], FR, 'g_viewX_', 0x1357),
                  P(O['viewY'], FR, 'g_viewY_', 0x2468),
                  P(O['weaponMask'], FR, 'g_bombDamageMask', 0x5A5A),
                  P(O['gunHits'], FR, 'g_gunHits', 0x31),
                  P(O['recCount'], FR, 'g_replayCount', rc),
                  P(O['missionTick'], FR, 'g_missionTick', 0x77)],
              'obs': [OFFOBS(0xB000, 0x80),
                      (O['commEvtFlag'], c(FR, 'g_commEventFlag'), 1),
                      OBS(O['recCount'], FR, 'g_replayCount'),
                      obsv(O['replayLog'] + 6 * rc,
                           c(FR, 'g_replayLog') + 6 * rc, 12)]}
             for arg in [0, 1]
             for ej in [0, 2]
             for rc in [0, 0xFE]]),

    # ---- moveStuff(): 16 sequential far->near (flag=1) or near->far
    #      (flag=0) stream transfers via moveNearFar
    dict(fn='moveStuff', exe=EG, mod=FR, regs=[],
         vectors=[
             {'args': [], 'patch':
                 [P(O['flagFtN'], FR, 'flagFarToNear', 1)]
                 + stream_patch(FR, np_, ng),
              'obs': ([obsv(O['landTgt'], c(FR, 'g_landTargetId'), 1),
                       obsv(O['waterTgt'], c(FR, 'g_waterTargetId'), 1),
                       OBS(O['planeCnt'], FR, 'g_planeCount'),
                       OBS(O['tgtEntCnt'], FR, 'g_targetEntityCount'),
                       OBS(O['planeScan'], FR, 'g_planeScanCount')]
                      + ([obsv(O['planeTab'], c(FR, 'g_planeTable'),
                               16 * np_)] if np_ else [])
                      + [OBS(O['gndUnitCnt'], FR, 'g_groundUnitCount')]
                      + ([obsv(O['simObjects'], c(FR, 'g_simObjects'),
                               36 * ng)] if ng else [])
                      + [
                      obsv(O['strPool'], c(FR, 'g_stringPool'), 0x2EE),
                      obsv(O['shapeCat'], c(FR, 'g_shapeTargetCategory'), 0x64),
                      obsv(O['killTally'], c(FR, 'g_tileKillTally'), 0x64),
                      obsv(O['mapCellFl'], c(FR, 'g_mapCellFlags'), 0x100),
                      OBS(O['unusedSaved'], FR, 'g_unusedSavedWord'),
                      OBS(O['padlockAc'], FR, 'g_padlockAircraft'),
                      obsv(O['waypoints'], c(FR, 'waypoints'), 0x10),
                      obsv(O['targetSlots'], c(FR, 'g_targetSlots'), 0x24),
                      obsv(O['farPtr'], c(FR, 'farPointer'), 4)])}
             for np_, ng in [(0, 0), (1, 1), (2, 0), (3, 2), (4, 3)]]
         + [
             {'args': [], 'patch':
                 [P(O['flagFtN'], FR, 'flagFarToNear', 0)]
                 + ds_src_patches(FR, np_, ng)
                 + stream_patch(FR, np_, ng)
                 + [(SC, SC, len(stream_bytes(np_, ng)),
                     '00' * len(stream_bytes(np_, ng)))],
              'obs': [OFFOBS(SC, len(stream_bytes(np_, ng))),
                      obsv(O['farPtr'], c(FR, 'farPointer'), 4)]}
             for np_, ng in [(0, 0), (1, 1), (2, 0), (3, 2)]]),

    # ---- initStoreData(): cursor init + moveStuff + nameTab scan +
    #      worldX/worldY from storeDefs[g_selStoreIdx].  nameTab entries are
    #      side-specific near pointers into each side's string pool, so only
    #      the unwritten 0xEE tail is observable; its start index proves the
    #      NUL-scan entry count agrees.  worldX/Y read storeDef words that
    #      sit inside the streamed planeTab bytes (same content both sides).
    dict(fn='initStoreData', exe=EG, mod=FR, regs=[],
         vectors=[
             {'args': [], 'patch':
                 # commData = DS:SC-0x7A so setCommWorldbufPtr leaves the
                 # cursor at SC where the stream image sits
                 [(O['commData'], c(FR, 'commData'), 4,
                   w16(SC - 0x7A) + w16(DS)),
                  P(O['selStoreIdx'], FR, 'g_selStoreIdx', sel),
                  # EN's g_storeDefs aliases g_planeTable: the oracle reads
                  # coordX/Y out of the just-streamed table, the cand out of
                  # its own array — patch it to the bytes the oracle sees.
                  (O['planeTab'] + 0x10 * sel + 2,
                   c(FR, 'g_storeDefs') + 0x10 * sel + 2, 4,
                   w16(_storedef_xy(np_, sel)[0])
                   + w16(_storedef_xy(np_, sel)[1])),
                  (O['nameTab'], c(FR, 'g_nameTab'), 0xC8, 'ee' * 0xC8)]
                 + stream_patch(FR, np_, ng, init_farptr=False, sel=sel),
              'obs': [obsv(O['farPtr'], c(FR, 'farPointer'), 4),
                      obsv(O['nameTab'] + 2 * nent,
                           c(FR, 'g_nameTab') + 2 * nent, 0xC8 - 2 * nent),
                      obsv(O['worldX'], c(FR, 'g_worldX'), 4),
                      obsv(O['worldY'], c(FR, 'g_worldY'), 4)]}
             for np_, ng, sel in [(1, 1, 0), (2, 2, 1), (0, 0, 0)]
             for nent in [1 + _pool_nuls(np_, ng)]]),

    # ---- buildRangeString(rangeRaw): "Range " + itoa(>>6) + "." +
    #      itoa((raw&0x3F)*2/13) + " km"
    dict(fn='buildRangeString', exe=EG, mod=TG, regs=[],
         patch=[(O['litRange'],
                 arg_lit_off(TG, 'buildRangeString', 'strcpy', 0, 1),
                 7, s8('Range ')),
                (O['litDot'],
                 arg_lit_off(TG, 'buildRangeString', 'strcat', 0, 1),
                 2, s8('.')),
                (O['litKm'],
                 arg_lit_off(TG, 'buildRangeString', 'strcat', 1, 1),
                 4, s8(' km'))],
         obs=[obsv(O['strBuf'], c(TG, 'strBuf'), 24)],
         vectors=[
             {'args': [r]} for r in [0, 1, 0x3F, 0x40, 0x7FF, 0x1234,
                                     0x8000, 0xFFFF]]),

    # ---- pure helpers pulled in by the combat cluster -------------
    # randomRange(n): seed32 = seed32*0x343FD + 0x269EC3; ret (n*rand)>>15.
    # The candidate's _rand is the MSC CRT routine whose 32-bit state has
    # no public symbol and moves with DGROUP layout - derive per module.
    dict(fn='randomRange', exe=EG, mod=FR, regs=['ax'],
         vectors=[
             {'args': [n], 'patch':
                 [(O['rngSeed'], rand_seed_cell(FR), 4,
                   w16(seed & 0xFFFF) + w16(seed >> 16))],
              'obs': [obsv(O['rngSeed'], rand_seed_cell(FR), 4)]}
             for seed in [1, 0xACE1, 0x12345678]
             for n in [0, 1, 7, 0x4000, -1]]),

    # sinMul(a,v) = (v * sinLerp(a)) >> 15;  LUT at 0x3776 (BSS in oracle)
    dict(fn='sinMul', exe=EG, mod=FR, oof=0xd2cc, regs=['ax'],
         patch=[(O['sinLut'], None, 520, ANGLE_LUT),
                (O['asinLut'], None, 520, ANGLE_LUT),
                (None, c(FR, 'g_angleLut'), 520, ANGLE_LUT)],
         vectors=[
             {'args': [a, v]}
             for a in [0, 0x800, 0x4000, 0xC000, 0xFFFF]
             for v in [0, 0x1234, 0x7FFF, -1]]),

    # cosMul(a,v) = sinMul(a + 0x4000, v)
    dict(fn='cosMul', exe=EG, mod=FR, oof=0xd2e4, regs=['ax'],
         patch=[(O['sinLut'], None, 520, ANGLE_LUT),
                (O['asinLut'], None, 520, ANGLE_LUT),
                (None, c(FR, 'g_angleLut'), 520, ANGLE_LUT)],
         vectors=[
             {'args': [a, v]}
             for a in [0, 0x800, 0x4000, 0xC000, 0xFFFF]
             for v in [0, 0x1234, 0x7FFF, -1]]),

    # readAxisInput(axis): inputDisabled -> 0; else commData->useJoy==0 ->
    # axisReads[axis]; commData->useJoy != 0 would hit the joystick ABI
    dict(fn='readAxisInput', exe=EG, mod=FR, regs=['ax'],
         vectors=[
             {'args': [ax_], 'patch':
                 [P(O['inputDisabled'], FR, 'g_inputDisabled', idis),
                  (O['commData'], c(FR, 'commData'), 4, w16(0xB000) + w16(DS)),
                  (0xB072, 0xB072, 2, w16(0)),
                  (O['axisReads'], c(FR, 'g_axisInputAccum'), 8,
                   w16(0x10) + w16(0x20) + w16(-0x30) + w16(0x40))]}
             for idis in [0, 1]
             for ax_ in [0, 1, 3]]),

    # hudMessage(s): strcpy(hudMsgBuf, s); hudMsgTtl = 3*frate
    dict(fn='hudMessage', exe=EG, mod=FR, oof=0x9192, regs=[],
         vectors=[
             {'args': [0xB200], 'patch':
                 [(0xB200, 0xB200, len(txt) + 1, s8(txt)),
                  P(O['frate'], FR, 'g_frameRateScaling', fr)],
              'obs': [obsv(O['hudMsgBuf'], c(FR, 'g_hudMessageBuf'), 32),
                      OBS(O['hudMsgTtl'], FR, 'g_hudMsgTimer')]}
             for txt in ['', 'X', 'Longer message 0123']
             for fr in [1, 4, 15]]),
]

emit(cases, os.path.join(ROOT, 'dosunit/egame_combat.json'))
