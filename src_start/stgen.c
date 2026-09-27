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
struct GameComm { int8 pad[0x7a]; int16 worldBuf; };
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
    uint16 x_coord, y_coord; /* 0x00, 0x02 */
    int16 pad4;             /* 0x04 */
    int16 targetFlags;      /* 0x06 */
    int16 pad8[2];          /* 0x08 */
    int16 objectIdx;        /* 0x0c */
    int16 padE;             /* 0x0e */
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
extern int16 missionMidX[];         /* dseg:0x3e0a */
extern uint8 targets[];             /* dseg:0xb946 */

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
            int8 pad3c[2]; int16 difficulty; };
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
