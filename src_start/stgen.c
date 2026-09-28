/* START.EXE — mission-generator helpers (f15se2 stgen.c lineage; F19 field
 * offsets differ from F15's, verified against START.EXE disasm) */
#include <stdlib.h>
#include <stdio.h>
#include "inttype.h"
#include <dos.h>

extern void movedata(int16 srcSeg, int16 srcOff, int16 dstSeg, int16 dstOff, int16 len);

uint8 far *moveDst;   /* ds:0x98c8 — worldBuf write cursor */

#define XYDIST_MAX 0x7fff

int16 rangeApprox(int16 deltaX, int16 deltaY) {
    int32 dist;
    deltaX = abs(deltaX);
    deltaY = abs(deltaY);
    if (deltaX > deltaY)
        dist = (int32)(deltaY >> 1) + (int32)deltaX;
    else
        dist = (int32)(deltaX >> 1) + (int32)deltaY;
    if (dist > XYDIST_MAX)
        dist = XYDIST_MAX;
    return (int16)dist;
}

void memAppend(const void *ptr, int16 itemsz, int16 count, FILE *unused) {
    const void far *farptr;
    farptr = ptr;
    movedata(FP_SEG(farptr), FP_OFF(farptr), FP_SEG(moveDst), FP_OFF(moveDst), itemsz * count);
    moveDst += itemsz * count;
}

/* seg000:0x8d06 — pull bytes back out of the comm buffer (movedata FROM
 * moveDst; arg order mirrors memAppend's) */
void commFetch(void *ptr, int16 itemsz, int16 count) {
    const void far *farptr;
    farptr = ptr;
    movedata(FP_SEG(moveDst), FP_OFF(moveDst), FP_SEG(farptr), FP_OFF(farptr), itemsz * count);
    moveDst += itemsz * count;
}

/* seg000:0x8cf0 — point the comm write cursor at commData->worldBuf */
struct GameComm { int8 pad[0x2e]; int16 missionRange;  /* +0x2e */
                  int8 pad30[8];  int16 missionKind[4];  /* +0x38..3e */
                  int16 missionStat[4];                  /* +0x40..46 */
                  int8 pad48[0x32]; int16 worldBuf; };
extern struct GameComm far *commData;         /* far ptr dseg:0xd066 */

FILE *setMoveDstComm7A() {           /* K&R: no params -> no bp frame */
    moveDst = (uint8 far *)&commData->worldBuf;
    return 1;
}

void doNothing(FILE *h) {   /* seg000:0x8d72 — bare ret */
}

/* FlightUnit stride 0x24, worldObjects stride 0x10, planes stride 0x20 */
typedef struct {
    int16 waypointIdx;     /* 0x00 */
    uint16 x, y;           /* 0x02, 0x04 (uint16: <<5 widening emits sub dx,dx) */
    int16 altitude;        /* 0x06 */
    int32 xPrecise;        /* 0x08 */
    int32 yPrecise;        /* 0x0c */
    int16 heading;         /* 0x10 */
    int16 pitch;           /* 0x12 */
    int16 roll;            /* 0x14 */
    int16 planeType;       /* 0x16 */
    int16 flags;           /* 0x18 */
    int16 maxSpeed;        /* 0x1a */
    int16 fuel;            /* 0x1c */
    int16 pad1e[3];        /* 0x1e */
} FlightUnit;

typedef struct {
    int16 link;             /* 0x00 — object name/link word (dseg 0xb38e) */
    uint16 x_coord, y_coord; /* 0x02, 0x04 (uint16: <<5 widening emits sub dx,dx) */
    int16 unitType;         /* 0x06 — ground-unit type (link-chase) */
    int16 targetFlags;      /* 0x08 */
    int16 escortType;       /* 0x0a */
    int16 escortNum;        /* 0x0c */
    int16 objectIdx;        /* 0x0e */
} WorldObject;

typedef struct {
    int16 maxSpeed;         /* 0x00 */
    int16 range;            /* 0x02 */
    int16 pad4[14];         /* 0x04 */
} PlaneEntry;

extern FlightUnit flightUnits[];
extern WorldObject worldObjects[];
extern PlaneEntry planes[];
FlightUnit flightUnits[8];
WorldObject worldObjects[1];
PlaneEntry planes[1];

