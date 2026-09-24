/* seg000 routines — 3D view/terrain render (ported, verified vs original) */
#include "inttype.h"
#include "pointers.h"

extern int8 g_objShade;          /* byte_2F856 */
extern int16 g_viewCenterX;         /* word_388C6 */
extern int16 g_viewCenterY;         /* word_35454 */

void setup3DTransform(const int16 *transform, int16 a, int16 b, int16 c,
                      int16 d, int16 e, int16 f, int16 g);  /* sub_119EA */
int16 far gfx_calcRowAddr(int16 a, int16 b);               /* sub_2F160 */
void far gfx_setBlitOffset(int16 a);                       /* sub_2F0AC */
void drawMapTiles(int16 mapX, int16 mapY, int16 zoomShift);/* sub_11564 */
void rasterize3DWorld(void);                               /* sub_11A64 */
void far renderSortedListFar(void);                        /* sub_20908 */
void far gfx_setBlitOffset2(void);                         /* sub_2F0A2 */
void far gfx_nop23(void);                                  /* sub_2F0D9 */
extern int8  g_offscreenRender;                            /* byte_32244 */

/* ==== seg000:0x1516 ==== */
void renderMapTerrain(const int16 *transform, int16 mapX, int16 mapY, int16 zoomShift) {
    int16 tmp0, tmp1;
    g_objShade = 0;
    setup3DTransform(transform, 0, 0, 0, 0, 0, 0, 0);
    gfx_setBlitOffset(gfx_calcRowAddr(transform[9], transform[7]));
    drawMapTiles(mapX, mapY, zoomShift);
    rasterize3DWorld();
}

/* ==== seg000:0x1a64 ==== */
void rasterize3DWorld(void) {
    renderSortedListFar();
    gfx_setBlitOffset2();
    gfx_nop23();
    g_offscreenRender = 0;
}

/* ==== seg000:0x1a7a ==== */
extern int16 g_hudVisible;            /* word_33D90 */
extern int8  g_halfScaleRender;       /* byte_330EA */
extern int16 g_overlayCenterX;        /* word_38B08 */
extern int16 g_overlayCenterY;        /* word_38B0C */
extern int16 g_clipMaxX, g_clipMaxY;  /* word_32A03/5 */
extern void  far gfx_setOvlVal2(int16);          /* sub_2F16F */
extern int16 far gfx_calcRowAddr(int16, int16);  /* sub_2F160 */
extern void  far gfx_setBlitOffset(int16);       /* sub_2F0AC */

void setupViewport(const int16 *rect) {
    int16 wx, wy;
    wx = rect[10] - rect[9] + 1;
    wy = rect[8] - rect[7] + 1;
    g_viewCenterX = ((wx + 1) >> 1) - 1;
    g_viewCenterY = ((wy + 1) >> 1) - 1;
    if (rect[7] == 0) {
        g_viewCenterY = (char)g_hudVisible != 0 ? (g_halfScaleRender != 0 ? 82 : 56) : 100;
    }
    gfx_setOvlVal2(wx - 1);
    gfx_setBlitOffset(gfx_calcRowAddr(rect[9], rect[7]));
    g_clipMaxX = wx - 1;
    g_clipMaxY = wy - 1;
    g_overlayCenterX = 0xA28;
    g_overlayCenterY = 0xA48;
    if (g_halfScaleRender != 0) {
        g_overlayCenterX += 16;
        g_overlayCenterY += 16;
    }
    if ((char)g_hudVisible != 0)
        g_overlayCenterY += 32;
}

/* ==== seg000:0x1b32 ==== */
extern int16 g_viewRotMatrix[];                       /* dseg:0x0AC2 */
extern void far buildRotationMatrixFar(int16 *, int16, int16, int16); /* sub_211C2 */

void setViewRotation(int16 rotX, int16 rotY, int16 rotZ) {
    buildRotationMatrixFar(g_viewRotMatrix, -rotX, -rotY, -rotZ);
}

/* ==== seg000:0x19ea ==== */
extern void setViewPosition(int16, int16, int16);      /* sub_11B56 */
extern void far transformModelVerticesFar(void);       /* sub_20C6C */
extern void far drawProjectionSphere(int16);           /* sub_204FE */
extern int16 g_posVisibleFlag;        /* word_32242 */
extern int16 g_detailLevel;           /* word_354BC */
extern int8  g_frameSyncPending;      /* byte_32DA3 */
extern int16 g_sortedObjCount;        /* word_2F968 */
extern int16 g_spinAngle;             /* word_2F976 */
extern int16 g_frameRateScaling;      /* word_33D92 */

void setup3DTransform(const int16 *model, int16 angleX, int16 angleY, int16 angleZ, int16 posX, int16 posY, int16 posZ, int16 renderScene) {
    setupViewport(model);
    setViewRotation(angleX, angleY, angleZ);
    setViewPosition(posX, posY, posZ);
    if (renderScene != 0) {
        g_posVisibleFlag = 0;
        if (g_detailLevel == 0)
            g_offscreenRender = 1;
        if (g_offscreenRender == 0)
            transformModelVerticesFar();
        while (g_frameSyncPending != 0);
        drawProjectionSphere(model[2]);
    }
    g_sortedObjCount = 0;
    g_spinAngle -= 0x3000 / g_frameRateScaling;
}

/* ==== seg000:0x1b56 ==== */
extern int16 g_viewPosX, g_viewPosY, g_viewPosZ;    /* word_2F920/2/4 */

void setViewPosition(int16 x, int16 y, int16 z) {
    g_viewPosX = x;
    g_viewPosY = y;
    g_viewPosZ = z;
}
