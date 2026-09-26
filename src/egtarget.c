/* egtarget.c — target bearing/label routines (F19) */
#include "inttype.h"
#include <string.h>

void setDrawColor(int16 c);                        /* sub_18F9C */
void drawTargetBox(int16 x, int16 y, int16 size, int16 a); /* sub_1C4FC */
void drawStringActivePage(const char *text, int16 x, int16 y, int16 color); /* sub_191E5 */
int16 computeBearing(int16 dx, int16 dy);          /* sub_1D29D */
int16 rangeApprox(int16 dx, int16 dy);             /* sub_1D23B (egmath.c) */
void drawStringBothPages(const char *text, int16 x, int16 y, int16 color);

extern int16 g_scopeArcColor;       /* word_35452 */
extern int16 g_targetBearing;       /* word_351DE */
extern int16 g_targetRange;         /* word_351DA */
extern int16 g_viewX_;              /* word_3837C */
extern int16 g_viewY_;              /* word_3838C */
extern char  g_itoaScratch[];       /* @0x9678 */
extern char  strBuf[];              /* @0x65E6 */
extern int8  g_shapeTargetCategory[]; /* @0x95F0 */
extern int16 g_landTargetId[];      /* @0x9670 */
extern int16 g_waterTargetId[];     /* @0x9510 */

/* vtxScratch.vproj.x.lo / y.lo */
extern int16 g_vprojXlo;            /* word_2FF24 */
extern int16 g_vprojYlo;            /* word_30108 */

struct MapTarget {                 /* F19 layout, 16 bytes */
    int16 active;                  /* +0 */
    int16 field02;
    int16 alertLevel;              /* +4 */
    int16 nameIndex;               /* +6 */
    int16 pad[4];
};
extern struct { int16 lead[3]; struct MapTarget planes[74]; } g_planeTable;

/* ==== seg000:0xc4fc ==== */
void drawHudViewLine(int16 x1, int16 y1, int16 x2, int16 y2); /* sub_18F38 */
extern int16 g_hudVisible;         /* word_33D90 */
extern int8  g_halfScaleRender;    /* byte_330EA */
extern int8  g_drawPage;           /* byte_388CA */

void drawTargetBox(int16 centerX, int16 centerY, int16 size, int16 mode) {
    int16 halfHeight, left, top, right, bottom;
    if (g_hudVisible == 0) {
        return;
    }
    if (g_halfScaleRender != 0) {
        size >>= 1;
    }
    halfHeight = size - (size >> 2);
    right = centerX + size;
    left = centerX - size;
    bottom = centerY + halfHeight;
    top = centerY - halfHeight;
    if (mode == 0) {
        drawHudViewLine(left, top, left, bottom);
        drawHudViewLine(left, bottom, right, bottom);
        drawHudViewLine(right, bottom, right, top);
        drawHudViewLine(right, top, left, top);
    } else {
        drawHudViewLine(centerX, top, right, centerY - (halfHeight >> 1));
        drawHudViewLine(right, centerY - (halfHeight >> 1), right, centerY + (halfHeight >> 1));
        drawHudViewLine(right, centerY + (halfHeight >> 1), centerX, bottom);
        drawHudViewLine(centerX, bottom, left, centerY + (halfHeight >> 1));
        drawHudViewLine(left, centerY + (halfHeight >> 1), left, centerY - (halfHeight >> 1));
        drawHudViewLine(left, centerY - (halfHeight >> 1), centerX, top);
    }
}

/* ==== seg000:0xc60c ==== */
extern int16 g_targetLock;            /* word_351D8 — target-lock acquired flag */
void drawFullscreenLine(int16 x1, int16 y1, int16 x2, int16 y2); /* sub_18D71 (egtacmap.c) */

/* Draw the target-lock reticle: "Zahvat celi" label plus a + crosshair at
 * (0xE4,0xA0).  cx/cy are stored then constant-folded into the line args. */
void drawLockReticle(void) {
    int16 cx, cy;
    if (g_targetLock != 0 && g_hudVisible != 0) {
        if (g_drawPage != 0) {
            drawStringActivePage("Zahvat celi", 0xCE, 0x8E, 0x0E);
        }
        setDrawColor(0x0E);
        cx = 0xE4;
        cy = 0xA0;
        drawFullscreenLine(cx - 0x0A, cy, cx + 0x0A, cy);
        drawFullscreenLine(cx, cy - 0x08, cx, cy + 0x08);
    }
}

