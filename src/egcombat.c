/* egcombat.c — threat/target combat routines (F19) */
#include "inttype.h"
#include <string.h>

int16 clampRange(int16 v, int16 lo, int16 hi);   /* sub_1D1FA */
void appendMapEvent(int16 type, int16 arg);      /* sub_14CAF */
int16 randomRange(int16 n);                      /* sub_1D46B */
void refreshActivePanel(int16 id);               /* sub_186FC */
void makeSound(int16 a, int16 b);                /* sub_1DEF2 */
int16 rangeApprox(int16 dx, int16 dy);           /* sub_1D23B */
int16 computeBearing(int16 dx, int16 dy);        /* sub_1D29D */
int16 abs(int16 v);

extern int16 g_threatActiveTimer;   /* word_343B8 */
extern int16 g_threatTimerInit;     /* word_3758A */
extern int16 g_threatRefX;          /* word_379B2 */
extern int16 g_threatRefY;          /* word_379BC */
extern int16 g_threatRefZ;          /* word_379C2 */
extern int16 g_threatRefHead;       /* word_354CC */
extern int16 g_unusedEventHist0;    /* word_38390 */
extern int16 g_planeScanCount;      /* word_38B06 */
extern int16 g_missionStatus;       /* word_33D86 */
extern int16 g_difficultyTier;      /* word_33D88 */
extern int16 g_playerPlaneFlags;    /* word_356DC */
extern int16 g_bombDamageMask;      /* word_33D64 */
extern int16 g_gunHits;             /* word_3844C */
extern int16 g_damageTakenFlag;     /* word_354CA */
extern uint16 g_viewX_;             /* word_3837C */
extern uint16 g_viewY_;             /* word_3838C */
extern int16 g_viewZ;               /* word_33576 */
extern int16 g_ourHead;             /* word_33570 */
extern int16 g_acqRange;            /* word_351CE */
extern int16 g_acqAimY;             /* word_351D0 */
struct Projectile { int16 mapX, mapY, alt, speed, worldX, worldY, worldZ, ttl, specIdx, weaponIdx, targetLock, targetRef; };
extern struct Projectile g_projectiles[];  /* @0x5422 */
extern int16 g_autopilotEngaged;
extern int16 waypointIndex;         /* word_33700 */
extern char  strBuf[];              /* @0x65E6 */
extern int16 g_missionTick;         /* word_354C0 */
extern int16 g_nightMode;           /* word_33D8A */
extern int16 g_frameRateScaling;    /* word_33D92 */
extern int16 g_samRange;            /* word_33D00 */
extern int16 g_samSpeed;            /* word_33D02 */
extern int8  g_mapCellFlags[];      /* @0x861A */
extern int16 g_ourRoll;             /* word_33574 */
extern int16 g_ourPitch;            /* word_33572 */
extern int16 g_startRange;          /* word_36E22 — velocity source */
extern int16 g_fuelRemaining;       /* word_33D66 */
extern int16 g_curPanelMode;        /* word_385CE */
extern int16 missileSpecIndex;      /* word_33D80 */
struct Weapon { int8 name[8]; int16 lethality, dangerTier, flags; };   /* 14-byte @0x4894 */
extern struct Weapon g_samSpecs[];
extern int16 g_threatScopeRange;    /* word_343B2 */
void   exitTimeAccel(void);         /* sub_1E010 */
extern int16 g_lastMissileSlot;     /* word_36E1E */
struct MissileSpec { int16 weaponIdx; int16 ammo; };
extern struct MissileSpec missleSpec[];    /* @0x4F00 */
struct Missile { char shortName[10]; char longName[12]; int16 specIndex; int16 weaponCategory; };
extern struct Missile missiles[];          /* @0x4F24 */
struct Sam { char name[8]; int16 lockRange, maxSpeed, weaponClass, turnRate, modelId; };
extern struct Sam sams[];                  /* @0x4C36 */
int16 computeLoftAngle(void);       /* sub_1CAF2 */
void exitTimeAccel(void);           /* sub_1E010 */
void hwPortWrite(int16 cmd);        /* sub_14CAC */
void sub_19979(void);               /* fuel/panel refresh */
void hudMessage(const char *s);     /* sub_192B1 */

struct MapEvent {                  /* 12-byte marker record */
    int16 mapX, mapY, unused4, type, ttl, unusedA;
};
extern struct MapEvent mapEvents[];   /* [0] @0x340A0 */