void positionUnit(int16 unit, int16 loc) {
    int16 planeType;
    planeType = flightUnits[unit].planeType;
    flightUnits[unit].x = worldObjects[loc].x_coord + 9;
    flightUnits[unit].y = worldObjects[loc].y_coord - 12;
    flightUnits[unit].xPrecise = (int32)flightUnits[unit].x << 5;
    flightUnits[unit].yPrecise = (int32)flightUnits[unit].y << 5;
    flightUnits[unit].altitude = worldObjects[loc].targetFlags & 0x200 ? 0x8c : 0xc;
    flightUnits[unit].maxSpeed = planes[planeType].maxSpeed;
    flightUnits[unit].heading = 0xfc00;
    flightUnits[unit].pitch = 0;
    flightUnits[unit].roll = 0;
    flightUnits[unit].flags |= 0x403;
    flightUnits[unit].waypointIdx = loc;
    flightUnits[unit].fuel = ((int32)planes[planeType].range << 0xd) / flightUnits[unit].maxSpeed;
}

int16 calcBearing(int16 dx, int16 dy) {   /* seg000:0x8b80 */
    int16 angle, result;
    int32 ratio;
    int16 divisor, swapped, quotient;
    if (dx == 0) {
        return (dy > 0) ? 0 : (int16)0x8000;
    }
    if (dy == 0) {
        return (dx > 0) ? 0x4000 : (int16)0xC000;
    }
    if (abs(dx) > abs(dy)) {
        ratio = (int32)abs(dy) << 0xe;
        divisor = abs(dx);
        swapped = 1;
    }
    else {
        ratio = (int32)abs(dx) << 0xe;
        divisor = abs(dy);
        swapped = 0;
    }
    quotient = ratio / (int32)divisor;
    angle = ((0x2800 - (((int32)abs((0x1333 - quotient)) * (int32)0xb00) >> 0xe)) * (int32)quotient) >> 0xe;
    if (dx > 0) {
        if (dy > 0) {
            result = swapped != 0 ? 0x4000 - angle : angle;
        }
        else {
            result = (swapped != 0) ? angle + 0x4000 : 0x8000 - angle;
        }
    }
    else {
        if (dy > 0) {
            result = (swapped != 0) ? angle + 0xC000 : -angle;
        }
        else {
            result = (swapped != 0) ? 0xC000 - angle : angle + 0x8000;
        }
    }
    return result;
}

/* seg000:0x866c — findNearestTerrain hit → snap wx/wy to the terrain anchor,
 * reuse a matching worldObjects[] slot or write into `slot`. */
extern int16 *nearestTerrainResult;         /* dseg:0xa4c6 */
extern int16 readItemSize;                  /* dseg:0xc978 */
int16 *findNearestTerrain(int32 wx, int32 wy); /* seg000:0x6e8c */

int16 findOrPlaceItem(int16 wx, int16 wy, int16 slot) {
    int16 j;
    if ((nearestTerrainResult = findNearestTerrain((int32)wx << 5,
            (0x8000 - (int32)wy) << 5)) != 0) {
        wx = ((int32 *)nearestTerrainResult)[1] >> 5;
        wy = -((((int32 *)nearestTerrainResult)[2] >> 5) - 0x8000);
        for (j = 3; j < readItemSize; j++) {
            if (worldObjects[j].x_coord == wx && worldObjects[j].y_coord == wy)
                return j;
        }
        worldObjects[slot].x_coord = wx;
        worldObjects[slot].y_coord = wy;
        worldObjects[slot].objectIdx = *nearestTerrainResult + 0x100;
        return slot;
    }
    return -1;
}

/* ---- parseWorld / exportWorldToComm (seg000:0x88a2, 0x8a0e) ---- */
extern FILE *fileHandle;            /* dseg:0x98f4 */
extern int16 groundUnitCount;       /* dseg:0x994a */
extern int16 worldObjectCount;      /* dseg:0xd05e */
extern int16 flightUnitCount;       /* dseg:0xca66 */
extern uint8 wldReadBuf1[];         /* dseg:0xca52 */
extern uint8 wldReadBuf7[];         /* dseg:0xc9e2 */
extern uint8 wldReadBuf8[];         /* dseg:0xc97a */
extern uint8 objectTypeTable[];     /* dseg:0xc162 */
extern uint8 terrainGrid[];         /* dseg:0xb842 */
extern int8 wldReadBuf11[];         /* dseg:0xcb38 */
extern int16 wldOffsets[];          /* dseg:0xca70 */
extern int16 missionDistAccum;      /* dseg:0xca4a */
extern int16 escortMissionFlag;     /* dseg:0x98ea */
extern uint16 missionMidX[];        /* dseg:0x3e0a */

