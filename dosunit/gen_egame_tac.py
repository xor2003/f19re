#!/usr/bin/env python3
# Generates dosunit/egame_tac.json - tacmap/target math cluster.
#
# Conventions (verified against lst/egame_en_ada.lst + image bytes):
#   EN dseg offset = IDA flat - 0x2ECF0   (dseg:0 = image 0x1ECF0)
#   patch 'bytes' are raw guest bytes -> words must be little-endian.
#   Patches land in BOTH guests' shared scratch DS, so oracle and candidate
#   patch ranges must never overlap -> patch only the elements a vector reads.
#   Oracle cells >= dseg 0x6600 are BSS -> explicit bytes, never size-pulls.
import sys, os, re, struct
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))) + '/tools')
from duspec import emit, cand_off
import capstone

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
EG = 'EGAME'

def c(mapname, sym):
    v = cand_off(os.path.join(ROOT, 'build/%s.MAP' % mapname), sym)
    assert v is not None, sym
    return v

def lea_base(mapname, fn):
    """imm of `lea ax,[bx+imm]` inside fn — the cand's 3D-table base moves
    with every test-exe relink, so derive it from the current build."""
    data = open(os.path.join(ROOT, 'build/%s.EXE' % mapname), 'rb').read()
    hdr = struct.unpack('<H', data[8:10])[0] * 16
    off = c(mapname, fn)
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_16)
    for i in md.disasm(data[hdr + off:hdr + off + 0x80], off):
        if i.mnemonic == 'lea' and 'bx +' in i.op_str:
            return int(re.search(r'0x[0-9a-f]+', i.op_str).group(0), 16)
    raise AssertionError(mapname + '/' + fn + ': no lea bx+imm')

TG = 'EGTARGET'; TM = 'EGTACMAP'
C = {
    'viewX':   c(TG, 'g_viewX_'),   'viewY':   c(TG, 'g_viewY_'),
    'tRange':  c(TG, 'g_targetRange'), 'tBear': c(TG, 'g_targetBearing'),
    'stores':  c(TG, 'g_storeDefs'), 'planes': c(TG, 'g_planeTable'),
    'sims':    c(TG, 'g_simObjects'),
    'catTab':  c(TG, 'g_shapeTargetCategory'),
    'airMark': c(TG, 'g_airTargetMark'), 'gndMark': c(TG, 'g_gndTargetMark'),
    'buf3d3':  c(TG, 'buf3d3'),     'idxTab':  c(TG, 'flt15_buf1'),
    'pitch':   c(TG, 'g_ourPitch'), 'viewZ':   c(TG, 'g_viewZ'),
    'mCX': c(TM, 'g_mapCenterX'),   'mCY': c(TM, 'g_mapCenterY'),
    'mZoom': c(TM, 'g_mapZoomLevel'),
    'storesTM': c(TM, 'g_storeDefs'), 'classTM': c(TM, 'g_classTab'),
    'statTM': c(TM, 'g_statTab'),
}

# EN dseg cell offsets (flat - 0x2ECF0)
O = {
    'zoom': 0x584E, 'mCX': 0x5852, 'mCY': 0x5854,       # zoom is a BYTE cell
    'viewX': 0x94EE, 'viewY': 0x94FE,
    'tRange': 0x6344, 'tBear': 0x6348,
    'stores': 0x80AA,   # stride 16: subIdx+0 coordX+2 coordY+4 flags+8 nameIdx+14
    'sims': 0x8852,     # stride 36: posX+2 posY+4
    'catTab': 0x95D2,   # byte per shape class
    'statTab': 0x5106,  # int8 rows of 13
    'buf3d3': 0x05F6,   # int16[128]
    'idxTab': 0x6352,   # int16[]
    'airMark': 0x94F2, 'gndMark': 0x9652,              # bytes
    'pitch': 0x46DC, 'viewZ': 0x46E0,
}

# candidate's (aircraftModels - world3dData) seg004 offset delta vs oracle 0x7530
CAND_3D_BASE = lea_base(TG, 'shapeDataOffset')  # lea ax,[bx+BASE] in the cand
ORCL_3D_BASE = 0x7530          # lea ax,word_36220[bx] in sub_1D0A8
IDX_BIAS = (ORCL_3D_BASE - CAND_3D_BASE) & 0xFFFF   # cand idxTab needs +this