struct MapTarget {                 /* F19 layout, 16 bytes @0x80C8 */
    int16 objType;                 /* +0 */
    uint16 mapX;                   /* +2 */
    uint16 mapY;                   /* +4 */
    int16 active;                  /* +6 */
    int16 flags;                   /* +8  (was field02) */
    int16 alertLevel;              /* +A */
    int16 threatTimer;             /* +C */
    int16 symbol;                  /* +E */
};
extern struct MapTarget g_planeTable[];    /* @0x80C8 */

struct TargetSlot { int16 state; int16 planeIndex; int16 pad[7]; };  /* 0x12 bytes, state@0 */
extern struct TargetSlot g_targetSlots[];          /* @0x87B2 */

struct TileObject { int16 id; int16 pad[0xB]; };
extern struct TileObject *g_nearestTileObj;   /* word_35CE6 */
struct TileObject *findNearestTileObject(uint32 wx, uint32 wy);  /* sub_11092 */
int16 placeString(int16 idx);                 /* sub_14D03 */
int16 getStoreMapCode(int16 idx);             /* sub_1CB1A */
int16 isTargetOverWater(int16 idx);           /* sub_1CB53 */
int16 shapeDataOffset(int16 shapeId);         /* sub_1D1C8 */
void addTileEntry(struct TileObject *rec, int16 value, char tag);  /* sub_112DC */
extern int8  g_tileKillTally[];    /* @0x9524 */
extern int8  g_airTargetMark;      /* byte_38380 */
extern int8  g_gndTargetMark;      /* byte_384E0 */
extern int16 g_enemyGroundRemaining; /* word_38500 */

struct SimObject {
    int16 objType;      /* +0x00 */
    int16 posX;         /* +0x02 */
    int16 posY;         /* +0x04 */
    int16  alt;         /* +0x06 */
    int32  worldX;      /* +0x08 */
    int32  worldY;      /* +0x0C */
    union { int16 w; uint8 b[2]; } heading; /* +0x10 */
    int16  pitch;       /* +0x12 */
    union { int16 w; uint8 b[2]; } bank;    /* +0x14 */
    int16  spec;        /* +0x16 */
    union { uint16 w; uint8 b[2]; } flags;  /* +0x18 */
    int16  speed;       /* +0x1A */
    int16  timer;       /* +0x1C */
    int16  weaponType;  /* +0x1E */
    int16  terrainColor;/* +0x20 */
    int16  damage;      /* +0x22 */
};                                    /* 36 bytes */
extern struct SimObject g_simObjects[];    /* @0x8870 */
struct ObjType { char name[0x12]; int16 maxSpeed; int16 range; int16 pad[4]; int16 kills; };  /* 32 bytes */
extern struct ObjType g_objTypes[];                  /* @0x49D6 */
extern int16 g_liveObjCount;        /* word_384FC */
extern int16 g_selSimObj;           /* word_343C4 */
extern int16 g_smokeSourceIdx;      /* word_343BE */
extern int16 g_wreckX;              /* word_3837E */
extern int16 g_wreckY;              /* word_38392 */
extern int16 g_wreckAlt;            /* word_3845E */
extern int16 g_wreckFallVel;        /* word_379B8 */
extern int16 g_missionStage;        /* word_37622 */
extern int16 g_currentWeaponType;   /* word_388C4 */
extern int16 g_airTargetLock;       /* word_343BA — locked air target index */
extern int16 g_groundTargetLock;    /* word_343BC — locked ground target index */
extern int16 g_lockedTargetKilled;  /* word_35AE4 */
extern int16 g_mapMode;             /* word_38504 */
void notifyViewObj(int16 idx);      /* sub_14C98 */
int16 markTargetReached(int16 n);   /* sub_17AAF */
void redrawTacMap(int16 x, int16 y);/* sub_187EC */