struct Target {             /* dseg:0xb946, stride 0x12 */
    int16 kind;             /* +0  mission-target kind */
    int16 objIdx;           /* +2  worldObjects[] index */
    int16 siteObj;          /* +4  site anchor object */
    int16 flags;            /* +6  lo=site flags, hi=extra */
    int16 siteIdx;          /* +8  siteParms[] index */
    char  name[6];          /* +0a coord string */
    int16 tail;             /* +10 */
};
extern struct Target targets[];

struct SiteParm {           /* dseg:0x4b0c, stride 0x0c */
    int16 theaterMask;      /* +0  bit per theater */
    int16 campMask;         /* +2  bit per campaign flag */
    int16 kind;             /* +4  -> targets.kind */
    int16 reqType;          /* +6  required unit-class byte */
    int16 flags;            /* +8  -> targets.flags lo byte */
    int16 extra;            /* +0a >0 -> flags hi byte, <0 -> -planeType */
};
extern struct SiteParm siteParms[];

struct LinkPair {           /* dseg:0x447e, stride 4 */
    int16 nextA;            /* +0 */
    int16 nextB;            /* +2 */
};
extern struct LinkPair linkTab[];

extern int16 escortObj;                         /* dseg:0xbb72 */
extern int32 tgtPreciseX, tgtPreciseY;          /* dseg:0xc146, 0xc1c6 */
extern uint8 loadoutTab[];                      /* dseg:0x46f6 13-stride */
extern int16 missionSpeedTab[];                 /* dseg:0x4506 */
extern char   briefTimeA[];                     /* dseg:0x4db6 */
extern char   briefTimeB[];                     /* dseg:0x4dbc */
extern char   briefTimeC[];                     /* dseg:0x4dc2 */
extern char   briefCoord2[];                    /* dseg:0x4dc8 */
extern int16  randMul(uint16 n);
extern int16  itemDistance(int16 a, int16 b);
extern char  *getItemCoordStr(int16 idx);

void parseWorld(const char *filename) {
    int16 j, l;
    if ((fileHandle = fopen(filename, "rb")) == 0) return;
    fread(wldReadBuf1, 2, 1, fileHandle);
    fread(&readItemSize, 2, 1, fileHandle);
    fread(&groundUnitCount, 2, 1, fileHandle);
    fread(&worldObjectCount, 2, 1, fileHandle);
    fread(worldObjects, 0x10, readItemSize, fileHandle);
    fread(&flightUnitCount, 2, 1, fileHandle);
    fread(flightUnits, 0x24, flightUnitCount, fileHandle);
    fread(wldReadBuf7, 0x64, 1, fileHandle);
    fread(wldReadBuf8, 0x64, 1, fileHandle);
    fread(objectTypeTable, 0x64, 1, fileHandle);
    fread(terrainGrid, 1, 0x100, fileHandle);
    fread(wldReadBuf11, 1, 0x2ee, fileHandle);
    fclose(fileHandle);
    wldOffsets[0] = 0xcb38;    /* dseg offset of wldReadBuf11 */
    j = 1;
    for (l = 0; l < 0x2ee; l++) {
        if (wldReadBuf11[l] == 0 && j < 0x64)
            wldOffsets[j++] = (int16)(wldReadBuf11 + l + 1);
    }
}

void exportWorldToComm(const char *filename) {
    int16 unused;
    if ((fileHandle = setMoveDstComm7A(filename, "wb")) == 0) return;
    memAppend(wldReadBuf1, 2, 1, fileHandle);
    memAppend(&readItemSize, 2, 1, fileHandle);
    memAppend(&groundUnitCount, 2, 1, fileHandle);
    memAppend(&worldObjectCount, 2, 1, fileHandle);
    memAppend(worldObjects, 0x10, readItemSize, fileHandle);
    memAppend(&flightUnitCount, 2, 1, fileHandle);
    memAppend(flightUnits, 0x24, flightUnitCount, fileHandle);
    memAppend(wldReadBuf7, 0x64, 1, fileHandle);
    memAppend(wldReadBuf8, 0x64, 1, fileHandle);
    memAppend(wldReadBuf11, 1, 0x2ee, fileHandle);
    memAppend(terrainGrid, 1, 0x100, fileHandle);
    memAppend(&missionDistAccum, 2, 1, fileHandle);
    memAppend(&escortMissionFlag, 2, 1, fileHandle);
    memAppend(missionMidX, 4, 4, fileHandle);
    memAppend(targets, 0x12, 2, fileHandle);
    doNothing(fileHandle);
}

