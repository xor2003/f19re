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

/* ==== seg000:0x4769 ==== */
extern int16 g_keyCode;                  /* word_384CE: pending keycode from _bios_keybrd */
extern int16 g_gaugeLevel;               /* word_343B2: HUD gauge position */
extern int16 g_ourRoll;                  /* word_33574 */
extern int16 g_viewZ;                    /* word_33576 */
extern int16 g_missionStatus;            /* word_33D86 */
extern int16 g_startRange;               /* word_36E22 */
extern int16 g_playerPlaneFlags;         /* word_356DC */
int16 dispatchKeyCmd(int16 key);         /* sub_1D4C6 */
int16 signExtendByte(int16 v);           /* sub_1D419 */
int16 computeBearing(int16 dx, int16 dy);/* sub_1D29D */
int16 abs(int16 v);
void updateHudGauge(void) {
    int16 acc, mag, dev, val, x, t, dd, ee;

    dispatchKeyCmd(g_keyCode);
    val = abs(signExtendByte(g_ourRoll >> 8));
    if (val >= 0x40)
        val = 0x80 - val;
    g_gaugeLevel = computeBearing((uint16)g_viewZ / 3 * (g_missionStatus + 1), 0x800) >> 8;
    g_gaugeLevel += (val >> 1) + ((uint16)g_startRange >> 10);
    if (g_playerPlaneFlags & 4)
        g_gaugeLevel += 0x18;
    if (g_playerPlaneFlags & 0x10)
        g_gaugeLevel += 0x10;
}

/* ==== seg000:0x47f1 ==== */
struct FireRec { int16 viewX, viewY, unk4, type, timer, unkA; };  /* stride 0xC @0x5230 */
extern struct FireRec g_fireRecs[];      /* @0x5230 */
extern int16 g_eventTimers[];            /* @0x4EF8 */
extern int16 g_viewX_, g_viewY_;         /* word_3837C / word_3838C */
extern int16 g_missionStatus;            /* word_33D86 */
extern int16 g_frameRateScaling;         /* word_33D92 */
extern int16 g_weaponMask;               /* word_33D64 */
extern char  g_nameBuf[];                /* @0x65E6 */
void hudMessage(const char *s);          /* sub_192B1 */
void makeSound(int16 id, int16 pri);     /* sub_1DEF2 */
void countermeasures(int16 type) {
    int16 n, i;
    n = -1;
    if (g_eventTimers[type]-- <= 0) {
        g_eventTimers[type] = 0;
        hudMessage("Zapas is~erpan");
        return;
    }
    if (type == 3) {
        if (g_fireRecs[0].timer == 0 && !(g_weaponMask & 0x40))
            n = 0;
    } else {
        for (i = 1; i < 4; i++)
            if (g_fireRecs[i].timer == 0)
                n = i;
    }
    if (n != -1) {
        g_fireRecs[n].viewX = g_viewX_;
        g_fireRecs[n].viewY = g_viewY_;
        g_fireRecs[n].type = type;
        g_fireRecs[n].timer = (9 - 2 * g_missionStatus) * g_frameRateScaling;
        switch (type) {
        case 1:
            strcpy(g_nameBuf, "Flare");
            break;
        case 2:
            strcpy(g_nameBuf, "Chaff");
            break;
        case 3:
            strcpy(g_nameBuf, "\\kran");
            g_fireRecs[n].timer <<= 3;
            break;
        }
        strcat(g_nameBuf, " wypu}en");
        hudMessage(g_nameBuf);
    }
    makeSound(0x16, 2);
}

/* ==== seg000:0x4905 ==== */
struct WSlot { int16 state, timer, pad[4]; };      /* sizeof = 0xC */
extern struct WSlot g_wpnSlots[];      /* @0x5236 */
void drawStatusItem(int16 idx, int16 val);         /* sub_19007 */
void tickWeaponSlots(void) {
    int16 i;
    for (i = 0; i < 4; i++)
        if (g_wpnSlots[i].timer != 0) {
            g_wpnSlots[i].timer--;
            if (g_wpnSlots[i].state == 3)
                drawStatusItem(7, g_wpnSlots[i].timer != 0 ? 0xC : 0);
            return;
        }
}