/* ==== seg000:0xc681 ==== */
void drawTargetLabel(const char *text, int16 color, int16 size) {
    if (g_vprojXlo == -1) {
        return;
    }
    setDrawColor(color);
    if (size < g_vprojXlo && 319 - size > g_vprojXlo &&
        size < g_vprojYlo && 110 - size > g_vprojYlo) {
        drawTargetBox(g_vprojXlo, g_vprojYlo, size, 1);
    }
    if (g_vprojXlo > 20 && g_vprojXlo < 280 &&
        g_vprojYlo > 0 && g_vprojYlo < 94) {
        drawStringActivePage(text, g_vprojXlo - (int16)strlen(text) * 2, g_vprojYlo + 5, g_scopeArcColor);
    }
}

/* ==== seg000:0xc719 ==== */
void buildRangeString(int16 rangeRaw) {
    int16 p, a, b, c, d;

    strcpy(strBuf, "Range ");
    strcat(strBuf, itoa(rangeRaw >> 6, g_itoaScratch, 10));
    strcat(strBuf, ".");
    strcat(strBuf, itoa((rangeRaw & 0x3f) * 2 / 13, g_itoaScratch, 10));
    strcat(strBuf, " km");
}

/* ==== seg000:0xc793 ==== */
extern int16 g_viewZ;             /* word_33576 */
extern int8  g_camExtFlag;        /* byte_3836E — low byte of g_viewMode */
extern int32 g_ViewX;             /* word_37CB8/37CBA */
extern int32 g_ViewY;             /* word_382D4/382D6 */
extern int32 g_camEyeX;           /* word_376DC/376DE */
extern int32 g_camEyeY;           /* word_379B4/379B6 */
extern int16 g_camEyeZ;           /* word_379BE */
extern int16 g_projDepth;         /* word_384D0 */
extern int16 g_aimClipSave;       /* word_351DC — raw X saved when off-screen */
int32 matVecDotAxis(int16 axis, int16 x, int16 y, int16 z); /* sub_1C95F (egmath.c) */

/* Project a world point through the view rotation and store the screen
 * position of the aim marker: X into g_vprojXlo, Y into g_vprojYlo,
 * depth into g_projDepth.  Marks g_vprojXlo = -1 when the point is
 * behind the camera or off the frustum edge. */
void computeAimProjection(int16 wx, int16 wy, int16 wz) {
    int16 x, y, z;
    int32 y0, y1, y2;

    x = g_viewX_ - wx;
    y = wy - g_viewY_;
    z = (wz - g_viewZ) >> 5;
    if (g_camExtFlag & 0x80) {
        x -= (int16)((g_ViewX - g_camEyeX) >> 5);
        y -= (int16)((g_ViewY - g_camEyeY) >> 5);
        z -= (int16)(-((int32)(uint16)g_viewZ - (int32)g_camEyeZ) >> 5);
    }
    y0 = matVecDotAxis(0, x, y, z);
    y1 = matVecDotAxis(1, x, y, z);
    y2 = matVecDotAxis(2, x, y, z);
    if (y2 < 0) {
        if (g_halfScaleRender != 0) {
            y0 >>= 1;
            y1 >>= 1;
        }
        if (-y2 >= y0) {
            if (y0 < y2) goto fail;
            g_vprojXlo = (int16)((y0 << 8) / y2) + 0xA0;
            g_vprojYlo = (int16)((y1 << 8) / y2);
            g_vprojYlo -= g_vprojYlo >> 2;
            g_vprojYlo += g_hudVisible ? (g_halfScaleRender ? 0x52 : 0x38) : 0x64;
            g_projDepth = (int16)(y2 >> 3);
            if (g_vprojXlo < 0 || g_vprojXlo > 0x13F) {
                g_aimClipSave = g_vprojXlo;
                g_vprojXlo = -1;
            }
            if (g_vprojYlo < 0 || g_vprojYlo > (g_hudVisible ? 0x70 : 0xC7)) {
                g_aimClipSave = g_vprojXlo;
                goto fail;
            }
            return;
        }
    }
fail:
    g_vprojXlo = -1;
}