/* seg000:0x8d98 — format "TD00"-style grid ref into bufCoordStr (dseg:0x98cc).
 * EN has only theaters 0-3; other values fall through with gridOff* uninitialized. */
struct GD { int8 pad[0x38]; int16 theater; int16 isCampaignMission;
            uint8 flags3c; int8 pad3d; int16 difficulty; };
extern struct GD far *gameData;             /* far ptr dseg:0x991c */
extern int8 bufCoordStr[];                  /* dseg:0x98cc */
extern void mystrcpy(char *d, const char *s);

char *formatGridRef(int16 wx, int16 wy, int16 theater) {
    int16 gridOffX, gridOffY;
    switch (gameData->theater) {
    case 0: mystrcpy(bufCoordStr, "TD00"); gridOffX = 6; gridOffY = 4; break;
    case 1: mystrcpy(bufCoordStr, "JZ00"); gridOffX = 0; gridOffY = 0; break;
    case 2: mystrcpy(bufCoordStr, "WX00"); gridOffX = 0; gridOffY = 0; break;
    case 3: mystrcpy(bufCoordStr, "CC00"); gridOffX = 3; gridOffY = 5; break;
    }
    wx = (((wx >> 5) * 0x14) >> 0xa) + gridOffX;
    while (wx > 9) {
        wx -= 0xa;
        bufCoordStr[0]++;
    }
    bufCoordStr[2] += (int8)wx;
    wy = (((wy >> 5) * 0x14) >> 0xa) + gridOffY;
    while (wy > 9) {
        wy -= 0xa;
        bufCoordStr[1]--;
    }
    bufCoordStr[3] += 9 - (int8)wy;
    return bufCoordStr;
}

/* seg000:0x8e9c — format "HH:MM" into buf (minutes floored to 5; first digit
 * is flagPrefix+1, set by runGenerator — END enbrief.c formatTime lineage) */
extern int16 missionTimeFlag;               /* dseg:0x44e4 (word_244E4) */

void formatTimeStr(char *buf, int16 v) {
    int16 h, m;
    mystrcpy(buf, "00:00");
    h = v / 0x708;
    buf[0] += missionTimeFlag + 1;
    buf[1] += h % 10;
    m = ((v / 0x1e) % 0x3c) / 5 * 5;
    buf[3] += m / 10;
    buf[4] += m % 10;
}

/* seg000:0x8e72 — clamp with a 0xC000 wrap guard (bearing-style clamp) */
int16 clampValue(int16 v, int16 lo, int16 hi) {
    if (v > hi) return hi;
    if (v >= lo) return v;
    if (v <= (int16)0xC000) return hi;
    return lo;
}

/* seg000:0x76c8 — snapshot gameData fields, pick theater world file, then
 * grid/terrain parse + mission generator. Frameless (no params/locals). */
extern int16 difficultySaved;               /* dseg:0x44e0 */
extern int16 theaterSaved;                  /* dseg:0x98c6 */
extern int16 flag4Saved;                    /* dseg:0x98c4 */
extern char *regnPlhPtr;                    /* dseg:0x4dac */
extern char *plhFiles[4];                   /* dseg:0x4dae: lb/pg/nc/ce .xxx */
extern void parseGridTerrain(void);         /* seg000:0x71f8 */
extern void runGenerator(void);             /* seg000:0x7738 */

void missionGenerate() {
    difficultySaved = gameData->difficulty;
    theaterSaved = gameData->theater;
    flag4Saved = gameData->isCampaignMission;
    switch (gameData->theater) {
    case 0: parseWorld("libya.wld"); break;
    case 1: parseWorld("gulf.wld"); break;
    case 2: parseWorld("nc.wld"); break;
    case 3: parseWorld("ce.wld"); break;
    }
    mystrcpy(regnPlhPtr, plhFiles[gameData->theater]);
    parseGridTerrain();
    runGenerator();
}

/* ---- runGenerator (seg000:0x7738) — campaign mission generator: pick two
 * target sites, score them, place escorts, export mission data to commData.
 * Retries via goto restart_40a8 (re-runs cycle++/check) — the inner target
 * pick is a do/while, so its re-rolls do NOT consume an cycle. */

