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

/* ==== seg000:0x33d9 ==== */
extern int32 g_ViewX;                  /* word_37CB8/37CBA */
extern int32 g_ViewY;                  /* word_382D4/382D6 */
extern int32 g_camEyeX;                /* word_376DC/376DE */
extern int32 g_camEyeY;                /* word_379B4/379B6 */
extern int16 g_camEyeZ;                /* word_379BE */
extern int32 g_viewTargetX;            /* word_384D6/384D8 */
extern int32 g_viewTargetY;            /* word_384DC/384DE */
extern int16 g_viewTargetAlt;          /* word_384E4 */
extern int16 g_viewTargetObj;          /* word_384E6 */
extern int16 g_viewHeading;            /* word_38A14 */
extern int16 g_viewPitch;              /* word_38370 */
extern int16 g_viewRoll;               /* word_379C4 */
extern int16 g_viewZ;                  /* word_33576 */
extern int16 g_viewMode;               /* word_3836E */
extern int16 g_externalCamDist;        /* word_343C6 */
extern int16 frameTick;                /* word_343B6 */
extern int16 g_frameRateScaling;       /* word_33D92 */
extern int16 g_hudVisible;             /* word_33D90 */
extern int16 g_currentWeaponType;      /* word_388C4 */
extern int16 g_airTargetLock;          /* word_343BA */
extern int16 g_groundTargetLock;       /* word_343BC */
extern int16 g_inputDisabled;          /* word_33D8C */
extern int16 g_lastMissileSlot;        /* word_36E1E */
extern int16 g_viewX_, g_viewY_;       /* word_3837C / word_3838C */
extern int16 g_crashCamX;              /* word_384E2 */
extern int16 g_crashCamY;              /* word_384F4 */
extern int16 g_crashCamZ;              /* word_384F8 */
extern int16 g_curPanelMode;           /* word_385CE */
extern int16 g_mapMode;                /* word_38504 */
extern int16 g_lockedTargetKilled;     /* word_35AE4 */
extern int16 g_nightMode;              /* word_33D8A */
extern int16 g_detailLevel;            /* word_354BC */
extern int16 g_skyColorIndex;          /* word_38374 — used as byte */
extern int8  g_horizonGroundColor;     /* byte_2F853 */
extern int8  g_posVisibleFlag;         /* byte_32242 */
extern int8  g_savedPosVisible;        /* byte_35D3A */
extern int8  g_extraScaleShift;        /* byte_34AC4 */
extern int16 g_viewClipBottom;         /* word_3358C */
extern int16 g_camRotMatrix[];         /* 0x80B6 */
extern uint16 FAR *g_viewParamsFar;    /* dword_354D0 */
struct CommData { int8 pad78[0x78]; int16 gfxModeNum; };
extern struct CommData FAR *commData;  /* dword_38B10 */
struct ViewSnapshot { int32 worldX, worldY; int16 alt, heading, pitch, roll; };
extern struct ViewSnapshot g_viewSnapshotRing[];   /* @0x7FB6 — 16 frames */
struct SimObject { int16 objType; uint16 posX; int16 posY; int16 alt;
                   int32 worldX, worldY;
                   union { int16 w; uint8 b[2]; } heading;
                   int16 pitch; union { int16 w; uint8 b[2]; } bank;
                   int16 spec; union { uint16 w; uint8 b[2]; } flags;
                   int16 speed, timer, weaponType, terrainColor, damage; };
extern struct SimObject g_simObjects[];            /* @0x8870 */
struct Projectile { int16 mapX, mapY, alt, speed, worldX, worldY, worldZ,
                    ttl, specIdx, weaponIdx, targetLock, targetRef; };
extern struct Projectile g_projectiles[];          /* @0x5422 */
struct MapTarget { int16 objType; uint16 mapX; uint16 mapY; int16 active;
                   int16 flags; int16 alertLevel; int16 threatTimer; int16 symbol; };
extern struct MapTarget g_planeTable[];            /* @0x80C8 */
void far gfx_waitRetrace(void);                    /* sub_2F183 */
void far gfx_waitRetrace2(void);                   /* sub_2F188 */
void far gfx_nop23(void);                          /* sub_2F0D9 */
void drawAirspeedTape(void);
void insertOutlineEdges(int16 *p);
void redrawTacMap(int16 x, int16 y);               /* sub_187EC */
void fillPanelBox(int16 panelId, int16 color);     /* sub_190E8 */
void loadColorPalette(int16 mode);                 /* sub_10504 */
void render3DView(int16, int16, int16, int32, int32, int32,
                  int16, int16, int16, int16);     /* sub_1044A */