/* ==== seg000:0xc9b2 ==== */
struct StoreDef { int16 subIdx; uint16 coordX; uint16 coordY; int16 f6; int8 flags; int8 f9; int16 padA; int16 padC; int16 nameIdx; };
struct SimObject { int16 f[0x12]; };
struct TileObject {
    int16 id;                      /* +0x00 */
    int16 dist;                    /* +0x02 */
    int32 x;                       /* +0x04 */
    int32 y;                       /* +0x08 */
    int16 entry;                   /* +0x0C */
    uint8 lod;                     /* +0x0E */
    uint8 subIndex;                /* +0x0F */
    uint8 tileX;                   /* +0x10 */
    uint8 tileY;                   /* +0x11 */
    int16 shapeOff;                /* +0x12 */
    uint8 flag;                    /* +0x14 */
    uint8 pad15;                   /* +0x15 */
};
extern struct StoreDef g_storeDefs[];   /* @0x80C8 */
extern struct SimObject g_simObjects[]; /* @0x8870 */
extern struct TileObject *g_nearestTileObj;  /* word_35CE6 */
struct TileObject *findNearestTileObject(uint32 worldX, uint32 worldY); /* sub_11092 (eg3dmap.c) */
extern int16 g_storeDefCount;    /* word_3838E — number of g_storeDefs entries */
extern int16 g_selGridX;         /* word_36F3A — last queried grid coord */
extern int16 g_selGridY;         /* word_36F3C */
extern int16 g_selTileId;        /* word_36F46 — last tile-object id + 0x100 */
extern int16 g_selStoreState;    /* word_343BE — store-selection state (-1/0) */

/* Map a grid coordinate to its g_storeDefs index: convert to world coords,
 * find the nearest tile object, recover its grid coords, then linear-search
 * the store table.  Returns the store index, 0 on a miss (caching the coords
 * and tile id), or -1 when no tile object exists at the position. */
int16 findStoreAtGrid(int16 gridX, int16 gridY) {
    int16 i;
    g_nearestTileObj = findNearestTileObject((uint32)gridX << 5, ((uint32)0x8000 - gridY) << 5);
    if (g_nearestTileObj != 0) {
        gridX = (int16)(g_nearestTileObj->x >> 5);
        gridY = (int16)(0x8000 - (g_nearestTileObj->y >> 5));
        for (i = 1; i < g_storeDefCount; i++) {
            if (g_storeDefs[i].coordX == gridX && g_storeDefs[i].coordY == gridY) {
                return i;
            }
        }
        g_selGridX = gridX;
        g_selGridY = gridY;
        g_selTileId = g_nearestTileObj->id + 0x100;
        if (g_selStoreState == 0) {
            g_selStoreState = -1;
        }
        return 0;
    }
    return -1;
}

/* ==== seg000:0xca74 ==== */
int16 computeTargetBearing(int16 targetX, int16 targetY, int16 wantBearing); /* sub_1CAB4 */
int16 bearingToStore(int16 i) {
    return computeTargetBearing(g_storeDefs[i].coordX, g_storeDefs[i].coordY, 1);
}

/* ==== seg000:0xca94 ==== */
int16 bearingToSimObject(int16 i) {
    return computeTargetBearing(g_simObjects[i].f[1], g_simObjects[i].f[2], 0);
}

/* ==== seg000:0xcab4 ==== */
int16 computeTargetBearing(int16 targetX, int16 targetY, int16 wantBearing) {
    int16 dx, dy;
    dx = g_viewX_ - targetX;
    dy = g_viewY_ - targetY;
    if (wantBearing != 0) {
        g_targetBearing = computeBearing(-dx, dy);
    }
    g_targetRange = rangeApprox(dx, dy);
    return g_targetRange;
}

/* ==== seg000:0xcaf2 ==== */
extern int16 g_ourPitch;                /* word_33572 */
extern int16 g_viewZ;                   /* word_33576 */
int16 abs(int16);
int16 hudPitchScale(void) {
    return (int16)(((int32)(0x4000 - abs(g_ourPitch)) << 12) / (uint32)(uint16)(g_viewZ + 0x1000) - 0x4000);
}