void runGenerator(void)
{
    int16 cycle;
    int16 mDist;
    int16 head;
    int16 mKind;
    int16 have;
    int16 waypt;
    int16 baseBrg;
    int16 pick;
    int16 tmpW;
    int16 rngLim;
    int16 minD2;
    int16 grType;
    int16 randW;
    int16 weap;
    int16 range[3];
    int16 randY;
    int16 retryCount;
    int16 sl;
    int16 okCnt;
    int16 m2;

    cycle = missionDistAccum = 0;
    minD2 = 0x1c2;
restart_40a8:
    cycle = cycle + 1;
    if (999 < cycle) goto counterMore1k;
    do {
        if (!(gameData->flags3c & 1)) {
            do {
                randW = randMul(worldObjectCount - 3) + 3;
            } while ((worldObjects[randW].targetFlags & 0xd01) != 1);
            targets[0].objIdx = randW;
        }
        else {
            do {
                randW = randMul(0xe0) * 0x80 + 0x840;
                randY = randMul(0xe0) * 0x80 + 0x840;
            } while ((terrainGrid[(randW >> 0xb) + ((randY >> 0xb) * 0x10)] & 3) != 0 ||
                     (targets[0].objIdx = findOrPlaceItem(randW, randY, 1)) == 0xffff ||
                     (worldObjects[targets[0].objIdx].targetFlags & 0x801) == 1);
        }
        do {
            randW = randMul(0xe0) * 0x80 + 0x840;
            randY = randMul(0xe0) * 0x80 + 0x840;
        } while ((terrainGrid[(randW >> 0xb) + ((randY >> 0xb) * 0x10)] & 3) != 0 ||
                 (targets[1].objIdx = findOrPlaceItem(randW, randY, 2)) == 0xffff ||
                 ((gameData->flags3c & 1) &&
                  (worldObjects[targets[1].objIdx].targetFlags & 0x801) == 1));
    } while (targets[0].objIdx == targets[1].objIdx ||
             (itemDistance(targets[0].objIdx, targets[1].objIdx) >> 6) > 0xc8);
    for (sl = 0; sl < 2; sl++) {
        range[sl] = 0x7fff;
        for (m2 = worldObjectCount; m2 < readItemSize; m2++) {
            register int16 f = worldObjects[m2].targetFlags;
            if ((f & 0x500) != 0 && (f & 0x201) != 0) {
                range[2] = clampValue(itemDistance(targets[sl].objIdx, m2) +
                    ((f & 0x100) != 0 ?
                     randMul(0x64) * 0x40 + 0xc80 : 0), 0, 0x7fff);
                if (range[2] < 0x7000 &&
                    randMul(0x500) + range[2] <
                        ((worldObjects[m2].targetFlags & 0x200) ? 0xc80 : 0) +
                        range[sl]) {
                    targets[sl].siteObj = m2;
                    range[sl] = range[2];
                }
            }
        }
    }
    if ((gameData->flags3c & 2) && theaterSaved == 0) {
        if (gameData->flags3c & 1) {
            targets[0].objIdx = 3;
            targets[1].objIdx = 0xf;
            targets[0].siteObj = 0x22;
            targets[1].siteObj = 0x21;
        }
        else {
            targets[0].objIdx = 0x16;
            targets[1].objIdx = 8;
            targets[0].siteObj = 0x22;
            targets[1].siteObj = 0x23;
        }
        range[0] = itemDistance(targets[0].objIdx, targets[0].siteObj);
        range[1] = itemDistance(targets[1].objIdx, targets[1].siteObj);
    }
    mDist = (itemDistance(targets[0].objIdx, targets[1].objIdx) >> 6) +
                (range[0] >> 6) + (range[1] >> 6);
    if (cycle + 0x2e4 < mDist || mDist < minD2) {
        minD2 -= 5 - difficultySaved;
        goto restart_40a8;
    }
    for (m2 = 0; m2 < 2; m2++) {
        targets[m2].kind = 0;
        for (retryCount = 0; retryCount < 2; retryCount++) {
            okCnt = 0;
            for (sl = 0; sl < 0x38; sl++) {
                if ((siteParms[sl].theaterMask & (1 << gameData->theater)) != 0 &&
                    (siteParms[sl].campMask & (1 << gameData->isCampaignMission)) != 0 &&
                    (int8)objectTypeTable[
                        worldObjects[targets[m2].objIdx].objectIdx & 0x7f] ==
                        siteParms[sl].reqType &&
                    (m2 == 0 || sl != targets[0].siteIdx)) {
                    if (retryCount != 0 && okCnt == pick) {
                        targets[m2].kind = siteParms[sl].kind;
                        targets[m2].siteIdx = sl;
                        targets[m2].flags = siteParms[sl].flags;
                        if (siteParms[sl].extra > 0)
                            targets[m2].flags += siteParms[sl].extra << 8;
                    }
                    okCnt++;
                }
            }
            pick = randMul(okCnt);
        }
    }
    if ((gameData->flags3c & 2) && theaterSaved == 0) {
        sl = (gameData->flags3c & 1) ? 5 : 0x37;
        targets[0].kind = siteParms[sl].kind;
        targets[0].siteIdx = sl;
        targets[0].flags = siteParms[sl].flags;
    }
    if (targets[0].kind == 0) goto restart_40a8;
    if (targets[1].kind == 0) goto restart_40a8;
    if (targets[0].siteIdx == targets[1].siteIdx) goto restart_40a8;
    if ((targets[0].flags & 1) && targets[1].kind == 1) goto restart_40a8;
    if ((targets[1].flags & 1) && targets[0].kind == 1) goto restart_40a8;
    if (range[0] < range[1] && !(gameData->flags3c & 2)) {
        tmpW = targets[0].objIdx;
        targets[0].objIdx = targets[1].objIdx;
        targets[1].objIdx = tmpW;
        tmpW = targets[0].kind;
        targets[0].kind = targets[1].kind;
        targets[1].kind = tmpW;
        tmpW = targets[0].siteObj;
        targets[0].siteObj = targets[1].siteObj;
        targets[1].siteObj = tmpW;
        tmpW = targets[0].siteIdx;
        targets[0].siteIdx = targets[1].siteIdx;
        targets[1].siteIdx = tmpW;
        tmpW = targets[0].flags;
        targets[0].flags = targets[1].flags;
        targets[1].flags = tmpW;
        tmpW = range[0];
        range[0] = range[1];
        range[1] = tmpW;
    }
    if ((targets[1].flags & 2) && (targets[1].flags & 2)) goto restart_40a8;
    if (targets[1].kind == 5) goto restart_40a8;
    if (targets[1].kind == 7) goto restart_40a8;
    if (targets[1].kind == 6) goto restart_40a8;
    if (targets[1].kind == 8) goto restart_40a8;
    if (targets[0].kind == 4 && difficultySaved == 0) goto restart_40a8;
    if ((targets[0].flags & 8) && targets[1].kind == 1) goto restart_40a8;
    if ((targets[1].flags & 8) && targets[0].kind == 1) goto restart_40a8;
    if ((targets[0].flags & 8) && (targets[1].flags & 8)) goto restart_40a8;
    if (targets[0].flags & 2)
        missionDistAccum =
            (itemDistance(targets[0].siteObj, targets[0].objIdx) >> 4) + 0x1c2;
    if (targets[1].flags & 2)
        missionDistAccum =
            (itemDistance(targets[0].siteObj, targets[1].objIdx) >> 4) + 0x1c2;
    escortMissionFlag = -1;
    if (siteParms[targets[0].siteIdx].extra < 0)
        flightUnits[0].planeType = -siteParms[targets[0].siteIdx].extra;
    if (targets[0].kind == 5) {
        tmpW = 0x7fff;
        escortObj = -1;
        for (m2 = 0; m2 < worldObjectCount; m2++) {
            range[2] = abs(itemDistance(targets[0].objIdx, m2) - range[0]);
            if (range[2] < tmpW &&
                (worldObjects[m2].targetFlags & 1) != 0 &&
                (worldObjects[m2].targetFlags & 0x100) == 0) {
                escortObj = m2;
                tmpW = range[2];
            }
        }
        if (escortObj == -1) goto restart_40a8;
        positionUnit(0, escortObj);
        flightUnits[0].waypointIdx = targets[0].objIdx;
        flightUnits[0].flags |= 4;
        escortMissionFlag = 0;
        mystrcpy(briefCoord2, getItemCoordStr(escortObj));
        missionDistAccum = itemDistance(escortObj, targets[0].objIdx) /
            ((flightUnits[escortMissionFlag].maxSpeed >> 6) * 3);
    }
    if (targets[0].kind == 7 || targets[0].kind == 6) {
        tmpW = 0x7fff;
        for (m2 = 3; m2 < readItemSize; m2++) {
            register int16 f = worldObjects[m2].targetFlags;
            if ((f & 0x500) == 0) continue;
            if ((f & 0xa00) != 0) continue;
            range[2] = abs(itemDistance(targets[0].objIdx, m2) - range[0]);
            if (range[2] >= tmpW) continue;
            if (m2 == targets[0].siteObj) continue;
            escortObj = m2;
            tmpW = range[2];
        }
        mystrcpy(briefCoord2, getItemCoordStr(escortObj));
        if (targets[0].kind == 7) {
            positionUnit(0, targets[0].objIdx);
            flightUnits[0].waypointIdx = escortObj;
            flightUnits[0].flags |= 4;
            escortMissionFlag = 0;
            escortObj = targets[0].objIdx;
        }
        else {
            positionUnit(0, escortObj);
            flightUnits[0].waypointIdx = targets[0].objIdx;
            flightUnits[0].flags |= 4;
            escortMissionFlag = 0;
            missionDistAccum = itemDistance(escortObj, targets[0].objIdx) /
                ((flightUnits[escortMissionFlag].maxSpeed >> 6) * 2);
        }
    }
    if (targets[0].kind == 8) {
        positionUnit(0, targets[0].objIdx);
        if (flightUnits[0].planeType == 2 && theaterSaved == 1)
            flightUnits[0].planeType = 0xc;
        flightUnits[0].waypointIdx = targets[0].objIdx;
        flightUnits[0].flags |= 0x40;
        escortMissionFlag = 0;
        escortObj = targets[0].objIdx;
    }
    if (escortMissionFlag == 0)
        flightUnits[0].fuel = 0x4e1f;
    for (m2 = 0; m2 < 2; m2++) {
        mystrcpy(targets[m2].name, getItemCoordStr(targets[m2].objIdx));
        if (targets[m2].objIdx < 3) {
            tmpW = 0x7fff;
            for (sl = 3; sl < readItemSize; sl++) {
                if ((worldObjects[sl].targetFlags & 0x500) == 0 &&
                    itemDistance(sl, targets[m2].objIdx) < tmpW &&
                    worldObjects[sl].link != 0) {
                    tmpW = itemDistance(sl, targets[m2].objIdx);
                    worldObjects[targets[m2].objIdx].link = worldObjects[sl].link;
                }
            }
        }
    }
    targets[0].tail = missionDistAccum >> 4;
counterMore1k:
    tgtPreciseX = (int32)worldObjects[targets[0].siteObj].x_coord << 5;
    tgtPreciseY = (-((int32)worldObjects[targets[0].siteObj].y_coord - 0x8000) << 5)
                  - (int32)((worldObjects[targets[0].siteObj].targetFlags & 0x200) ?
                            0 : 0x708);
    missionMidX[2] = worldObjects[targets[0].objIdx].x_coord;
    missionMidX[3] = worldObjects[targets[0].objIdx].y_coord;
    missionMidX[0] = (worldObjects[targets[0].siteObj].x_coord / 2) +
                     (missionMidX[2] / 2);
    missionMidX[1] = (worldObjects[targets[0].siteObj].y_coord / 2) +
                     (missionMidX[3] / 2);
    missionMidX[6] = worldObjects[targets[1].siteObj].x_coord;
    missionMidX[7] = worldObjects[targets[1].siteObj].y_coord;
    missionMidX[4] = worldObjects[targets[1].objIdx].x_coord;
    missionMidX[5] = worldObjects[targets[1].objIdx].y_coord;
    if (targets[0].flags & 0x10) {
        missionMidX[2] = ((missionMidX[2] >> 0xa) << 0xa) + 0x200;
        missionMidX[3] = ((missionMidX[3] >> 0xa) << 0xa) + 0x200;
    }
    for (m2 = 0; m2 < flightUnitCount - 4; m2++) {
        if ((int8)flightUnits[m2].flags & 0x80) {
            rngLim = (range[0] / 4) * (4 - difficultySaved);
            if ((int8)flightUnits[m2].flags & 0x40)
                rngLim = range[0] << 1;
            do {
                range[2] = randMul(worldObjectCount - 3) + 3;
            } while ((worldObjects[range[2]].targetFlags & 0x100) ||
                     rangeApprox(missionMidX[0] - worldObjects[range[2]].x_coord,
                                 missionMidX[1] - worldObjects[range[2]].y_coord) >
                     (rngLim += 0x10));
            positionUnit(m2, range[2]);
            rngLim = 0x3000;
            baseBrg = calcBearing(
                worldObjects[targets[0].siteObj].x_coord - flightUnits[m2].x,
                flightUnits[m2].y - worldObjects[targets[0].siteObj].y_coord);
            for (sl = 0; sl < 8; sl++) {
                waypt = randMul(worldObjectCount) + 1;
                if ((worldObjects[waypt].targetFlags & 0x400) == 0) {
                    head = calcBearing(
                        worldObjects[waypt].x_coord - flightUnits[m2].x,
                        flightUnits[m2].y - worldObjects[waypt].y_coord);
                    if (abs(baseBrg - head) < rngLim) {
                        rngLim = abs(baseBrg - head);
                        flightUnits[m2].waypointIdx = waypt;
                        break;
                    }
                }
            }
        }
        if ((flightUnits[m2].flags & 0x100) != 0 && escortMissionFlag != -1) {
            positionUnit(m2, escortObj);
            flightUnits[m2].fuel = 0x4e1f;
        }
        if (m2 != 0) {
            range[2] = 0;
            do {
                waypt = randMul(worldObjectCount - 3) + 3;
            } while (!((worldObjects[waypt].targetFlags & 0x801) == 1 &&
                       worldObjects[waypt].escortNum == 0) &&
                     range[2]++ < 20);
            worldObjects[waypt].escortType = flightUnits[m2].planeType;
            worldObjects[waypt].escortNum = randMul(theaterSaved + 1) + 1;
        }
    }
    for (m2 = 0; m2 < groundUnitCount; m2++) {
        grType = worldObjects[m2].unitType;
        if (grType != 0 && grType != 0x15) {
            switch (randMul(5) + (gameData->isCampaignMission != 0) +
                    difficultySaved) {
            case 0:
            case 1:
            case 3:
                grType = linkTab[grType].nextB;
            case 2:
            case 4:
            case 6:
                break;
            case 5:
            case 7:
            case 8:
                grType = linkTab[grType].nextA;
                break;
            }
            worldObjects[m2].unitType = grType;
            if ((worldObjects[m2].targetFlags & 8) != 0 &&
                gameData->isCampaignMission + difficultySaved + 2 < randMul(0xa))
                worldObjects[m2].unitType = 0;
        }
    }
    for (randW = 0; randW < 0x10; randW++) {
        for (randY = 0; randY < 0x10; randY++) {
            if ((terrainGrid[randY + randW * 0x10] & 0x10) != 0 &&
                randMul(5) >= difficultySaved)
                terrainGrid[randY + randW * 0x10] &= 0xef;
        }
    }
    commData->missionKind[0] = 1;
    commData->missionKind[2] = randMul(2) ? 5 : 9;
    commData->missionKind[3] = 0;
    commData->missionKind[1] = 2;
    commData->missionRange = mDist << 4;
    if (mDist * 16 > 0x2710)
        commData->missionKind[2] = 0x11;
    have = 0;
    for (m2 = 0; m2 < 2; m2++) {
        weap = -1;
        if (targets[m2].kind == 1 && have == 0) {
            weap = 0x10;
            have = 1;
        }
        if (targets[m2].kind == 4 || targets[m2].kind == 3)
            weap = 0x13;
        if (targets[m2].kind == 2) {
            do {
                weap = randMul(0x10);
            } while (loadoutTab[weap * 13 +
                ((int8)wldReadBuf7[
                    worldObjects[targets[m2].objIdx].objectIdx & 0x7f] &
                 0xf)] < 4);
        }
        if (weap != -1)
            commData->missionKind[m2] = weap;
    }
    if ((gameData->flags3c & 2) && theaterSaved == 0) {
        if (gameData->flags3c & 1) {
            commData->missionKind[0] = 5;
            commData->missionKind[2] = 1;
        }
        else {
            commData->missionKind[2] = 5;
            commData->missionKind[0] = 1;
        }
        commData->missionKind[3] = 0;
        if (mDist * 16 > 0x2710)
            commData->missionKind[3] = 0x11;
    }
    for (m2 = 0; m2 < 4; m2++)
        commData->missionStat[m2] =
            missionSpeedTab[commData->missionKind[m2] * 13];
    mKind = targets[0].siteIdx + targets[1].siteIdx;
    missionTimeFlag = ((uint8)mKind & 3) == 0;
    mKind = (mKind & 0xf) << 8;
    if (targets[0].kind == 1 || targets[1].kind == 1)
        missionTimeFlag = 0;
    if (targets[0].kind == 4 || targets[1].kind == 4)
        missionTimeFlag = 1;
    formatTimeStr(briefTimeA, mKind);
    formatTimeStr(briefTimeB, mKind + missionDistAccum);
    formatTimeStr(briefTimeC, mKind + missionDistAccum + 0x1c2);
    missionDistAccum -= (mKind + missionDistAccum) % 0x96;
}