int16 clampRange(int16, int16, int16);             /* sub_1D1FA */
int16 rangeApprox(int16, int16);                   /* sub_1D23B */
int16 computeBearing(int16, int16);                /* sub_1D29D */
int16 sinMul(int16, int16);                        /* sub_1D3EC */
int16 cosMul(int16, int16);                        /* sub_1D404 */
extern int16 g_rearViewShape[];                    /* @0x4734 */
extern int16 g_leftViewShape[];                    /* @0x4818 */
extern int16 g_rightViewShape[];                   /* @0x47CE */
extern int16 g_frontViewShape[];                   /* @0x471E */
extern int16 *g_pageFront;                         /* word_34646 */
extern int16 *g_pageOffscreen;                     /* word_34676 */
extern int16 *g_pageBack;                          /* word_3465E */
void far gfx_copyRect(int16 src, int16 sx, int16 sy, int16 dst,
                      int16 dx, int16 dy, int16 w, int16 h);      /* sub_2F0FC */

void renderFrame(void) {
    int16 camDist, savedCamDist, rg, coffset, dx, dy, prevVis;

    g_camEyeX = g_viewTargetX = g_ViewX;
    g_camEyeY = g_ViewY;
    g_viewTargetY = 0x100000 - g_ViewY;
    g_camEyeZ = g_viewZ + 0x18;
    g_viewTargetAlt = g_viewZ;
    camDist = g_externalCamDist = clampRange(g_externalCamDist, 2, 8);
    switch (g_viewMode) {
    case 0x00:
    case 0x44:
        g_viewHeading = g_ourHead;
        g_viewPitch = g_ourPitch;
        g_viewRoll = g_ourRoll;
        break;
    case 0x41:
        g_viewHeading = g_ourHead + 0x8000;
        g_viewPitch = -g_ourPitch;
        g_viewRoll = -g_ourRoll;
        break;
    case 0x43:
        g_viewHeading = g_ourHead + 0x4000;
        g_viewPitch = -g_ourRoll;
        g_viewRoll = g_ourPitch;
        break;
    case 0x42:
        g_viewHeading = g_ourHead - 0x4000;
        g_viewPitch = g_ourRoll;
        g_viewRoll = -g_ourPitch;
        break;
    case 0x84:
        prevVis = (frameTick - g_frameRateScaling) & 0xF;
        g_viewHeading = g_viewSnapshotRing[prevVis].heading;
        g_viewPitch = g_viewSnapshotRing[prevVis].pitch;
        g_viewRoll = g_viewSnapshotRing[prevVis].roll;
        g_camEyeX = g_viewSnapshotRing[prevVis].worldX;
        g_camEyeY = g_viewSnapshotRing[prevVis].worldY;
        g_camEyeZ = g_viewSnapshotRing[prevVis].alt;
        break;
    case 0x85:
        g_viewHeading = g_ourHead - 0x4000;
        g_viewPitch = 0;
        g_viewRoll = 0;
        g_camEyeX = (int32)sinMul(g_ourHead + 0x4000, 0x18 << camDist) + g_ViewX;
        g_camEyeY = (int32)cosMul(g_ourHead + 0x4000, 0x18 << camDist) + g_ViewY;
        break;
    case 0x86:
        g_viewHeading = 0x8000;
        g_viewPitch = 0;
        g_viewRoll = 0;
        g_camEyeY = (int32)(0x18 << camDist) + g_ViewY;
        break;
    case 0x87:
        g_viewHeading = g_ourHead;
        g_viewPitch = 0;
        g_viewRoll = 0;
        g_camEyeX = (int32)sinMul(g_ourHead + 0x8000, 0x18 << camDist) + g_ViewX;
        g_camEyeY = (int32)cosMul(g_ourHead + 0x8000, 0x18 << camDist) + g_ViewY;
        g_camEyeZ = (4 << camDist) + g_viewZ;
        break;
    case 0x88:
    case 0x89:
    case 0x8B:
        if (g_viewMode != 0x89) {
            if (g_currentWeaponType == 1) {
                if (!(*(int8 *)&g_airTargetLock & 0x80))
                    g_viewTargetObj = g_airTargetLock + 0x20;
            } else {
                if (!(*(int8 *)&g_groundTargetLock & 0x80))
                    g_viewTargetObj = g_groundTargetLock + 0x40;
            }
        } else {
            if (g_inputDisabled == 0)
                g_viewTargetObj = g_lastMissileSlot;
        }
        savedCamDist = camDist;
        if (!(*(int8 *)&g_viewTargetObj & 0x40)) {
            if (!(*(int8 *)&g_viewTargetObj & 0x20)) {
                if (g_projectiles[g_viewTargetObj].ttl != 0) {
                    g_viewTargetX = (int32)(uint16)g_projectiles[g_viewTargetObj].mapX << 5;
                    g_viewTargetY = (int32)(uint16)g_projectiles[g_viewTargetObj].mapY << 5;
                    g_viewTargetAlt = g_projectiles[g_viewTargetObj].alt;
                } else {
                    g_projectiles[g_viewTargetObj].worldX = g_ourHead;
                    g_projectiles[g_viewTargetObj].worldY = g_ourPitch;
                }
                camDist = 3;
            } else {
                g_viewTargetX = g_simObjects[g_viewTargetObj & 0x1F].worldX;
                g_viewTargetY = g_simObjects[g_viewTargetObj & 0x1F].worldY;
                g_viewTargetAlt = g_simObjects[g_viewTargetObj & 0x1F].alt;
                camDist = 5;
            }
        } else {
            g_viewTargetX = (int32)(uint16)g_planeTable[g_viewTargetObj & 0x3F].mapX << 5;
            g_viewTargetY = (int32)(uint16)g_planeTable[g_viewTargetObj & 0x3F].mapY << 5;
            g_viewTargetAlt = 0x32;
            camDist = 7;
        }
        if (g_inputDisabled == 0)
            camDist = savedCamDist;
        dx = (int16)(g_viewTargetX >> 5) - g_viewX_;
        dy = (int16)(g_viewTargetY >> 5) - g_viewY_;
        rg = rangeApprox(dx, dy);
        g_viewHeading = computeBearing(dx, -dy);
        g_viewPitch = -computeBearing((g_viewTargetAlt - g_viewZ) >> 5, rg);
        g_viewRoll = 0;
        coffset = cosMul(g_viewPitch, 0x18 << camDist);
        if (*(int8 *)&g_viewTargetObj & 0x60) {
            if (g_viewMode == 0x88) {
                g_camEyeX = (int32)sinMul(g_viewHeading + 0x8000, coffset) + g_ViewX;
                g_camEyeY = (int32)cosMul(g_viewHeading + 0x8000, coffset) + g_ViewY;
                g_camEyeZ = (4 << camDist) + sinMul(g_viewPitch, 0x18 << camDist) + g_viewZ;
                g_viewPitch = -g_viewPitch;
            } else {
                g_camEyeX = (int32)sinMul(g_viewHeading, coffset) + g_viewTargetX;
                g_camEyeY = (int32)cosMul(g_viewHeading, coffset) - g_viewTargetY + 0x100000;
                g_camEyeZ = (4 << camDist) - sinMul(g_viewPitch, 0x18 << camDist) + g_viewTargetAlt;
                (*((char *)&g_viewHeading + 1)) += 0x80;
            }
        } else {
            g_viewHeading = g_projectiles[g_viewTargetObj].worldX;
            g_viewPitch = g_projectiles[g_viewTargetObj].worldY - 0x400;
            coffset = cosMul(g_viewPitch, 0x10 << camDist);
            g_camEyeX = g_viewTargetX - (int32)sinMul(g_viewHeading, coffset);
            g_camEyeY = 0x100000 - ((int32)cosMul(g_viewHeading, coffset) + g_viewTargetY);
            g_camEyeZ = g_viewTargetAlt - sinMul(g_viewPitch, 0x10 << camDist);
        }
        break;
    case 0x8C:
        g_viewPitch = 0xF400;
        g_viewRoll = 0;
        g_camEyeX = (int32)g_crashCamX << 5;
        g_camEyeY = ((int32)0x8000 - g_crashCamY) << 5;
        g_camEyeZ = g_crashCamZ;
        break;
    }
    if (abs(g_viewPitch) > 0x4000 || g_viewPitch == 0x8000) {
        g_viewPitch = 0x8000 - g_viewPitch;
        (*((char *)&g_viewHeading + 1)) += 0x80;
        g_viewRoll = 0x8000 - g_viewRoll;
    }
    if (g_viewMode == 0) {
        memcpy(g_camRotMatrix, g_orientMatrix, 0x12);
    } else {
        buildRotationMatrixFar(g_camRotMatrix, g_viewHeading, g_viewPitch, g_viewRoll);
    }
    g_camEyeZ = g_camEyeZ < 0x10 ? 0x10 : g_camEyeZ;
    prevVis = g_hudVisible;
    g_hudVisible = ((int8)g_viewMode & 0xC0) == 0;
    if (prevVis != g_hudVisible) {
        g_pageFront[8] = g_hudVisible ? 0x6C : 0xC7;
        g_pageBack[8] = g_hudVisible ? 0x6C : 0xC7;
        gfx_waitRetrace();
        if (g_hudVisible != 0) {
            gfx_nop23();
            gfx_copyRect(*g_pageOffscreen, 0, 0x6D, *g_pageFront, 0, 0x6D, 0x140, 0x5B);
            gfx_copyRect(*g_pageOffscreen, 0, 0x6D, *g_pageBack, 0, 0x6D, 0x140, 0x5B);
            drawAirspeedTape();
            if (g_mapMode == 0)
                redrawTacMap(g_viewX_, g_viewY_);
            if (g_curPanelMode == 0x13) {
                g_airTargetLock = g_groundTargetLock = 0xFFFF;
                fillPanelBox(2, 3);
                g_lockedTargetKilled = 0;
            }
        } else {
            gfx_copyRect(*g_pageOffscreen, 0, 0x96, *g_pageFront, 0, 0x96, 0x21, 0x32);
            gfx_copyRect(*g_pageFront, 0, 0x6D, *g_pageOffscreen, 0, 0x6D, 0x140, 0x5B);
        }
        gfx_waitRetrace2();
    }
    g_horizonGroundColor = (((uint8 FAR *)g_viewParamsFar)[0x38] & 2) ? 2 : 6;
    *(int8 *)&g_skyColorIndex = 3;
    if (g_detailLevel == 0 && commData->gfxModeNum != 0) {
        g_horizonGroundColor = 3;
        *(int8 *)&g_skyColorIndex = 0xB;
    }
    loadColorPalette(g_nightMode);
    g_posVisibleFlag = 0;
    render3DView(-g_viewHeading, g_viewPitch, g_viewRoll,
                 g_camEyeX, g_camEyeY, (int32)g_camEyeZ,
                 0, 0, 0x140, g_hudVisible ? 0x6D : 0xC8);
    g_extraScaleShift = 0;
    g_savedPosVisible = g_posVisibleFlag;
    switch (g_viewMode) {
    case 0x44:
        insertOutlineEdges(g_frontViewShape);
        break;
    case 0x41:
        insertOutlineEdges(g_rearViewShape);
        break;
    case 0x43:
        insertOutlineEdges(g_rightViewShape);
        break;
    case 0x42:
        insertOutlineEdges(g_leftViewShape);
        break;
    }
    g_viewClipBottom = (g_curPanelMode == 0x13 || g_mapMode == 1 || g_hudVisible == 0) ? 0xC8 : 0x6D;
}

