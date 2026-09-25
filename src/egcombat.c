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
void hudMessage(const char *s);     /* sub_192B1 */

struct MapEvent {                  /* 12-byte marker record */
    int16 mapX, mapY, unused4, type, ttl, unusedA;
};
extern struct MapEvent mapEvents[];   /* [0] @0x340A0 */

struct MapTarget {                 /* F19 layout, 16 bytes */
    int16 active;                  /* +0 */
    int16 field02;
    int16 alertLevel;              /* +4 */
    int16 pad[5];
};
extern struct { int16 lead[3]; struct MapTarget planes[74]; } g_planeTable;  /* planes @0x80CE */

struct TargetSlot { int16 state; int16 pad[8]; };  /* 0x12 bytes, state@0 */
extern struct TargetSlot g_targetSlots[];          /* @0x87B2 */

struct SimObject {
    int16 objType;      /* +0x00 */
    uint16 posX;        /* +0x02 */
    uint16 posY;        /* +0x04 */
    int16  alt;         /* +0x06 */
    int16  motion[8];   /* +0x08..+0x15 */
    int16  spec;        /* +0x16 */
    union { uint16 w; uint8 b[2]; } flags;  /* +0x18 */
    int16  speed;       /* +0x1A */
    int16  timer;       /* +0x1C */
    int16  weaponType;  /* +0x1E */
    int16  terrainColor;/* +0x20 */
    int16  damage;      /* +0x22 */
};                                    /* 36 bytes */
extern struct SimObject g_simObjects[];    /* @0x8870 */
struct ObjType { char name[30]; int16 kills; };      /* 32 bytes */
extern struct ObjType g_objTypes[];                  /* @0x49D6 */
extern int16 g_liveObjCount;        /* word_384FC */
extern int16 g_selSimObj;           /* word_343C4 */
extern int16 g_smokeSourceIdx;      /* word_343BE */
extern int16 g_wreckX;              /* word_3837E */
extern int16 g_wreckY;              /* word_38392 */
extern int16 g_wreckAlt;            /* word_3845E */
extern int16 g_wreckFallVel;        /* word_379B8 */
extern int16 g_missionStage;        /* word_37622 */
extern int16 g_extViewActive;       /* word_388C4 */
extern int16 g_viewObjIdx;          /* word_343BA — externally-viewed object idx */
extern int16 g_extViewReset;        /* word_35AE4 */
void notifyViewObj(int16 idx);      /* sub_14C98 */
void completeObjective(int16 n);    /* sub_17AAF */

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
        if (g_planeTable.planes[planeIdx].active != 0) {
            g_planeTable.planes[planeIdx].alertLevel = clampRange(g_planeTable.planes[planeIdx].alertLevel, ((g_missionStatus + g_difficultyTier) << 4) - 16, 0xFF);
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
            completeObjective(0);
            evt |= 0x80;
        }
        appendMapEvent(evt, g_simObjects[idx].spec + (g_simObjects[idx].flags.w & 0x4000 ? 0x80 : 0));
        if (g_simObjects[idx].speed == 0)
            g_simObjects[idx].flags.w &= 0x1C1;
    }
    strcpy(strBuf, g_objTypes[g_simObjects[idx].spec].name);
    makeSound(2, 2);
    if (g_extViewActive == 1 && idx == g_viewObjIdx)
        g_extViewReset = 1;
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