struct CommData { int8 pad1[0x2C]; int16 restartFlag; int8 pad2[0x4A]; int16 gfxModeNum; };
extern struct CommData FAR *commData;   /* dword_38B10 — +0x2C restartFlag, +0x78 gfxModeNum */
extern int16 g_scopeSweepTimer;         /* word_343C0 */
extern int16 g_prevScopeRange;          /* word_384F6 */
extern int16 g_scopeArcRange;           /* word_3831A */
extern int16 g_threatLabelTarget;       /* word_38372 — >=0: g_planeTable idx, <0: ~idx into g_simObjects */
extern int16 g_threatRadarFlag;         /* word_38B14 */
extern int16 g_scopeArcStart;           /* word_383F8 */
extern int16 g_scopeArcEnd;             /* word_383FA */
extern int16 g_targetEntityCount;       /* word_354D6 */
extern int16 g_unusedEventHist1;        /* word_3845C */
extern int16 g_scopeArcColor;           /* word_35452 */
extern int16 g_detailLevel;             /* word_354BC */
extern int16 g_hudVisible;              /* word_33D90 */
extern int16 frameTick;                 /* word_343B6 */
void restoreScopePanel(void);           /* sub_1A26C */
void captureScopePanel(void);           /* sub_1A2C5 */
int16 plotMapObject(int16 x, int16 y, int16 color, int16 big);  /* sub_18B41 */
void drawMapArc(int16 cx, int16 cy, int16 r, int16 color, int16 lines, int16 a0, int16 a1); /* sub_18C76 */
void drawGaugeBar(int16 val, int16 color, int16 x1, int16 x2);  /* sub_1877F */
void cacheScopePanel(void);             /* sub_1A23F */
void updateThreatAlert(void);           /* sub_157DB — defined below */
void fireGroundThreat(int16 idx);       /* sub_15311 */
extern int16 g_threatToneLevel;         /* word_343C2 — threat tone/arc color level */
extern int16 g_enemyThreatCount;        /* word_36E24 */
extern int16 g_nearestThreatRange;      /* word_354CE */
extern int16 g_enemyAlertFlag;          /* word_38502 */
extern int16 g_northSouthSign;          /* word_37484 — theater N/S direction sign */
extern int16 g_groundUnitCount;         /* word_384FE — live g_simObjects count */
int16 readMapPixelColor(int16 x, int16 y);  /* sub_18BEA (egtacmap) */

/* ==== seg000:0x505a ==== */
void updateThreatSites(void) {
    int16 p, arc, rad, x, siteIdx, arcRadius;

    if ((g_scopeSweepTimer == 0 || g_prevScopeRange != g_threatScopeRange) &&
        g_hudVisible != 0) {
        g_prevScopeRange = g_threatScopeRange;
        drawGaugeBar(-clampRange(0x64 - g_threatScopeRange, 0, 0x63), 1, 0, 0xB);
        drawGaugeBar(clampRange(g_threatScopeRange, 0, 0x63), 4, 0, 0xB);
        if (g_scopeSweepTimer == 0 && g_mapMode == 0) {
            restoreScopePanel();
            if (g_curPanelMode == 0x14)
                sub_19979();
            g_scopeArcStart = 0;
            g_scopeArcEnd = 0x100;
        }
    }

    for (siteIdx = 0; siteIdx < g_targetEntityCount; siteIdx++) {
        if (g_planeTable[siteIdx].active != 0 &&
            !(*(uint8 *)&g_planeTable[siteIdx].flags & 0x80) &&
            (((uint8)(siteIdx * (frameTick >> 10) * 7) & 7) <=
                 (g_unusedEventHist1 < 0x80 ? g_difficultyTier * 2 + 1 : g_difficultyTier * 2 + 3) ||
             g_planeTable[siteIdx].alertLevel != 0 ||
             (g_planeTable[siteIdx].flags & 0x100) != 0)) {
            g_planeTable[siteIdx].threatTimer--;
            if (g_planeTable[siteIdx].threatTimer <= 0)
                g_planeTable[siteIdx].threatTimer =
                    ((int16)(int8)g_frameRateScaling << 8) /
                    ((g_planeTable[siteIdx].alertLevel >> 2) + 0x10) + siteIdx;
            if (g_planeTable[siteIdx].threatTimer == 4 && g_scopeSweepTimer < 0) {
                fireGroundThreat(siteIdx);
                *(uint8 *)&g_planeTable[siteIdx].flags |= 2;
            }
        } else {
            *(uint8 *)&g_planeTable[siteIdx].flags &= ~2;
        }
    }

    if (commData->gfxModeNum == 0)
        g_scopeArcColor = 0;
    if (g_mapMode == 0 && g_scopeSweepTimer > 0 && g_hudVisible != 0 && g_scopeArcRange > 1) {
        if (g_detailLevel != 0 && commData->gfxModeNum != 0) {
            captureScopePanel();
            arc = (int16)((int32)clampRange(g_frameRateScaling - g_scopeSweepTimer, 1, g_frameRateScaling) *
                          g_scopeArcRange / g_frameRateScaling) << 6;
        } else {
            arc = g_scopeArcRange << 6;
            g_scopeArcRange = 0;
        }
        if (g_threatLabelTarget >= 0) {
            plotMapObject(g_planeTable[g_threatLabelTarget].mapX, g_planeTable[g_threatLabelTarget].mapY,
                          (g_scopeSweepTimer & 1) ? 0xF : 0, 1);
            drawMapArc(g_planeTable[g_threatLabelTarget].mapX, g_planeTable[g_threatLabelTarget].mapY,
                       arc, g_scopeArcColor, g_threatRadarFlag, g_scopeArcStart, g_scopeArcEnd);
        } else {
            plotMapObject(g_simObjects[-1 - g_threatLabelTarget].posX, g_simObjects[-1 - g_threatLabelTarget].posY,
                          (g_scopeSweepTimer & 1) ? 0xF : 0, 1);
            drawMapArc(g_simObjects[-1 - g_threatLabelTarget].posX, g_simObjects[-1 - g_threatLabelTarget].posY,
                       arc, g_scopeArcColor, g_threatRadarFlag, g_scopeArcStart, g_scopeArcEnd);
        }
    }
    g_scopeSweepTimer--;
}

