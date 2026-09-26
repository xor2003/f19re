/* egflight.c — F19 flight-model routines (seg000).
   Verified byte-identical against EGAME.EXE via tools/portcheck.py. */
#include "inttype.h"

/* ==== seg000:0x304a ==== */
extern int16 g_rotationCounter;        /* word_33580 */
extern int8  g_orientationDirty;       /* byte_33585 */
extern int16 g_orientMatrix[];         /* 0x46A6 */
extern int16 g_matrixScratch[];        /* 0x46EE */
extern void far multiplyMatrix3x3Far(const int16 *, const int16 *, int16 *); /* sub_2144C */
extern void *memcpy(void *, const void *, int);

void applyRotationDelta(const int16 *matA, const int16 *matB) {
    int16 p, a;

    g_rotationCounter++;
    if (!(*(char *)&g_rotationCounter & 7)) {
        g_orientationDirty = 1;
    }
    multiplyMatrix3x3Far(matA, matB, g_matrixScratch);
    memcpy(g_orientMatrix, g_matrixScratch, 18);
}

/* ==== seg000:0x3085 ==== */
extern int16 g_ourPitch;                 /* word_33572 */
extern int16 g_ourHead;                  /* word_33570 */
extern int16 g_ourRoll;                  /* word_33574 */
extern int8  g_rollWasNonzero;           /* byte_33582 */
extern int16 valueToAngle(int16);        /* sub_132F1 */
extern int16 complementAngle(int16);     /* sub_13374 */
extern int16 cosine(int16);              /* sub_11C1D — stack-arg table helper */
extern uint16 signedRatio16(int16, int16); /* sub_13277 */
extern int16 abs(int16);

void computeAttitudeAngles(void) {
    int16 cosPitch;

    g_ourPitch = valueToAngle(-g_orientMatrix[5]);
    cosPitch = cosine(g_ourPitch);
    if (cosPitch != 0) {
        if (abs(g_orientMatrix[2]) < 0x5a81) {
            g_ourHead = valueToAngle(abs((int16)signedRatio16(g_orientMatrix[2], cosPitch)));
        } else {
            g_ourHead = complementAngle(abs((int16)signedRatio16(g_orientMatrix[8], cosPitch)));
        }
        if (g_orientMatrix[2] <= 0 && g_orientMatrix[8] < 0) {
            (*((char *)&g_ourHead + 1)) += 0x80;
        }
        if (g_orientMatrix[2] > 0 && g_orientMatrix[8] < 0) {
            g_ourHead = 0x8000 - g_ourHead;
        }
        if (g_orientMatrix[2] < 0 && g_orientMatrix[8] > 0) {
            g_ourHead = -g_ourHead;
        }
        if (abs(g_orientMatrix[3]) < 0x5a81) {
            g_ourRoll = valueToAngle(abs((int16)signedRatio16(g_orientMatrix[3], cosPitch)));
        } else {
            g_ourRoll = complementAngle(abs((int16)signedRatio16(g_orientMatrix[4], cosPitch)));
        }
        if (g_orientMatrix[3] <= 0 && g_orientMatrix[4] < 0) {
            *((char *)&g_ourRoll + 1) += 0x80;
        }
        if (g_orientMatrix[3] > 0 && g_orientMatrix[4] < 0) {
            g_ourRoll = 0x8000 - g_ourRoll;
        }
        if (g_orientMatrix[3] < 0 && g_orientMatrix[4] > 0) {
            /* Force MSC to emit sub ax, ax; sub ax, g_ourRoll. */
            g_ourRoll = 0x10000 - g_ourRoll;
        }
    } else {
        g_ourRoll = 0;
        g_ourHead = valueToAngle(g_orientMatrix[1]);
        if (g_orientMatrix[3] <= 0 && g_orientMatrix[4] < 0) {
            (*((char *)&g_ourHead + 1)) += 0x80;
        }
        if (g_orientMatrix[3] > 0 && g_orientMatrix[4] < 0) {
            g_ourHead = 0x8000 - g_ourHead;
        }
        if (g_orientMatrix[3] < 0 && g_orientMatrix[4] > 0) {
            g_ourHead = -g_ourHead;
        }
    }
    if (g_ourPitch > 0x38e3 && g_ourPitch < 0x4001) {
        g_orientationDirty = 1;
    }
    if (g_ourPitch < (int16)0xc71d && g_ourPitch > (int16)0xbfff) {
        g_orientationDirty = 1;
    }
    if (g_rollWasNonzero != 0 && g_ourRoll == 0) {
        g_orientationDirty = 1;
    }
}