/* ==== seg000:0x4956 ==== */
struct BulletTrack { int16 posX, posY, alt, velX, velY, velZ; };  /* stride 0xC @0x9BA6 */
extern struct BulletTrack bulletTracks[];
extern int16 g_bulletTrackCount;      /* word_373EC */
extern int16 g_gunAmmo;               /* word_33D82 */
extern int16 g_gunFiredFlag;          /* word_354C6 */
extern int16 g_ourHead;               /* word_33570 */
extern int16 g_ourPitch;              /* word_33572 */
extern int16 g_viewZ;                 /* word_33576 */
extern int16 g_ejectState;            /* word_382D8 */
extern int16 frameTick;               /* word_343B6 */
int16 readAxisInput(int16 axis);      /* sub_1D484 */
int16 sinMul(int16 angle, int16 val); /* sub_1D3EC */
int16 cosMul(int16 angle, int16 val); /* sub_1D404 */
int16 clampRange(int16 v, int16 lo, int16 hi);  /* sub_1D1FA */
void updateBulletsAndFire(void) {
    int16 tmp, sel, i, j, n;

    for (i = 0; i < g_bulletTrackCount + 4; i++) {
        if (bulletTracks[i].posX != 0) {
            bulletTracks[i].posX += bulletTracks[i].velX;
            bulletTracks[i].posY += bulletTracks[i].velY;
            bulletTracks[i].alt += bulletTracks[i].velZ;
        }
    }
    if (!(frameTick & 1)) {
        return;
    }
    n = (frameTick >> 1) % g_bulletTrackCount;
    if (!readAxisInput(0)) goto no_fire;
    if (g_gunAmmo <= 0) goto no_fire;
    if (g_ejectState != 0) goto no_fire;
    g_gunAmmo = clampRange(g_gunAmmo - 40 / g_frameRateScaling, 0, 1000);
    makeSound(4, 2);
    j = 0x70 / g_frameRateScaling;
    bulletTracks[n].velZ = sinMul(g_ourPitch, j) << 5;
    j = cosMul(g_ourPitch, j);
    bulletTracks[n].velX = sinMul(g_ourHead, j);
    bulletTracks[n].velY = -cosMul(g_ourHead, j);
    bulletTracks[n].posX = bulletTracks[n].velX + g_viewX_;
    bulletTracks[n].posY = bulletTracks[n].velY + g_viewY_;
    bulletTracks[n].alt = bulletTracks[n].velZ + g_viewZ - 2;
    g_gunFiredFlag = 1;
    return;
no_fire:
    bulletTracks[n].posX = 0;
    g_gunFiredFlag = 0;
}

/* ==== seg000:0x4aa8 ==== */
struct StoreDef {
    int16  subIdx;   /* +0x0 */
    uint16 coordX;   /* +0x2 */
    uint16 coordY;   /* +0x4 */
    int8   pad[8];   /* +0x6 */
    int16  nameIdx;  /* +0xE */
};                                              /* stride 0x10 */
extern struct StoreDef g_storeDefs[];   /* @0x80C8 */
struct Particle { int16 posX, posY, alt, spin; };   /* stride 8 */
extern struct Particle g_particles[8];  /* @0x5260 */
extern int16 g_smokeSourceIdx;          /* word_343BE */
extern int16 g_smokeParticleSlot;       /* word_34110 */
extern int16 frameTick;                 /* word_343B6 */
int16 randomRange(int16 range);         /* sub_1D46B */
void updateTracerParticles(void) {
    int16 i, slot;
    if (g_smokeSourceIdx != -1) {
        for (i = 0; i < 8; i++) {
            g_particles[i].alt += 10;
            g_particles[i].posY += g_particles[i].alt >> 9;
            *(((char *)&g_particles[i].spin) + 1) += 6;
        }
        if (!((char)frameTick & 0x0f)) {
            slot = (frameTick >> 4) & 7;
            g_particles[slot].posX = g_storeDefs[g_smokeSourceIdx].coordX;
            g_particles[slot].posY = g_storeDefs[g_smokeSourceIdx].coordY;
            g_particles[slot].alt = 0x80;
            g_particles[slot].spin = randomRange(0x100) << 8;
            g_smokeParticleSlot = slot;
        }
    }
}

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

/* ==== seg000:0x4d77 ==== */
extern char g_strpool[];               /* @0x9764 */
extern char *g_nameTabBase;            /* word_38506 = g_nameTab[0] slot */
extern int16 g_selStoreIdx;            /* word_37626 */
extern int32 g_worldX, g_worldY;       /* word_37CB8 / word_382D4 */
extern int16 flagFarToNear;            /* word_384CA */
void initStoreData(void) {
    int16 n, i;
    setCommWorldbufPtr();
    flagFarToNear = 1;
    moveStuff();
    g_nameTabBase = g_strpool;
    n = 1;
    for (i = 0; i < 0x2EE; i++) {
        if (g_strpool[i] == 0 && n < 0x64)
            g_nameTab[n++] = &g_strpool[i + 1];
    }
    g_worldX = ((uint32)g_storeDefs[g_selStoreIdx].coordX << 5) + 2;
    g_worldY = ((int32)0x8000 - g_storeDefs[g_selStoreIdx].coordY) << 5;
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

/* ==== seg000:0x4bf8 ==== */
extern int16 g_altitude;               /* word_33578 */
extern int16 g_startRange;             /* word_36E22 */
void initFlightParams(void) {
    g_altitude = 0x3E8;
    g_startRange = 0x1900;
}