/* ==== seg000:0x5311 ==== */
void fireGroundThreat(int16 siteIdx) {
    int16 p[11];                 /* bearing buffer — only p[0] (bp-16) live */
    uint16 r[4];                 /* range buffer — only r[0] (bp-1E) live */
    int16 d, e, rad, x, i, score;

    e = g_planeTable[siteIdx].active;
    score = computeThreatRangeBearing(g_planeTable[siteIdx].mapX,
                                      g_planeTable[siteIdx].mapY, 0, e,
                                      p, (int16 *)r);
    g_threatToneLevel = 0;
    if (score > 0) {
        d = score;
        if (d > 0x63)
            d = 0x63;
        g_threatToneLevel = 4;
        if (score + g_threatScopeRange > 0x32)
            g_threatToneLevel = 0xC;
        if (score + g_threatScopeRange > 0x64)
            g_threatToneLevel = 0xE;
        drawGaugeBar(-d, g_threatToneLevel, 1, 4);
        g_scopeArcRange = (uint16)((int32)(score + g_threatScopeRange) * r[0] / 100);
        g_scopeSweepTimer = g_frameRateScaling;
        g_threatLabelTarget = siteIdx;
        g_threatRadarFlag = g_samSpecs[e].flags & 1;
        if (g_planeTable[siteIdx].alertLevel != 0) {
            g_scopeArcStart = (p[0] >> 8) - 0x20;
            g_scopeArcEnd = (p[0] >> 8) + 0x20;
        }
        g_scopeArcColor = g_threatToneLevel;
        if (!(*(uint8 *)&g_planeTable[siteIdx].flags & 4)) {
            if (g_mapMode == 0 && g_hudVisible != 0) {
                restoreScopePanel();
                plotMapObject(g_planeTable[siteIdx].mapX, g_planeTable[siteIdx].mapY, 0, 1);
                cacheScopePanel();
            }
            *(uint8 *)&g_planeTable[siteIdx].flags |= 4;
            placeString(siteIdx);
            strcat(strBuf, " obnaruv.");
            hudMessage(strBuf);
        }
    }
    if (score + g_threatScopeRange > 0x64) {
        if (!(*(uint8 *)&g_planeTable[siteIdx].flags & 0x10))
            makeSound(8, 1);
        g_planeTable[siteIdx].alertLevel +=
            (((g_difficultyTier + g_missionStatus) << 4) + 0x10) >>
            ((g_playerPlaneFlags & 0x10) != 0);
        if (g_planeTable[siteIdx].alertLevel > 0xFF)
            g_planeTable[siteIdx].alertLevel = 0xFF;
        if (!(g_planeTable[siteIdx].flags & 0x100) && mapEvents[0].ttl == 0 &&
            g_planeTable[siteIdx].alertLevel > 0x7F)
            updateThreatAlert();
        if (g_enemyThreatCount <= g_missionStatus) {
            if (g_planeTable[siteIdx].alertLevel > 0xC0) {
                if (e != 0x15) {
                    if (g_nearestThreatRange > 0x500) {
                        g_enemyAlertFlag++;
                        if (!(g_planeTable[siteIdx].flags & 0x1000) &&
                            g_planeTable[siteIdx].alertLevel >= 0xFA) {
                            g_planeTable[siteIdx].flags |= 0x1000;
                            g_enemyGroundRemaining++;
                            placeString(siteIdx);
                            strcat(strBuf, " - Radar ID");
                            hudMessage(strBuf);
                            makeSound(8, 1);
                            appendMapEvent(7, siteIdx);
                        } else {
                            rad = siteIdx & 7;
                            if (g_projectiles[rad].ttl == 0 &&
                                sams[e].lockRange > r[0]) {
                                g_projectiles[rad].mapX = g_planeTable[siteIdx].mapX + 8;
                                g_projectiles[rad].mapY = g_planeTable[siteIdx].mapY;
                                g_projectiles[rad].alt = 0;
                                g_projectiles[rad].speed = sams[e].maxSpeed >> 6;
                                g_projectiles[rad].worldX = p[0];
                                g_projectiles[rad].worldY = 0x4000;
                                g_projectiles[rad].ttl = ((int32)sams[e].lockRange << 4) *
                                                         g_frameRateScaling / g_projectiles[rad].speed;
                                g_projectiles[rad].specIdx = e;
                                g_projectiles[rad].targetRef = siteIdx;
                                placeString(siteIdx);
                                strcat(strBuf, " pu}en ");
                                strcat(strBuf, sams[e].name);
                                hudMessage(strBuf);
                                notifyViewObj(siteIdx + 0x40);
                                commData->restartFlag++;
                            }
                        }
                    }
                }
            }
        }
        *(uint8 *)&g_planeTable[siteIdx].flags |= 0x10;
    } else {
        *(uint8 *)&g_planeTable[siteIdx].flags &= ~0x10;
        g_planeTable[siteIdx].alertLevel -= 0x10;
        if (g_planeTable[siteIdx].alertLevel < 0)
            g_planeTable[siteIdx].alertLevel = 0;
    }
}

