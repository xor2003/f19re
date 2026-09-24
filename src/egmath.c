/* seg000 routines — math helpers (ported, verified vs original) */
#include <stdlib.h>
#include "inttype.h"

#define XYDIST_MAX 0x7fff

/* asm helpers (seg000, register/sp convention — not C-ported) */
int16 sine(int16 angle);        /* sub_11C2D */
int16 fixedMulQ14(int16 a, int16 b); /* sub_11BC6 */

/* ==== seg000:0xd3ec ==== */
int16 sinMul(int16 angle, int16 value) {
    return fixedMulQ14(sine(angle), value);
}

/* ==== seg000:0xd404 ==== */
int16 cosMul(int16 angle, int16 value) {
    enum { QUARTER_TURN = 0x4000 };
    return sinMul(angle + QUARTER_TURN, value);
}

/* ==== seg000:0xd29d ==== */
#define BEARING_EAST  0x4000
#define BEARING_SOUTH 0x8000
#define BEARING_WEST  0xC000
int16 computeBearing(int16 deltaX, int16 deltaY) {
    int16 angle, result;
    int32 numer;
    int16 denom, swapped, ratio;

    if (deltaX == 0) {
        if (deltaY > 0) return 0;
        return BEARING_SOUTH;
    }
    if (deltaY == 0) {
        if (deltaX > 0) return BEARING_EAST;
        return BEARING_WEST;
    }
    if (abs(deltaX) > abs(deltaY)) {
        numer = (int32)abs(deltaY) << 0xe;
        denom = abs(deltaX);
        swapped = 1;
    } else {
        numer = (int32)abs(deltaX) << 0xe;
        denom = abs(deltaY);
        swapped = 0;
    }
    ratio = (int16)(numer / (int32)denom);
    angle = (int16)(((0x2800L - (((int32)abs(0x1333 - ratio) * 0xB00L) >> 0xe)) * (int32)ratio) >> 0xe);
    if (deltaX > 0) {
        if (deltaY > 0)
            result = swapped ? BEARING_EAST - angle : angle;
        else
            result = swapped ? angle + BEARING_EAST : BEARING_SOUTH - angle;
    } else {
        if (deltaY > 0)
            result = swapped ? angle + BEARING_WEST : -angle;
        else
            result = swapped ? BEARING_WEST - angle : angle + BEARING_SOUTH;
    }
    return result;
}

/* ==== seg000:0xd23b ==== */
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

/* ==== seg000:0xd1fa ==== */
int16 clampRange(int16 value, int16 minVal, int16 maxVal) {
    /* Unlike a plain clamp, very negative wrapped angles select the high end. */
    if (value > maxVal) {
        return maxVal;
    }
    if (value >= minVal) {
        return value;
    }
    if (value <= (int16)0xC000) {
        return maxVal;
    }
    return minVal;
}

/* ==== seg000:0xd223 ==== */
int16 clampValue(int16 value, int16 minVal, int16 maxVal) {
    if (value > maxVal) {
        return maxVal;
    }
    if (value < minVal) {
        return minVal;
    }
    return value;
}

/* ==== seg000:0xd419 ==== */
int16 signExtendByte(int16 v) {
    if ((uint8)v < 0x80) {
        v = (uint8)v;
    } else {
        v = (uint8)v - 0x100;
    }
    return v;
}

/* ==== seg000:0xd436 ==== */
int16 signOf(int16 value) {
    if (value == 0) {
        return 0;
    }
    if (value > 0) {
        return 1;
    }
    return -1;
}

extern int16 g_inputDisabled;         /* word_33D8C */
extern int16 g_rngSeed;               /* word_35236 */
int16 getTimeOfDay(void);             /* sub_11FE2 */

/* ==== seg000:0xd453 ==== */
void seedRng(void) {
    if (g_inputDisabled == 0) {
        g_rngSeed = getTimeOfDay();
    }
    srand(g_rngSeed);
}

/* ==== seg000:0xd46b ==== */
int16 randomRange(int16 maxVal) {
    return (int16)(((int32)rand() * (int32)maxVal) >> 0xF);
}