def w16(v): return struct.pack('<H', v & 0xFFFF).hex()
def b8(v):  return struct.pack('<B', v & 0xFF).hex()

def cat_bytes():
    # catTab[j] nibble = j&0xF, upper bits varied to prove masking
    return bytes(((j & 0x0F) | ((j & 3) << 4)) & 0xFF for j in range(128)).hex()

def stat_bytes():
    # statTab[j] = varied signed bytes incl. >=0x80 (cbw sign-extend check)
    return bytes(((j * 37 + 11) & 0xFF) for j in range(13 * 24)).hex()

OBSR = [(O['tRange'], C['tRange'], 2), (O['tBear'], C['tBear'], 2)]

def zoom_patches(zl):
    return [(None, C['mZoom'], 2, w16(zl)), (O['zoom'], None, 1, b8(zl))]

def view_patches(vx, vy):
    return [(O['viewX'], C['viewX'], 2, w16(vx)),
            (O['viewY'], C['viewY'], 2, w16(vy))]

def store_field(i, fld, val):
    return [(O['stores'] + 16 * i + fld, C['stores'] + 16 * i + fld, 2, w16(val))]

cases = [
    # ---- mapXToScreen(x): ((x - centerX) >> (10 - zoom)) + 0x5B
    #   EN instance is routine_252 @ img 0x89e7 (map name is only on the RU twin)
    dict(fn='mapXToScreen', exe=EG, mod=TM, oof=0x89e7,
         vectors=[
             {'args': [x], 'patch': zoom_patches(zl) + [
                 (O['mCX'], C['mCX'], 2, w16(cx))]}
             for x in [0, 1, 100, 0x800, 0x4000, 0x7FFF, -1, -0x4000,
                       -0x8000, 0x1234]
             for cx, zl in [(0, 0), (0x4000, 3), (0xF000, 6),
                            (0x8000, 10), (0x7FFF, 11)]]),
    dict(fn='mapYToScreen', exe=EG, mod=TM,
         vectors=[
             {'args': [y], 'patch': zoom_patches(zl) + [
                 (O['mCY'], C['mCY'], 2, w16(cy))]}
             for y in [0, 1, 100, 0x800, 0x4000, 0x7FFF, -1, -0x4000,
                       -0x8000, 0x1234]
             for cy, zl in [(0, 0), (0x4000, 3), (0xF000, 6),
                            (0x8000, 10), (0x7FFF, 11)]]),

    # ---- computeTargetBearing(x,y,f): writes targetBearing/targetRange
    dict(fn='computeTargetBearing', exe=EG, mod=TG,
         obs=OBSR,
         vectors=[
             {'args': [x, y, f], 'patch': view_patches(vx, vy)}
             for x, y, f in [(0, 0, 0), (0, 0, 1), (100, 100, 1),
                             (0x4000, 0x2000, 0), (-1, -1, 1),
                             (0x7FFF, 0x7FFF, 1), (-0x8000, 0x8000, 0),
                             (0, 1, 1), (1, 0, 1)]
             for vx, vy in [(0, 0), (0x64, 0xC8)]]),

    # ---- bearingToStore(i): storeDefs[i].coordX/coordY (stride 16)
    dict(fn='bearingToStore', exe=EG, mod=TG,
         obs=OBSR,
         vectors=[
             {'args': [i], 'patch': view_patches(vx, vy)
                  + store_field(i, 2, sx) + store_field(i, 4, sy)}
             for i in [0, 1, 2, 5, 10, 31, 50, 73]
             for vx, vy, sx, sy in [(0, 0, 0x100, 0x200),
                                    (0x64, 0xC8, 0x20, 0x40),
                                    (0x1000, -0x800, -0x400, 0x800)]]),

    # ---- bearingToSimObject(i): simObjects[i].posX/posY (stride 36)
    dict(fn='bearingToSimObject', exe=EG, mod=TG,
         obs=OBSR,
         vectors=[
             {'args': [i], 'patch': view_patches(vx, vy) + [
                 (O['sims'] + 36 * i + 2, C['sims'] + 36 * i + 2, 2, w16(sx)),
                 (O['sims'] + 36 * i + 4, C['sims'] + 36 * i + 4, 2, w16(sy))]}
             for i in [0, 1, 3, 7, 19]
             for vx, vy, sx, sy in [(0, 0, 0x100, 0x200),
                                    (0x64, 0xC8, -0x20, 0x40)]]),

    # ---- hudPitchScale(): ((0x4000-|pitch|)<<12)/(viewZ+0x1000)-0x4000
    dict(fn='hudPitchScale', exe=EG, mod=TG,
         vectors=[
             {'args': [], 'patch': [
                 (O['pitch'], C['pitch'], 2, w16(p)),
                 (O['viewZ'], C['viewZ'], 2, w16(z))]}
             for p in [0, 1, 0x400, 0x1000, 0x3FFF, 0x4000, 0x4001,
                       -1, -0x4000, -0x8000]
             for z in [0, 0x0800, 0x1000, 0x7FFF, -0x8000, -0x1000]]),

    # ---- isTargetOverWater(i): catTab[nameIndex&0x7F]&0xF in {9,11,12}
    #   oracle reads nameIdx word at stores+16i+14; cand reads
    #   g_planeTable.planes[i].nameIndex = _g_planeTable+6+16i+6.
    dict(fn='isTargetOverWater', exe=EG, mod=TG,
         patch=[(O['catTab'], C['catTab'], 128, cat_bytes())],
         vectors=[
             {'args': [i], 'patch': [
                 (O['stores'] + 16 * i + 14, C['planes'] + 12 + 16 * i, 2, w16(n))]}
             for i in [0, 1, 5]
             for n in [0, 1, 8, 9, 10, 11, 12, 13, 0x7F, 0x80, 0x89, 0x8C]]),

    # ---- getStoreMapCode(i): flags&0x80 -> (water?air:gnd)mark+0x100
    #   oracle shares one table (stores+14) for nameIdx and isTargetOverWater's
    #   nameIndex; cand's isTargetOverWater reads the separate g_planeTable
    #   copy, so the cand plane cell must get the same logical value.
    dict(fn='getStoreMapCode', exe=EG, mod=TG,
         patch=[(O['catTab'], C['catTab'], 128, cat_bytes()),
                (O['airMark'], C['airMark'], 1, '41'),
                (O['gndMark'], C['gndMark'], 1, '42')],
         vectors=[
             {'args': [i], 'patch': [
                 (O['stores'] + 16 * i + 8, C['stores'] + 16 * i + 8, 1, b8(fl)),
                 (O['stores'] + 16 * i + 14, C['stores'] + 16 * i + 14, 2, w16(n)),
                 (None, C['planes'] + 12 + 16 * i, 2, w16(n))]}
             for i in [0, 2, 7]
             for fl in [0x00, 0x80, 0xFF, 0x7F]
             for n in [0, 9, 11, 0x8C]]),

    # ---- shapeDataOffset(id): id&0x100 -> buf3d3[id&0x7f];
    #      else idxTab[id]+0x7530 (cand base 0x49D4 -> bias cand table by +0x2B5C)
    dict(fn='shapeDataOffset', exe=EG, mod=TG,
         vectors=[
             {'args': [i], 'patch':
                 ([(O['buf3d3'] + 2 * (i & 0x7f), C['buf3d3'] + 2 * (i & 0x7f),
                    2, w16(0x111 + i))] if i & 0x100 else
                  [(O['idxTab'] + 2 * i, None, 2, w16(0x222 + i)),
                   (None, C['idxTab'] + 2 * i, 2, w16(0x222 + i + IDX_BIAS))])}
             for i in [0, 1, 2, 50, 0x7F, 0x80, 0xFF,
                       0x100, 0x101, 0x150, 0x17F, 0x180, 0x200]]),

    # ---- getWeaponStat(w,sel): statTab[w*13 + (catTab[nameIdx&0x7F]&0xF)]
    dict(fn='getWeaponStat', exe=EG, mod=TM,
         patch=[(O['catTab'], C['classTM'], 128, cat_bytes()),
                (O['statTab'], C['statTM'], 13 * 24, stat_bytes())],
         vectors=[
             {'args': [w, s], 'patch': [
                 (O['stores'] + 16 * s + 14, C['storesTM'] + 16 * s + 14, 2, w16(n))]}
             for w in [0, 1, 5, 12]
             for s in [0, 3]
             for n in [0, 9, 0x8C, 0xFF]]),
]

emit(cases, os.path.join(os.path.dirname(os.path.abspath(__file__)),
                         'egame_tac.json'))