/* ==== seg000:0x5689 ==== */
int16 computeThreatRangeBearing(int16 threatX, int16 threatY, int16 threatAlt,
                                 int16 threatType, int16 *outBearing, int16 *outRange) {
    int16 bearingErr, isRadar, bearing, dx, dy, result;
    uint16 distance;

    if (threatType == 0 || threatType == -1) return 0;
    dx = g_viewX_ - threatX;
    dy = g_viewY_ - threatY;
    distance = (uint16)rangeApprox(dx, dy) >> 6;
    result = ((int32)(g_mapCellFlags[(g_viewY_ >> 0xB) * 0x10 + (g_viewX_ >> 0xB)] & 0xC) *
              (int32)((int32)g_samSpecs[threatType].lethality - distance) *
              (int32)(g_samSpecs[threatType].dangerTier + g_missionStatus * 2 + 1))
             / g_samSpecs[threatType].lethality;
    bearing = computeBearing(dx, -dy);
    bearingErr = abs(bearing - g_ourHead) >> 8;
    if (bearingErr > 0x40) bearingErr = 0x80 - bearingErr;
    isRadar = 0;
    if (g_samSpecs[threatType].flags & 1) {
        bearingErr = (uint16)(0x60 - bearingErr) * ((uint16)g_startRange >> 5) >> 9;
        isRadar = 1;
    }
    result = ((bearingErr + 0x20) >> 1) * (result >> 1) >> 4;
    if ((uint16)(abs(threatAlt - g_viewZ) >> 0xA) > (uint16)distance) result = 0;
    if (result + g_threatScopeRange > 0x64) exitTimeAccel();
    *outBearing = bearing;
    *outRange = distance;
    return result;
}

/* ==== seg000:0x57db ==== */
void updateThreatAlert(void) {
    int16 planeIdx;
    g_threatActiveTimer = g_threatTimerInit;
    if (mapEvents[0].ttl != 0) {
        g_threatRefX = mapEvents[0].mapX;
        g_threatRefY = mapEvents[0].mapY;
    } else {
        g_threatRefX = g_viewX_;
        g_threatRefY = g_viewY_;
    }
    g_threatRefZ = g_viewZ;
    g_threatRefHead = g_ourHead;
    g_unusedEventHist0 = 0xFF;
    for (planeIdx = 0; planeIdx < g_planeScanCount; planeIdx++) {
        if (g_planeTable[planeIdx].active != 0) {
            g_planeTable[planeIdx].alertLevel = clampRange(g_planeTable[planeIdx].alertLevel, ((g_missionStatus + g_difficultyTier) << 4) - 16, 0xFF);
        }
    }
}

