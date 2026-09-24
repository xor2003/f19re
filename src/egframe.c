/* egframe.c — frame/state transfer routines (F19) */
#include "inttype.h"
#include <string.h>

void moveNearFar(void *nearPtr, int16 count);   /* sub_15001 */
int16 setCommWorldbufPtr(void);                  /* sub_15045 */
void moveStuff(void);                            /* sub_14F18 */

extern int16 flagFarToNear;              /* word_384CA */

/* ==== seg000:0x4ef7 ==== */
struct FrameRec { int16 tick; int8 y, x, a, b; };      /* packed 6-byte record */
extern struct { struct FrameRec events[0x100]; } g_replayLog;  /* @0x8E64 */

void moveDataFar() {
    int16 unused1, unused2;
    setCommWorldbufPtr();
    flagFarToNear = 0;
    moveStuff();
    moveNearFar(g_replayLog.events, sizeof(g_replayLog.events));
}

/* ==== seg000:0x4f18 ==== */
extern int16 g_landTargetId[];           /* @0x9670 */
extern int16 g_waterTargetId[];          /* @0x9510 */
extern int16 g_planeCount;               /* @0x951E */
extern int16 g_targetEntityCount;        /* @0x6666 */
extern int16 g_planeScanCount;           /* @0x9C96 */
extern int16 g_planeTable[];             /* @0x80C8 */
extern int16 g_groundUnitCount;          /* @0x968E */
extern int16 g_simObjects[];             /* @0x8870 */
extern int8  g_shapeTargetCategory[0x64];/* @0x95F0 */
extern int8  g_tileKillTally[0x64];      /* @0x9524 */
extern int8  g_stringPool[0x2EE];        /* @0x9764 */
extern int8  g_mapCellFlags[0x100];      /* @0x861A */
extern int16 g_unusedSavedWord;          /* @0x9656 */
extern int16 g_padlockAircraft;          /* @0x5554 */
extern int16 waypoints[8];             /* @0x4880 */
extern int16 g_targetSlots[0x12];       /* @0x87B2 */

struct MapTarget { int16 f[8]; };        /* sizeof = 0x10 */
struct SimObject { int16 f[0x12]; };     /* sizeof = 0x24 */

/* ==== seg000:0x4b38 ==== */
extern int16 g_wreckAlt;               /* word_3845E */
extern int16 g_wreckFallVel;           /* word_379B8 */
void applyGravityFall(void) {
    if (g_wreckAlt > 0) {
        if (g_wreckFallVel > -16) {
            g_wreckFallVel -= 12;
        }
        g_wreckAlt += g_wreckFallVel;
    }
}

/* ==== seg000:0x4b53 ==== */
void seedRng(void);
void clearStatusPanel(void);
int16 randomRange(int16 range);
extern int16 frameTick;                /* word_343B6 */
extern int16 g_nightMode;              /* word_33D8A */
extern int16 g_unusedFrameVal;         /* word_35450 */
extern int16 g_missionTick;            /* word_354C0 */
extern int16 g_setupSlots[];           /* @0x37622 (stride 0x12) */
void initFrameRandom(void) {
    int16 seedSum, unused0, unused1, unused2;
    seedRng();
    clearStatusPanel();
    frameTick = randomRange(0x1000) & 0x7ff8;
    seedSum = g_setupSlots[4] + g_setupSlots[0xD];
    g_nightMode = ((seedSum & 3) == 0);
    if (g_setupSlots[0] == 1 || g_setupSlots[9] == 1) {
        g_nightMode = 0;
    }
    if (g_setupSlots[0] == 4 || g_setupSlots[9] == 4) {
        g_nightMode = 1;
    }
    g_unusedFrameVal = (seedSum & 0xF) << 8;
    g_missionTick = 0;
}

/* ==== seg000:0x4bc8 ==== */
extern int16 g_trackedEnemyIdx;        /* word_343B4 */
void resetSimObjectLocks(void) {
    int16 i;
    for (i = 0; i < g_groundUnitCount; i++) {
        ((struct SimObject *)g_simObjects)[i].f[0x10] = -1;
    }
    g_trackedEnemyIdx = -1;
}