/* ==== seg000:0x3277 ==== */
uint16 signedRatio16(int16 numerator, int16 denominator) {
    char numeratorSign = 1;
    char denominatorSign = 1;
    int32 absNumerator;
    int32 absDenominator;

    if (numerator < 0) numeratorSign = -1;
    if (denominator < 0) denominatorSign = -1;
    absNumerator = (int32)(numerator < 0 ? -numerator : numerator);
    absDenominator = (int32)(denominator < 0 ? -denominator : denominator);
    return (uint16)((uint16)((((uint32)(uint16)absNumerator) << 16) / absDenominator >> 1)) * (uint16)(int16)numeratorSign * (uint16)(int16)denominatorSign;
}

/* ==== seg000:0x32f1 ==== */
#define ASIN_TABLE_SHIFT 9
#define WORD_DEGREE_STEP 256
extern const int16 g_angleLut[];     /* @0x3984 */

int16 valueToAngle(int16 value) {
    int16 angle, magnitude, tableIndex, tableSpan;

    if (value == (int16)0x8000) return (int16)0xc000;
    magnitude = abs(value);
    tableIndex = (magnitude >> ASIN_TABLE_SHIFT) + 1;
    for (; tableIndex >= 0; tableIndex--) {
        if (g_angleLut[tableIndex] <= magnitude) {
            tableSpan = g_angleLut[tableIndex + 1] - g_angleLut[tableIndex];
            angle = (int16)((int32)(magnitude - g_angleLut[tableIndex]) * WORD_DEGREE_STEP / (int32)tableSpan) + tableIndex * WORD_DEGREE_STEP;
            break;
        }
    }
    if (value < 0) {
        angle = -angle;
    }
    return angle;
}

/* ==== seg000:0x3374 ==== */
int16 complementAngle(int16 value) {
    enum { WORD_DEGREES_QUARTER_TURN = 0x4000 };
    return WORD_DEGREES_QUARTER_TURN - valueToAngle(value);
}

/* ==== seg000:0x3253 ==== */
int16 FAR buildRotationMatrixFar(int16 *matrix, int16 angleX, int16 angleY, int16 angleZ);

void rebuildOrientation() {
    buildRotationMatrixFar(g_orientMatrix, g_ourHead, g_ourPitch, g_ourRoll);
    g_orientationDirty = 0;
    g_rotationCounter = 0;
}

/* ==== seg000:0x3bfb ==== */
extern int16 g_hudVisible;             /* word_33D90 */
extern int16 g_engineThrust;           /* word_33588 */
extern int16 *g_pageFront;             /* word_34646 */
extern int16 *g_pageOffscreen;         /* word_34676 */
extern int16 *g_pageBack;              /* word_3465E */
extern int8  g_drawPage;               /* byte_388CA */
extern int16 g_tapeClipX;              /* word_346D8 */
void setDrawColor(int16 color);                      /* sub_18F9C */
void drawFullscreenLine(int16 x1, int16 y1, int16 x2, int16 y2); /* sub_18D71 */
void blitSprite(int16 dx, int16 dy, int16 sx, int16 sy, int16 w, int16 h, int16 t); /* sub_19912 */
void far gfx_copyRect(int16 src, int16 sx, int16 sy, int16 dst,
                      int16 dx, int16 dy, int16 w, int16 h);      /* sub_2F0FC */

void drawAirspeedTape(void) {
    int16 rung, i, savedPage;

    if (g_hudVisible == 0)
        return;
    gfx_copyRect(*g_pageOffscreen, 0, 0x96, *g_pageFront, 0, 0x96, 0x21, 0x32);
    rung = g_engineThrust / 5;
    for (i = -3; i < 3; i++) {
        register int16 sx, sy;
        setDrawColor(7);
        sx = rung + i;
        sy = 0xBA - rung;
        drawFullscreenLine(sx, sy, i + 0x15, 0xA5);
        setDrawColor(i == -3 ? 4 : 0xC);
        drawFullscreenLine(i, 0xBA, sx, sy);
    }
    rung = g_engineThrust / 5 - 0xB;
    g_tapeClipX = 0xC7;
    savedPage = g_drawPage;
    g_drawPage = 0;
    blitSprite(rung, 0xA0 - rung, 0x86, 0x24, 0x18, 0x10, 1);
    g_drawPage = savedPage;
    g_tapeClipX = 0x6D;
    gfx_copyRect(*g_pageFront, 0, 0x96, *g_pageBack, 0, 0x96, 0x21, 0x32);
}

/* ==== seg000:0x3d8f ==== */
void FAR audio_engineDroneOff(void);
void updateEngineSound(void);          /* sub_1DF1A */
extern int16 g_frameTimingAccum;       /* word_32DD0 */
int16 kbhit(void);
int16 _bios_keybrd(int16 cmd);

void waitForKeyPress(void) {
    int16 savedTiming;

    audio_engineDroneOff();
    savedTiming = g_frameTimingAccum;
loop:
    while (kbhit() == 0);
    if (_bios_keybrd(0) == 0x1900)
        goto loop;
    updateEngineSound();
    g_frameTimingAccum = savedTiming;
}