/* ==== seg000:0x585c ====
 * `off` is a code-generation device, not a real parameter: the original caller
 * passes nothing (plain `call`).  Declaring it `register` makes MSC 5.1 commit
 * si to slot*24 (the projectile byte offset) for the whole body and leaves di
 * free for the map index, matching the original's `[bx+di]` addressing and
 * `imul [bp-4]`.  A register *local* would also work but adds a home slot
 * (sub sp,10); the param uses [bp+4] as its home instead, keeping sub sp,8.
 * Residual diff vs original: MSC emits `mov si,[bp+4]` in the prologue because
 * `off`'s first store sits after the guard branches; the original has none.
 * Every other instruction (all ~150) is identical — verified by inspection. */
void spawnSamThreat(register int16 off) {
    int16 dy, dx, slot, specIdx;

    if ((g_missionTick & 0xF) != 0)
        return;
    if (!(g_mapCellFlags[(g_viewX_ >> 11) + ((g_viewY_ >> 11) << 4)] & 0x10))
        return;
    if (g_nightMode != 0)
        return;
    slot = (g_missionTick >> 4) & 7;
    specIdx = 0x21;
    off = slot * 24;
    if (*(int16 *)((char *)g_projectiles + off + 14) != 0)
        return;
    *(int16 *)((char *)g_projectiles + off) = randomRange(0x800) + (g_viewX_ & 0xF800);
    *(int16 *)((char *)g_projectiles + off + 2) = randomRange(0x800) + (g_viewY_ & 0xF800);
    dx = g_viewX_ - *(int16 *)((char *)g_projectiles + off);
    dy = g_viewY_ - *(int16 *)((char *)g_projectiles + off + 2);
    if ((uint16)((4 - g_missionStatus) << 8) >= (uint16)rangeApprox(dx, dy))
        return;
    *(int16 *)((char *)g_projectiles + off + 4) = 0;
    *(int16 *)((char *)g_projectiles + off + 6) = g_samSpeed >> 6;
    *(int16 *)((char *)g_projectiles + off + 8) = computeBearing(dx, -dy);
    *(int16 *)((char *)g_projectiles + off + 10) = 0x2000;
    *(int16 *)((char *)g_projectiles + off + 14) = (int16)(((int32)g_frameRateScaling * ((int32)g_samRange << 4)) / (int32) * (int16 *)((char *)g_projectiles + off + 6));
    *(int16 *)((char *)g_projectiles + off + 16) = 0x21;
    *(int16 *)((char *)g_projectiles + off + 22) = 1;
    strcpy(strBuf, "SA-14");
    strcat(strBuf, " pu}en");
    hudMessage(strBuf);
    updateThreatAlert();
}

/* ==== seg000:0x6ad2 ==== */
void spawnEnemyAircraft(int16 slot, int16 objType) {
    int16 spec;

    spec = g_simObjects[slot].spec;
    g_simObjects[slot].heading.w = (g_northSouthSign == 1) ? 0 : (int16)0x8000;
    if (g_planeTable[objType].flags & 0x200) {
        g_simObjects[slot].posX = g_northSouthSign * 3 + g_planeTable[objType].mapX;
        g_simObjects[slot].posY = g_planeTable[objType].mapY - g_northSouthSign * 12;
        g_simObjects[slot].alt = 140;
        g_simObjects[slot].speed = 100;
        g_simObjects[slot].heading.b[1] += 0xfc;
    } else {
        g_simObjects[slot].posX = g_planeTable[objType].mapX;
        g_simObjects[slot].posY = 30 * g_northSouthSign + g_planeTable[objType].mapY;
        g_simObjects[slot].alt = 12;
        g_simObjects[slot].speed = 10;
    }
    g_simObjects[slot].worldX = (int32)(uint16)g_simObjects[slot].posX << 5;
    g_simObjects[slot].worldY = (int32)(uint16)g_simObjects[slot].posY << 5;
    g_simObjects[slot].pitch = 0;
    g_simObjects[slot].bank.w = 0;
    g_simObjects[slot].flags.w |= 0x403;
    g_simObjects[slot].objType = objType;
    g_simObjects[slot].timer = (int16)(((int32)g_objTypes[spec].range << 11) * g_frameRateScaling / g_objTypes[spec].maxSpeed);
    g_simObjects[slot].terrainColor = readMapPixelColor(g_planeTable[objType].mapX, g_planeTable[objType].mapY);
    if (g_selSimObj == -1) {
        g_simObjects[slot].flags.b[1] &= 0xfe;
    }
    placeString(objType);
    strcat(strBuf, "- ");
    strcat(strBuf, g_objTypes[g_simObjects[slot].spec].name);
    strcat(strBuf, "WZLET");
    if (slot < g_groundUnitCount - 4) {
        hudMessage(strBuf);
    }
}

