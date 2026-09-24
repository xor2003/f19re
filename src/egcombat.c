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
extern int16 g_viewX_;              /* word_3837C */
extern int16 g_viewY_;              /* word_3838C */
extern int16 g_viewZ;               /* word_33576 */
extern int16 g_ourHead;             /* word_33570 */
extern int16 g_acqRange;            /* word_351CE */
extern int16 g_acqAimY;             /* word_351D0 */
struct Projectile { int16 mapX, mapY, alt, speed, worldX, worldY, worldZ, ttl, specIdx, weaponIdx, targetLock, targetRef; };
extern struct Projectile g_projectiles[];  /* @0x5422 */
extern int16 g_autopilotEngaged;
extern int16 waypointIndex;         /* word_33700 */
extern char  strBuf[];              /* @0x65E6 */

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