/* ==== seg000:0x3387 ==== */
int16 isqrt(int16 value) {
    int16 quotient, estimate;
    value = abs(value);
    if (value < 4) {
        return 1;
    }
    estimate = value >> 2;
    do {
        quotient = value / estimate;
        estimate = (estimate + quotient) >> 1;
    } while (abs(estimate - quotient) > 1);
    return estimate;
}

/* ==== seg000:0xd484 ==== */
extern int16 g_axisInputAccum[];      /* @0x5C94 */
int16 FAR misc_readJoystick(int16 axis);  /* sub_2F1FB thunk */
struct CommSetup { int8 pad[0x72]; int16 setupUseJoy; };
extern struct CommSetup FAR *commData;    /* dword_38B10 */

int16 readAxisInput(int16 axisIdx) {
    int16 value;

    if (g_inputDisabled) {
        value = 0;
    } else {
        value = ((commData->setupUseJoy) ? misc_readJoystick(axisIdx) : 0) + g_axisInputAccum[axisIdx];
    }
    return value;
}

/* ==== seg000:0xcc7e ==== */
extern int8  g_drawPage;              /* byte_388CA */
extern int16 *g_pageFront;            /* word_34646 */
extern int16 *g_pageBack;             /* word_3465E */
extern int32 g_ViewX;                 /* word_37CB8/37CBA */
extern int32 g_ViewY;                 /* word_382D4/382D6 */
extern int32 g_camEyeX;               /* word_376DC/37DE */
extern int32 g_camEyeY;               /* word_379B4/9B6 */
extern int16 g_camEyeZ;               /* word_379BE */
extern int16 g_viewMode;              /* byte_3836E */
extern int8  g_halfScaleRender;       /* byte_330EA */
extern int16 g_curLod;                /* word_388C2 */
extern int16 g_viewZ;
extern uint8 FAR g_world3dData[];     /* seg004:0 */

void pascal shiftLongLeftInPlace(int16 count, int32 *ptr);   /* unknown_libname_3 @0xFE4A */
void pascal shiftLongRightInPlace(int16 count, int32 *ptr);  /* unknown_libname_4 @0xFE6A */
int16 FAR projectSceneObject(uint8 FAR *model, int16 yaw, int16 pitch, int16 roll, int16 relX, int16 relY, int16 flag); /* sub_20716 */
void setViewPosition(int16 x, int16 y, int16 z);
int16 shapeDataOffset(int16 shapeId);

void drawWorldObject(int16 shapeId, int32 worldX, int32 worldY, int16 altitude, int16 objYaw, int16 objPitch, int16 objRoll, int16 scaleShift) {
    int16 *dpage;
    int16 dataOff;
    int32 ox, pg;
    int16 sh, dz;

    dataOff = shapeDataOffset(shapeId);
    dpage = (g_drawPage == 0) ? g_pageFront : g_pageBack;
    pg = worldX - g_ViewX;
    ox = worldY + g_ViewY - 0x01000000L;
    sh = altitude - g_viewZ;
    if ((g_viewMode & 0x80) != 0) {
        pg += g_ViewX - g_camEyeX;
        ox += g_camEyeY - g_ViewY;
        sh += g_viewZ - g_camEyeZ;
    }
    scaleShift = (g_halfScaleRender != 0) ? (scaleShift - 2) : (scaleShift - 3);
    if (scaleShift > 0) {
        shiftLongLeftInPlace(scaleShift, &pg);
        shiftLongLeftInPlace(scaleShift, &ox);
        sh <<= (char)scaleShift;
    }
    if (scaleShift < 0) {
        *(char *)&dz = -scaleShift;
        shiftLongRightInPlace(dz, &pg);
        shiftLongRightInPlace(dz, &ox);
        sh >>= (char)dz;
    }
    if ((int16)labs(pg) < 0x7FFF) {
        if ((int16)labs(ox) < 0x7FFF) {
            setViewPosition(0, 0, -sh);
            g_curLod = 1;
            projectSceneObject(g_world3dData + dataOff, -objYaw, objPitch, objRoll, (int16)pg, -(int16)ox, altitude != 0);
        }
    }
}