/* ==== seg000:0x4c05 ==== */
extern int16 g_gunHits;                /* word_33D64 */
extern int16 g_bombDamageMask;         /* word_3844C */
extern int16 g_gunAmmo;                /* word_33D82 */
extern int16 g_fuelRemaining;          /* word_33D66 */
extern int16 g_stores[][2];            /* @0x4F02 */
void initWeaponLoadout(void) {
    int16 i;
    i = g_gunHits = g_bombDamageMask = 0;
    do {
        g_stores[i][0] = 9;
        i++;
    } while (i < 4);
    g_gunAmmo = 0x3E8;
    g_fuelRemaining = 0x1388;
}

/* ==== seg000:0x4c3c ==== */
struct CommSnap {
    int8  pad_0[0x26];
    int16 type;                     /* +0x26 */
    int16 arg;                      /* +0x28 */
    int8  pad_2a[0x0A];
    int16 gunHits;                  /* +0x34 */
    int16 dmgMask;                  /* +0x36 */
    int8  pad_38[0x3C];
    int16 viewY;                    /* +0x74 */
    int16 viewX;                    /* +0x76 */
};
struct CommData;
extern struct CommData FAR *commData;   /* dword_38B10 */
extern int16 g_ejectState;              /* word_382D8 */
extern int8 g_commEventFlag;            /* byte_38D18 */
extern int16 g_viewX_, g_viewY_;        /* word_3838C / word_3837C */
void recordFrame(uint8 a, uint8 b);
void commitCommSnapshot(int16 arg) {
    if (g_ejectState == 0 || arg == 0) {
        g_commEventFlag = 1;
        ((struct CommSnap FAR *)commData)->arg = arg;
        if (arg == 0 && g_ejectState == 0)
            ((struct CommSnap FAR *)commData)->type = 3;
        ((struct CommSnap FAR *)commData)->viewY = g_viewY_;
        ((struct CommSnap FAR *)commData)->viewX = g_viewX_;
        ((struct CommSnap FAR *)commData)->gunHits = g_gunHits;
        ((struct CommSnap FAR *)commData)->dmgMask = g_bombDamageMask;
        recordFrame(8, 0);
    }
}

/* ==== seg000:0x4c98 ==== */
void hwPortWrite(int16 cmd);           /* sub_14CAC (noop hw thunk) */
void sendSoundCmd(uint8 v) {
    hwPortWrite((v << 8) + 0xDB);
}

/* ==== seg000:0x4caf ==== */
extern int16 g_replayCount;            /* word_351C4 */
extern int16 g_missionTick;            /* word_354C0 */
extern int16 g_viewX_, g_viewY_;       /* word_3838C / word_3837C */
void recordFrame(uint8 a, uint8 b) {
    if (g_replayCount < 0xFF) {
        g_replayLog.events[g_replayCount].tick = g_missionTick;
        g_replayLog.events[g_replayCount].y = (uint16)g_viewY_ >> 7;
        g_replayLog.events[g_replayCount].x = (uint16)g_viewX_ >> 7;
        g_replayLog.events[g_replayCount].a = a;
        g_replayLog.events[g_replayCount].b = b;
        g_replayLog.events[g_replayCount += 1].a = 0;
    }
}

/* ==== seg000:0x4d03 ==== */
struct StoreDef { int16 subIdx; int8 pad[0xC]; int16 nameIdx; }; /* stride 0x10 */
extern struct StoreDef g_storeDefs[];   /* @0x80C8 */
extern char *g_nameTab[];               /* @0x9696 (word_38506) */
extern char g_nameBuf[];                /* @0x65E6 (byte_35456) */
void buildStoreName(int16 i) {
    strcpy(g_nameBuf, g_nameTab[g_storeDefs[i].nameIdx & 0x7F]);
    if (strlen(g_nameTab[g_storeDefs[i].subIdx]) != 0) {
        strcat(g_nameBuf, " ");
        strcat(g_nameBuf, g_nameTab[g_storeDefs[i].subIdx]);
    }
    if ((int16)strlen(g_nameBuf) > 0x19) {
        g_nameBuf[0x18] = '.';
        g_nameBuf[0x19] = 0;
    }
}