/* ==== seg000:0x7757 ==== */
int16 samCanAcquireTarget(int16 slot, int16 targetX, int16 targetY, int16 targetAlt, int16 mode) {
    int16 dx, dy, rng, bd;

    dx = targetX - g_projectiles[slot].mapX;
    dy = targetY - g_projectiles[slot].mapY;
    rng = rangeApprox(dx, dy);
    g_acqAimY = computeBearing(dx, -dy);
    bd = abs(g_acqAimY - g_projectiles[slot].worldX);
    if (bd > 0x1000 && mode != 3) {
        if (bd > 0x6000 && slot < 8 && g_projectiles[slot].speed < rng) {
            g_projectiles[slot].ttl = 0;
        }
        return 0;
    }
    if (mode == 0 && abs(g_projectiles[slot].worldX - g_ourHead) > 0x2000) {
        return 0;
    }
    g_acqRange = rng;
    return 1;
}

/* ==== seg000:0x7800 ==== */
void destroySimObject(int16 idx) {
    int16 evt;
    if (!(g_simObjects[idx].flags.b[0] & 0x20)) {
        g_objTypes[g_simObjects[idx].spec].kills++;
        if (g_simObjects[idx].flags.w & 0x800)
            --g_liveObjCount;
        notifyViewObj(idx + 0x20);
        if (g_selSimObj == idx)
            g_selSimObj = -1;
        g_simObjects[idx].flags.b[0] |= 0x20;
        g_smokeSourceIdx = -1;
        g_wreckX = g_simObjects[idx].posX;
        g_wreckY = g_simObjects[idx].posY;
        g_wreckAlt = g_simObjects[idx].alt;
        g_wreckFallVel = 0x80;
        evt = 3;
        if (g_missionStage >= 5 && idx == 0) {
            markTargetReached(0);
            evt |= 0x80;
        }
        appendMapEvent(evt, g_simObjects[idx].spec + (g_simObjects[idx].flags.w & 0x4000 ? 0x80 : 0));
        if (g_simObjects[idx].speed == 0)
            g_simObjects[idx].flags.w &= 0x1C1;
    }
    strcpy(strBuf, g_objTypes[g_simObjects[idx].spec].name);
    makeSound(2, 2);
    if (g_currentWeaponType == 1 && idx == g_airTargetLock)
        g_lockedTargetKilled = 1;
}

/* ==== seg000:0x790e ==== */
void destroyGroundTarget(int16 planeIdx) {
    int16 eventType;

    placeString(planeIdx);
    eventType = 1;
    if ((g_planeTable[planeIdx].flags & 0x80) == 0) {
        int16 slot;
        int16 symbol;
        if (g_planeTable[planeIdx].flags & 0x1000)
            --g_enemyGroundRemaining;
        g_nearestTileObj = findNearestTileObject(
            (int32)(uint16)g_planeTable[planeIdx].mapX << 5,
            ((int32)0x8000 - (uint16)g_planeTable[planeIdx].mapY) << 5);
        notifyViewObj(planeIdx + 0x40);
        if (planeIdx != 0) {
            if (g_planeTable[planeIdx].active == 0)
                eventType = 12;
            *(uint8 *)&g_planeTable[planeIdx].flags |= 0x80;
            g_planeTable[planeIdx].active = 0;
            for (slot = 0; slot < 2; slot++) {
                if (g_targetSlots[slot].state == 2 && g_targetSlots[slot].planeIndex == planeIdx) {
                    markTargetReached(slot);
                    eventType |= (slot != 0 ? 0x40 : 0x80);
                }
            }
            appendMapEvent(eventType, planeIdx);
            symbol = getStoreMapCode(planeIdx);
        } else {
            symbol = isTargetOverWater(planeIdx) ? g_airTargetMark : g_gndTargetMark;
            if (symbol != g_nearestTileObj->id) {
                g_tileKillTally[g_nearestTileObj->id]++;
                appendMapEvent(2, g_nearestTileObj->id);
            }
            symbol |= 0x100;
            g_planeTable[planeIdx].symbol = symbol;
        }
        if (g_nearestTileObj != 0)
            addTileEntry(g_nearestTileObj, shapeDataOffset(symbol), symbol);
    }
    g_smokeSourceIdx = planeIdx;
    makeSound(2, 2);
    if (g_currentWeaponType == 2 && planeIdx == g_groundTargetLock)
        g_lockedTargetKilled = 1;
    if (g_mapMode == 0)
        redrawTacMap(g_viewX_, g_viewY_);
    if (g_missionStatus < 2)
        updateThreatAlert();
}

