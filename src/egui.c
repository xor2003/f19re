/* seg000 routines — UI/pic helpers (ported, verified vs original) */
#include <string.h>
#include "inttype.h"

/* TODO: real decls come from generated EGAME.EXE.h; these stand-ins keep the
 * module self-contained for portcheck. */
extern char colorLut[];
extern char g_colorPalettes[];
extern int16 g_mapX;            /* word_32A0C-2 … display point regs */
extern int16 g_mapY;
extern int16 g_clipMinX;
extern int16 g_clipMinY;
extern int16 g_drawColor;
extern int16 g_vtxX;
extern int16 g_vtxY;

void setDrawColor(int16 color);
void drawLine(int16 x1, int16 y1, int16 x2, int16 y2);
int16 openFile(const char *path, int16 mode);
void picBlit(int16 handle, int16 page, int16 mode);
void closeFile(int16 handle);

/* ==== seg000:0x0504 ==== */
void loadColorPalette(int16 idx) {
    memcpy(colorLut, g_colorPalettes + idx * 16, 16);
}

/* ==== seg000:0x978c ==== */
void drawMapMarkerBox(int16 unused1, int16 unused2, int16 color) {
    setDrawColor(color);
    drawLine(g_mapX - 4, g_mapY - 3, g_mapX + 4, g_mapY - 3);
    drawLine(g_mapX + 4, g_mapY - 3, g_mapX + 4, g_mapY + 3);
    drawLine(g_mapX + 4, g_mapY + 3, g_mapX - 4, g_mapY + 3);
    drawLine(g_mapX - 4, g_mapY + 3, g_mapX - 4, g_mapY - 3);
}

/* seg000:0xe40a openBlitClosePic — stays asm (0xE1xx pic/file cluster) */

extern int16 g_radarScopeRange;  /* byte at word_346E6 */
extern int16 g_viewX_;           /* word_3837C */
extern int16 g_viewY_;           /* word_3838C */
extern int16 g_projDepth;        /* word_384D0 */
extern int16 g_ourHead;          /* word_33570 */
extern int16 g_vprojX;           /* word_2FF24 */
extern int16 g_vprojY;           /* word_30108 */

int16 sinMul(int16 a, int16 b);    /* sub_1D3EC */
int16 cosMul(int16 a, int16 b);    /* sub_1D404 */

/* ==== seg000:0x9810 ==== */
void projectMapPoint(int16 mapX, int16 mapY) {
    int16 scaledX, scaledY;
    char shift;
    g_projDepth = 0;
    shift = 7 - (char)g_radarScopeRange;
    scaledX = (mapX - g_viewX_) >> shift;
    scaledY = (g_viewY_ - mapY) >> shift;
    g_vprojX = cosMul(g_ourHead, scaledX) - sinMul(g_ourHead, scaledY);
    g_vprojY = cosMul(g_ourHead, scaledY) + sinMul(g_ourHead, scaledX);
    g_vprojX += 0x5C;
    g_vprojY = -g_vprojY + 0xAC;
    if (g_vprojX < 0x2C || g_vprojX > 0x8C)
        g_projDepth = -1;
    if (g_vprojY < 0x7F || g_vprojY > 0xC0)
        g_projDepth = -1;
}

/* sprite/param block for blitGaugeSprite — dseg 0x2828 */
struct GaugeParams {
    int16 bufPtr, srcX, srcY, page, dstX, dstY, width, height;
};
extern struct GaugeParams gaugeSpriteParams;
extern int16 gfxBufPtr;          /* word_38D1C */
extern uint8 g_drawPage;         /* byte at word_346A8+ */

void far gfx_blitSpriteClipped(int16 *params);

/* ==== seg000:0x98bc ==== */
void blitGaugeSprite(int16 srcCol, int16 srcRow, int16 destX, int16 destY) {
    gaugeSpriteParams.bufPtr = gfxBufPtr;
    gaugeSpriteParams.srcX = srcCol * 8 + 1;
    gaugeSpriteParams.srcY = srcRow * 8 + 0x29;
    gaugeSpriteParams.page = (g_drawPage != 0);
    gaugeSpriteParams.dstX = destX - 3;
    gaugeSpriteParams.dstY = destY - 3;
    gaugeSpriteParams.width = 7;
    gaugeSpriteParams.height = 7;
    gfx_blitSpriteClipped((int16 *)&gaugeSpriteParams);
}

extern int16 g_lineX1, g_lineX2, g_lineY1, g_lineY2;
extern int16 g_viewCenterX, g_viewCenterY;
extern char far *g_modelStreamPtr;  /* dword_2F8F8 */
void far gfx_setColor(uint8 color); /* far: seg002 callee */
void far drawClipLineGlobal(void);

/* ==== seg000:0x1802 ==== */
void drawModelPoint(int16 x, int16 y) {
    g_lineX2 = g_lineX1 = x + g_viewCenterX;
    g_lineY2 = g_lineY1 = -y + g_viewCenterY;
    ++g_modelStreamPtr;
    gfx_setColor((uint8)*g_modelStreamPtr++);
    drawClipLineGlobal();
}

/* viewport parms struct — word_34646 points at it */
struct VpParms { int16 f[11]; };
extern struct VpParms *g_vpParms;   /* word_34646 */
extern int16 g_clipMaxX;            /* word_32A03 */
extern int16 g_clipMaxY;            /* word_32A05 */

int16 far gfx_calcRowAddr(int16 a, int16 b);  /* sub_2F160 */
void far gfx_setBlitOffset(int16 a);          /* sub_2F0AC */
void far gfx_nop23(void);                     /* sub_2F0D9 */

/* ==== seg000:0x8d9a ==== */
void drawViewportLine(int16 x1, int16 y1, int16 x2, int16 y2) {
    int16 height, width;

    width = g_vpParms->f[10] - g_vpParms->f[9] + 1;
    height = g_vpParms->f[8] - g_vpParms->f[7] + 1;
    gfx_setBlitOffset(gfx_calcRowAddr(g_vpParms->f[9], g_vpParms->f[7]));
    g_clipMaxX = width - 1;
    g_clipMaxY = height - 1;
    gfx_setColor(g_vpParms->f[2]);
    g_lineX1 = x1;
    g_lineY1 = y1;
    g_lineX2 = x2;
    g_lineY2 = y2;
    drawClipLineGlobal();
    gfx_nop23();
}