/* ==== seg000:0x3bfb ==== */
extern int16 g_hudVisible;             /* word_33D90 */
extern int16 g_engineThrust;           /* word_33588 */
extern int8  g_drawPage;               /* byte_388CA */
extern int16 g_tapeClipX;              /* word_346D8 */
void setDrawColor(int16 color);                      /* sub_18F9C */
void drawFullscreenLine(int16 x1, int16 y1, int16 x2, int16 y2); /* sub_18D71 */
void blitSprite(int16 dx, int16 dy, int16 sx, int16 sy, int16 w, int16 h, int16 t); /* sub_19912 */

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

/* ==== seg000:0x3d23 ==== */
extern int16 g_edgeQuad[];              /* word_32A09 — {prevX, curX, prevY, curY} */
void far gfx_setObjAttr(int16 attr);    /* sub_2F0CF */
void far beginEdgeGroup(void);          /* sub_21D2E */
void far insertEdge(void);              /* sub_21EB0 */
void far endEdgeGroup(void);            /* sub_21D18 */

void insertOutlineEdges(int16 *p) {
    while (*p != -1) {
        gfx_setObjAttr(((const uint8 *)*p++)[0x9E8]);
        beginEdgeGroup();
        p += 2;
        while (*p != -1) {
            g_edgeQuad[0] = p[-2];
            g_edgeQuad[2] = p[-1];
            g_edgeQuad[1] = *p++;
            g_edgeQuad[3] = *p++;
            insertEdge();
        }
        endEdgeGroup();
        p++;
    }
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