/* ==== seg000:0xcb1a ==== */
extern int8 g_airTargetMark;            /* byte_38380 */
extern int8 g_gndTargetMark;            /* byte_384E0 */
int16 isTargetOverWater(int16 wpIdx);   /* sub_1CB53 */
int16 getStoreMapCode(int16 idx) {
    if (g_storeDefs[idx].flags & 0x80) {
        return (isTargetOverWater(idx) != 0 ? g_airTargetMark : g_gndTargetMark) + 0x100;
    }
    return g_storeDefs[idx].nameIdx;
}

/* ==== seg000:0xcb53 ==== */
int16 isTargetOverWater(int16 wpIdx) {
    int16 category;

    category = ((char *)g_shapeTargetCategory)[g_planeTable.planes[wpIdx].nameIndex & 0x7f] & 0x0f;
    return (category == 12 || category == 9 || category == 11) ? 1 : 0;
}

/* ==== seg000:0xd1c8 ==== */
extern uint8 FAR g_aircraftModels[];  /* seg004:0x7530 */
extern uint8 FAR g_world3dData[];       /* seg004:0 */
extern int16 flt15_buf1[];             /* @0x6378 */
extern int16 buf3d3[];                 /* @0x602 */

int16 shapeDataOffset(int16 shapeId) {
    if (shapeId & 0x100) {
        return buf3d3[shapeId & 0x7f];
    }
    return (int16)(&g_aircraftModels[flt15_buf1[shapeId]] - g_world3dData);
}

/* ==== seg000:0xcdde ==== */
extern int16 g_targetInHudFlag;       /* word_358E0 */
extern int16 g_detailLevel;           /* word_354BC */
extern int16 g_gfxModeUnset;          /* word_2EEE6 */
extern int16 frameTick;               /* word_343B6 */
extern int16 *g_targetViewParams;     /* word_346A6 */
extern int16 g_trkRoll;               /* word_35234 */
extern int16 g_trkBearing;            /* word_3522C */
extern int16 g_trkPitch;              /* word_35232 */
extern int16 g_trkRange;              /* word_3522A */
extern int16 g_trkSize;               /* word_3522E */
extern int16 g_trkScale;              /* word_35230 */
extern int16 g_viewX_, g_viewY_;      /* word_3837C / word_3838C */
extern int8  g_extraScaleShift;       /* byte_34AC4 */
extern int16 g_ourHead;               /* word_33570 */
extern int16 g_ourRoll;               /* word_33574 */
extern int16 g_extViewPitch;          /* word_354AE */
extern uint16 FAR *g_viewParamsFar;    /* dword_354D0 */
extern int8  g_offscreenRender;       /* byte_32244 */
extern int8  g_shapeTargetCategory[]; /* dseg:0x95F0 ([bx-6A10h]) */
extern uint8 colorLut[];              /* byte_2F85B base-3 */
extern char  strBuf[];                /* 0x65E6 */
extern char  g_itoaScratch[];         /* 0x9678 */

extern int16 g_viewZ;
int16 cosMul(int16, int16);                    /* sub_1D404 */
int16 sinMul(int16, int16);                    /* sub_1D3EC */
void FAR fillSpanRect(int16 *params, int16 x0, int16 y0, int16 x1, int16 y1); /* sub_21A58 */
void setup3DTransform(int16 *p, int16 a, int16 b, int16 c, int16 d, int16 e, int16 f, int16 g); /* sub_119EA */
void rasterize3DWorld(void);                   /* sub_11A64 */
int16 FAR projectSceneObject(uint8 FAR *model, int16 yaw, int16 pitch, int16 roll, int16 relX, int16 relY, int16 flag); /* sub_20716 */

