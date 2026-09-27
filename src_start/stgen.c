/* START.EXE — mission-generator helpers (f15se2 stgen.c lineage; F19 field
 * offsets differ from F15's, verified against START.EXE disasm) */
#include <stdlib.h>
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

void memAppend(const void *ptr, int16 itemsz, int16 count) {
    const void far *farptr;
    farptr = ptr;
    movedata(FP_SEG(farptr), FP_OFF(farptr), FP_SEG(moveDst), FP_OFF(moveDst), itemsz * count);
    moveDst += itemsz * count;
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
    int16 x_coord, y_coord; /* 0x00, 0x02 */
    int16 pad4;             /* 0x04 */
    int16 targetFlags;      /* 0x06 */
    int16 pad8[4];          /* 0x08 */
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