void moveStuff() {
    moveNearFar(g_landTargetId, 1);
    moveNearFar(g_waterTargetId, 1);
    moveNearFar(&g_planeCount, 2);
    moveNearFar(&g_targetEntityCount, 2);
    moveNearFar(&g_planeScanCount, 2);
    moveNearFar(g_planeTable, g_planeCount * (int16)sizeof(struct MapTarget));
    moveNearFar(&g_groundUnitCount, 2);
    moveNearFar(g_simObjects, g_groundUnitCount * (int16)sizeof(struct SimObject));
    moveNearFar(g_shapeTargetCategory, sizeof(g_shapeTargetCategory));
    moveNearFar(g_tileKillTally, sizeof(g_tileKillTally));
    moveNearFar(g_stringPool, sizeof(g_stringPool));
    moveNearFar(g_mapCellFlags, sizeof(g_mapCellFlags));
    moveNearFar(&g_unusedSavedWord, 2);
    moveNearFar(&g_padlockAircraft, 2);
    moveNearFar(waypoints, sizeof(waypoints));
    moveNearFar(g_targetSlots, sizeof(g_targetSlots));
}

/* ==== seg000:0x5001 ==== */
#include "pointers.h"
#include <memory.h>
#include <dos.h>
#include <string.h>
extern uint8 FAR *farPointer;          /* word_351C6/351C8 */

void moveNearFar(void *nearPtr, int16 count) {
    void FAR *farPtr = nearPtr;
    if (flagFarToNear != 0) {
        movedata(FP_SEG(farPointer), FP_OFF(farPointer), FP_SEG(farPtr), FP_OFF(farPtr), count);
    } else {
        movedata(FP_SEG(farPtr), FP_OFF(farPtr), FP_SEG(farPointer), FP_OFF(farPointer), count);
    }
    farPointer += count;
}

/* ==== seg000:0x5045 ==== */
struct CommData { int8 pad[0x7A]; int8 worldBuf[1]; };
extern struct CommData FAR *commData;  /* dword_38B10 */

int16 setCommWorldbufPtr() {
    farPointer = (uint8 FAR *)&commData->worldBuf;
    return 0;
}

/* ==== seg000:0xdef2 ==== */
extern int16 g_soundPriorityFloor;    /* word_34B08 */
extern int16 g_ejectState;            /* word_382D8 */
void FAR audio_playSound(int16 id);   /* sub_2F228 thunk */
void updateEngineSound(void);         /* sub_1DF1A */

void makeSound(int16 soundId, int16 priority) {
    if (priority >= g_soundPriorityFloor) {
        if (g_ejectState == 0 || priority > 1) {
            audio_playSound(soundId);
        }
    }
    updateEngineSound();
}

/* ==== seg000:0xdf3c ==== */
extern int16 g_frameRateScaling;      /* word_33D92 */
extern int16 g_frameSyncWait;         /* word_34AFE */
extern int16 g_timeAccelMode;         /* word_343D0 */
extern int16 g_bulletTrackCount;      /* word_373EC */
extern int16 g_threatTimerInit;       /* word_3758A */
extern int16 g_threatDisplayTtl;      /* word_35D38 */
int16 clampRange(int16 v, int16 lo, int16 hi);

void recalcTimeScale(void) {
    if (g_frameRateScaling > 15) {
        g_frameSyncWait = clampRange((-(120 / g_frameRateScaling - 9)) >> 1, 1, 4);
    } else {
        g_frameSyncWait = 0;
    }
    g_frameRateScaling = clampRange(g_frameRateScaling, 4 - g_timeAccelMode, 15);
    g_bulletTrackCount = clampRange(g_frameRateScaling << 1, 3, 16);
    g_threatTimerInit = 250 * g_frameRateScaling;
    g_threatDisplayTtl = 200 * g_frameRateScaling;
}

/* ==== seg000:0xe010 ==== */
void exitTimeAccel(void) {
    if (g_timeAccelMode == 2) {
        g_timeAccelMode = 1;
        g_frameRateScaling <<= 1;
        recalcTimeScale();
    }
}

/* ==== seg000:0x4bf8 ==== */
extern int16 g_altitude;               /* word_33578 */
extern int16 g_startRange;             /* word_36E22 */
void initFlightParams(void) {
    g_altitude = 0x3E8;
    g_startRange = 0x1900;
}