void drawTargetView(int16 shapeId, int16 worldX, int16 worldY, int16 altitude, int16 objYaw, int16 objPitch, int16 objRoll, int16 mode, int16 shift) {
    int16 unusda, horize, brgg, rngl, pdelo, catgf, ptchg, doffh, cidx, radb, bdelc, relX, relY, relZ;
    char catLowd;

    g_targetInHudFlag = 1;
    if (mode == 1 && g_detailLevel == 0 && g_gfxModeUnset != 0 && (frameTick & 3) != 0) {
        return;
    }

    doffh = shapeDataOffset(shapeId);
    if (g_drawPage == 0) {
        *g_targetViewParams = 0;
    } else {
        *g_targetViewParams = 1;
    }

    if (mode < 2) {
        g_trkRoll = 0;
        relX = worldX - g_viewX_;
        relY = worldY - g_viewY_;
        relZ = (altitude - g_viewZ) >> 5;
        brgg = computeBearing(relX, -relY);
        ptchg = computeBearing(relZ, rangeApprox(relX, relY));
        rngl = rangeApprox(relZ, rangeApprox(relX, relY));

        if (mode == 1) {
            g_trkRange = rngl;
            g_trkSize = (rngl >> 4) + 400;
            g_trkScale = (g_trkSize << 5) / (rngl + 1);
            rngl = g_trkSize << 2;
            g_trkBearing = brgg;
            g_trkPitch = ptchg;
        } else {
            g_trkScale = (g_trkRange << 5) / (rngl + 1);
            if (g_trkScale > 0x100) {
                g_trkScale = 0x100;
            }
            if (g_trkScale < 4) {
                g_trkScale = 4;
            }
            bdelc = ((brgg - g_trkBearing) >> 5) * g_trkScale;
            pdelo = ((ptchg - g_trkPitch) >> 5) * g_trkScale;
            if (abs(bdelc) > 0x1000) {
                return;
            }
            if (abs(pdelo) > 0x1000) {
                return;
            }
            brgg = (bdelc << 2) + g_trkBearing;
            ptchg = (pdelo << 2) + g_trkPitch;
            rngl = (g_trkSize << 5) / g_trkScale << 2;
        }

        radb = cosMul(ptchg, rngl);
        g_extraScaleShift = 2;
        if (shift < 0) {
            g_extraScaleShift = (uint8)(shift + 2);
            shift = 0;
        }
        relX = sinMul(brgg, radb) >> (char)shift;
        relY = -(cosMul(brgg, radb)) >> (char)shift;
        relZ = sinMul(ptchg, rngl) >> (char)shift;
    } else {
        relX = (worldX - g_viewX_) << 4;
        relY = (worldY - g_viewY_) << 4;
        relZ = (altitude - g_viewZ) >> 1;
        g_trkBearing = g_ourHead;
        g_trkPitch = g_extViewPitch;
        g_trkRoll = g_ourRoll;
        g_trkScale = 0x20;
        g_extraScaleShift = 2;
    }

    if (mode == 1 || mode == 3) {
        horize = (int16)((int32)g_trkScale * (int32)(g_trkPitch >> 2) >> 5) + 0xA0;
        if (horize < 0x7C || g_trkPitch < (int16)0xE800) {
            horize = 0x7C;
        }
        if (horize > 0xC4 || g_trkPitch > 0x1800) {
            horize = 0xC4;
        }
        g_targetViewParams[2] = colorLut[3];
        if (horize != 0x7C) {
            fillSpanRect(g_targetViewParams, 0xB0, 0x7C, 0x118, horize);
        }
        cidx = (g_viewParamsFar[0x1C] > 1) ? 2 : 6;
        catgf = (int16)(signed char)g_shapeTargetCategory[shapeId & 0x7f];
        if (catgf & 0x10) {
            cidx = 8;
        }
        catLowd = (char)(catgf & 0xf);
        if (catLowd == 12 || catLowd == 9 || catLowd == 11) {
            cidx = 1;
        }
        g_targetViewParams[2] = colorLut[cidx];
        if (horize != 0xC4) {
            fillSpanRect(g_targetViewParams, 0xB0, horize, 0x118, 0xC4);
        }
    }

    g_offscreenRender = 1;
    setup3DTransform(g_targetViewParams, -g_trkBearing, g_trkPitch, g_trkRoll, 0, 0, 0, 0);
    projectSceneObject(g_world3dData + doffh, -objYaw, objPitch, objRoll, relX, -relY, relZ);
    rasterize3DWorld();
    g_offscreenRender = 0;

    if (mode == 1) {
        strcpy(strBuf, "BRG ");
        strcat(strBuf, itoa((uint16)g_trkBearing / 0xB6, g_itoaScratch, 10));
        drawStringActivePage(strBuf, 0xF6, 0xBC, 0xF);
    }
    g_extraScaleShift = 0;
}
