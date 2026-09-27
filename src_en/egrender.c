/* seg000 routines — 3D frame orchestrator (ported, verified vs original) */
/* Original module compiled /Ot /Oa: g_viewParams stays cached in bx across
   the consecutive pointer stores at the top of render3DView. */
#include "inttype.h"
#include "pointers.h"

void setup3DTransform(const int16 *transform, int16 a, int16 b, int16 c,
                      int16 d, int16 e, int16 f, int16 g);  /* sub_119EA */
void rasterize3DWorld(void);                               /* sub_11A64 */

/* ==== seg000:0x044a ==== */
extern int16 *g_viewParams;              /* word_2F268 — ptr to the transform/clip record */
extern int16 g_frameSyncWait;            /* word_34AFE */
extern const uint8 colorLut[];           /* byte table @0x9E8 — color index LUT */
extern int16 g_skyColorIndex;            /* word_38374 — used as byte */
extern int8  g_renderPageToggle;         /* byte_2F26A */
uint8 far gfx_getDrawPage(void);         /* sub_2F10B */
void waitFrameSync(int16);               /* sub_104E2 */
void projectObjects(int16, int16, int32, int32, int32); /* sub_10522 */
void updateTargetingHud(void);           /* sub_1AC4C */
void drawHudWorldOverlay(void);          /* sub_1B2CA */

void render3DView(int16 camX, int16 camY, int16 camZ, int32 worldX, int32 worldY, int32 worldZ,
                  int16 clipLeft, int16 clipTop, int16 clipWidth, int16 clipHeight) {
    g_viewParams[7] = clipTop;
    g_viewParams[8] = clipTop + clipHeight - 1;
    g_viewParams[9] = clipLeft;
    g_viewParams[10] = clipLeft + clipWidth - 1;
    *g_viewParams = gfx_getDrawPage() & 0xFF;
    waitFrameSync(g_frameSyncWait);
    g_viewParams[2] = (uint8)colorLut[g_skyColorIndex & 0xFF];
    setup3DTransform(g_viewParams, camX, camY, camZ, 0, 0, (int16)worldZ, 1);
    projectObjects(camX, camY, worldX, worldY, worldZ);
    updateTargetingHud();
    rasterize3DWorld();
    drawHudWorldOverlay();
    g_renderPageToggle ^= 1;
}

/* ==== seg000:0x04e2 ==== */
extern int8 g_timerTick;                 /* byte_32DD2 — incremented by the timer ISR */

void waitFrameSync(int16 frames) {
    uint8 targetTick;
    if (frames > 0) {
        targetTick = (uint8)frames + g_timerTick;
        while (g_timerTick != targetTick) ;
    }
}