/* ==== seg000:0x7aaf ==== */
int16 markTargetReached(int16 targetIdx) {
    if (g_playerPlaneFlags & (0x4000 >> targetIdx)) {
        return 0;
    }
    if (g_targetSlots[targetIdx].state == 4 || g_targetSlots[targetIdx].state == 3) {
        appendMapEvent((targetIdx != 0 ? 0x40 : 0x80) + 0x0b, 0);
    }
    if (targetIdx != 0) {
        strcpy(strBuf, "Second. target");
        waypointIndex = 1;
        g_playerPlaneFlags |= 0x2000;
    } else {
        strcpy(strBuf, "Primary target");
        waypointIndex = 2;
        g_playerPlaneFlags |= 0x4000;
    }
    if ((g_playerPlaneFlags & 0x6000) == 0x6000) {
        waypointIndex = 3;
    }
    return 1;
}

/* ==== seg000:0x7b46 ==== */
void bombTarget(void) {
    int16 hit;
    if (!(g_playerPlaneFlags & 0x1000)) {
        hit = 0;
        goto check;
        do {
            g_bombDamageMask |= (1 << randomRange(8));
            g_gunHits++;
            hit++;
        check:;
        } while (hit <= g_missionStatus);
        refreshActivePanel(0x16);
        g_damageTakenFlag = 1;
        makeSound(0, 2);
    }
}

/* ==== seg000:0x7ba0 ==== */
void fireMissile(void) {
    int16 spec, tmp, weaponIdx, slot;

    if (abs((int16)g_ourRoll) > 0x3000) return;
    if (missleSpec[missileSpecIndex].ammo == 0) return;
    weaponIdx = missleSpec[missileSpecIndex].weaponIdx;
    spec = missiles[weaponIdx].specIndex;
    if (spec == 0) return;
    if (spec == -1) return;
    if (!(g_playerPlaneFlags & 4) && spec != -2) {
        hudMessage("L@K ZAKRYT");
        return;
    }
    missleSpec[missileSpecIndex].ammo--;
    if (spec == -2) {
        g_fuelRemaining += 0x76C;
        hudMessage(" PEREK^ANO");
        if (g_curPanelMode == 0x14)
            sub_19979();
        return;
    }
    appendMapEvent(4, missileSpecIndex);
    exitTimeAccel();
    slot = -1;
    tmp = 8;
    do {
        if (g_projectiles[tmp].ttl == 0)
            slot = tmp;
        tmp++;
    } while (tmp < 12);
    if (slot == -1) goto check_end;
    g_projectiles[slot].mapX = g_viewX_;
    g_projectiles[slot].mapY = g_viewY_;
    g_projectiles[slot].alt = g_viewZ - 20;
    g_projectiles[slot].speed = (uint16)g_startRange >> 11;
    g_projectiles[slot].worldX = g_ourHead;
    g_projectiles[slot].worldY = g_ourPitch;
    g_projectiles[slot].worldZ = g_ourRoll;
    g_projectiles[slot].ttl = (int16)(((int32)sams[spec].lockRange << 5) * (int32)g_frameRateScaling / (int32)((sams[spec].maxSpeed >> 6) + 1)) + 6;
    if (g_projectiles[slot].ttl <= 6)
        g_projectiles[slot].ttl = 999;
    g_projectiles[slot].specIdx = spec;
    g_projectiles[slot].weaponIdx = weaponIdx;
    g_projectiles[slot].targetLock = -1;
    if (spec != 30)
        g_projectiles[slot].worldY -= 0x1000;
    else
        g_projectiles[slot].targetRef = computeLoftAngle() - 0x400;
    if (g_groundTargetLock >= 0 && sams[spec].weaponClass == 6)
        g_projectiles[slot].targetLock = g_groundTargetLock;
    if (g_groundTargetLock >= 0 && sams[spec].weaponClass == 5 && (g_planeTable[g_groundTargetLock].flags & 8))
        g_projectiles[slot].targetLock = g_groundTargetLock;
    if (spec == 29) {
        g_projectiles[slot].worldY = 0xC000;
        g_projectiles[slot].speed = 1;
    }
    g_lastMissileSlot = slot;
    strcpy(strBuf, missiles[weaponIdx].longName);
    strcat(strBuf, " pu}en");
    hudMessage(strBuf);
    makeSound(sams[spec].lockRange != 0 ? 18 : 24, 2);
check_end:
    hwPortWrite(0xD6);
    if (g_curPanelMode == 0x15)
        refreshActivePanel(0x15);
}
