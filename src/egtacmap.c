/* egtacmap.c — tactical map / screen drawing routines (F19) */
#include "inttype.h"
#include "pointers.h"
#include <dos.h>
#include <stdlib.h>
#include <string.h>

int16 mapXToScreen(int16 x);              /* sub_18B06 */
int16 mapYToScreen(int16 y);              /* sub_18B1D */
void drawClippedLineRegion(int16 sx1, int16 sy1, int16 sx2, int16 sy2,
                           int16 clipL, int16 clipR, int16 clipT, int16 clipB, int16 both); /* sub_18E12 */
int16 clampRange(int16 v, int16 lo, int16 hi);   /* sub_1D1FA */
int16 readScreenPixel(int16 screenX, int16 screenY); /* sub_19282 */
void FAR fillSpanRect(const int16 *dst, int16 left, int16 top, int16 right, int16 bottom); /* sub_21A58 */
void drawStringCentered(int16 *page, const char *text, int16 x, int16 y, int16 color); /* sub_19219 */
void drawViewportLine(int16 x1, int16 y1, int16 x2, int16 y2);   /* sub_18D9A */
void FAR gfx_drawString(int16 *page, const char *str, int16 len); /* sub_2F043 */
void fillPanelBox(int16 panelId, int16 color);               /* sub_190E8 */
void drawCenteredLabelBox(int16 panelId, const char *text);  /* sub_19123 */
void drawPanelText(int16 panelId, const char *text, int16 color); /* sub_190CB */
void drawStringBothPages(const char *text, int16 x, int16 y, int16 color); /* sub_191B4 */

extern int16 g_panelLabelOn;     /* word_33D90 */

extern int16 g_scopeClipLeft;    /* word_384D2 */
extern int16 g_scopeClipRight;   /* word_388C8 */
extern int16 g_scopeClipTop;     /* word_384D4 */
extern int16 g_scopeClipBottom;  /* word_38A0C */
extern int16 g_mapMode;          /* word_38504 */
extern int16 *g_pageFront;       /* word_34646 */
extern int16 *g_pageBack;        /* word_3465E */
extern int16 g_mapCenterX;       /* word_346E8 */
extern int16 g_mapCenterY;       /* word_346EA */
extern int16 g_mapZoomLevel;     /* word_346E4 */
extern union REGS regs;          /* @0x95DE */
extern uint16 FAR *g_viewParamsFar; /* dword_354D0 */
extern int8  g_halfScaleRender;  /* byte_330EA */
extern int8  g_drawPage;         /* byte_388CA */

/* ==== seg000:0x7e62 ==== */
void clearStatusPanel(void) {
    drawPanelText(2, "", 0);
}

/* ==== seg000:0x8751 ==== */
extern int16 g_scopeCenterX;     /* word_354A8 */
extern int16 g_scopeCenterY;     /* word_354AC */
void zoomIn(void);                              /* sub_18A1B */
void initTacMapView(void) {
    g_mapMode = 0;
    g_scopeClipLeft = 0x28;
    g_scopeClipRight = 0x90;
    g_scopeClipTop = 0x7C;
    g_scopeClipBottom = 0xC4;
    g_scopeCenterX = 0x68;
    g_scopeCenterY = 0x48;
    zoomIn();
}

/* ==== seg000:0x8a1b ==== */
extern int16 g_viewMode;         /* word_3836E */
extern int16 g_externalCamDist;  /* word_343C6 */
extern int16 g_radarScopeRange;  /* word_346E6 */
extern int16 g_viewX_, g_viewY_; /* word_3838C / word_3837C */
void redrawTacMap(int16 x, int16 y);            /* sub_187EC */
void zoomIn(void) {
    if (g_viewMode & 0x80) {
        g_externalCamDist--;
    } else {
        if (g_mapMode == 0 && g_mapZoomLevel < 9) {
            g_mapZoomLevel++;
            redrawTacMap(g_viewX_, g_viewY_);
        }
        if (g_mapMode == 1) {
            g_radarScopeRange++;
        }
    }
}

/* ==== seg000:0x8a54 ==== */
void zoomOut(void) {
    if (g_viewMode & 0x80) {
        g_externalCamDist++;
    } else {
        if (g_mapMode == 0 && g_mapZoomLevel > 2) {
            g_mapZoomLevel--;
            redrawTacMap(g_viewX_, g_viewY_);
        }
        if (g_mapMode == 1 && g_radarScopeRange != 0) {
            g_radarScopeRange--;
        }
    }
}

/* ==== seg000:0x8b06 ==== */
int16 mapXToScreen(int16 mapX) {
    return ((mapX - g_mapCenterX) >> (10 - (uint8)g_mapZoomLevel)) + 0x5B;
}

/* ==== seg000:0x8b1d ==== */
int16 mapYToScreen(int16 mapY) {
    return (((mapY - g_mapCenterY) >> (10 - (uint8)g_mapZoomLevel)) * 3 >> 1 >> 1) + 0x9F;
}

/* ==== seg000:0x8bea ==== */
int16 readMapPixelColor(int16 mapX, int16 mapY) {
    int16 screenX, screenY, color;
    if (g_mapMode != 0) return 0;
    screenX = mapXToScreen(mapX);
    screenY = mapYToScreen(mapY);
    screenX = clampRange(screenX, g_scopeClipLeft, g_scopeClipRight);
    screenY = clampRange(screenY, g_scopeClipTop, g_scopeClipBottom);
    color = -1;
    if (screenX > g_scopeClipLeft && screenX < g_scopeClipRight && screenY > g_scopeClipTop && screenY < g_scopeClipBottom) {
        color = readScreenPixel(screenX, screenY);
    }
    return color;
}

/* ==== seg000:0x8d2a ==== */
void drawMapLine(int16 x1, int16 y1, int16 x2, int16 y2) {
    drawClippedLineRegion(mapXToScreen(x1), mapYToScreen(y1), mapXToScreen(x2), mapYToScreen(y2), g_scopeClipLeft, g_scopeClipRight, g_scopeClipTop, g_scopeClipBottom, 1);
}

/* ==== seg000:0x8d71 ==== */
void drawFullscreenLine(int16 x1, int16 y1, int16 x2, int16 y2) {
    drawClippedLineRegion(x1, y1, x2, y2, 0, 319, 0, 199, 1);
}

/* ==== seg000:0x8f10 ==== */
void drawScreenLineOnePage(int16 x1, int16 y1, int16 x2, int16 y2) {
    drawClippedLineRegion(x1, y1, x2, y2, 0, 319, 0, 199, 0);
}

/* ==== seg000:0x8f38 ==== */
void drawHudViewLine(int16 x1, int16 y1, int16 x2, int16 y2) {
    if (g_halfScaleRender != 0) {
        if (g_viewParamsFar[0x20] < 2) {
            drawViewportLine(x1, y1, x2, y2);
        } else {
            drawClippedLineRegion(x1, y1, x2, y2, 104, 216, 62, 108, 0);
        }
    } else {
        drawClippedLineRegion(x1, y1, x2, y2, 48, 271, 15, 108, 0);
    }
}

/* ==== seg000:0x8f9c ==== */
void setDrawColor(int16 color) {
    g_pageFront[2] = color;
    g_pageBack[2] = color;
}

/* ==== seg000:0x8fb2 ==== */
void fillRectBoth(int16 x1, int16 y1, int16 x2, int16 y2) {
    fillSpanRect(g_pageFront, x1, y1, x2, y2);
    fillSpanRect(g_pageBack, x1, y1, x2, y2);
}

/* ==== seg000:0x90cb ==== */
void drawPanelText(int16 panelId, const char *text, int16 color) {
    fillPanelBox(panelId, color);
    drawCenteredLabelBox(panelId, text);
}

/* ==== seg000:0x90e8 ==== */
void fillPanelBox(int16 panelId, int16 color) {
    setDrawColor(color);
    if (panelId == 1) {
        fillRectBoth(0x28, 0x7C, 0x8F, 0xC3);
    } else {
        fillRectBoth(0xB0, 0x7C, 0x118, 0xC4);
    }
}

/* ==== seg000:0x9123 ==== */
void drawCenteredLabelBox(int16 panelId, const char *text) {
    int16 xl, y, xr;
    if (strlen(text) == 0 || g_panelLabelOn == 0) {
        return;
    }
    if (panelId == 1) {
        xl = 0x28;
        xr = 0x8F;
    } else {
        xl = 0xB0;
        xr = 0x118;
    }
    y = 0x7C;
    y -= 8;
    setDrawColor(8);
    fillRectBoth(xl + 5, y, xr - 5, y + 4);
    drawStringBothPages(text, ((xl + xr) >> 1) - (strlen(text) << 1), y, 0xB);
}

/* ==== seg000:0x91b4 ==== */
void drawStringBothPages(const char *text, int16 screenX, int16 screenY, int16 color) {
    drawStringCentered(g_pageFront, text, screenX, screenY, color);
    drawStringCentered(g_pageBack, text, screenX, screenY, color);
}

/* ==== seg000:0x91e5 ==== */
void drawStringActivePage(const char *text, int16 screenX, int16 screenY, int16 color) {
    if (g_drawPage == 0) {
        drawStringCentered(g_pageFront, text, screenX, screenY, color);
    } else {
        drawStringCentered(g_pageBack, text, screenX, screenY, color);
    }
}

/* ==== seg000:0x9219 ==== */
void drawStringCentered(int16 *strStruct, const char *text, int16 screenX, int16 screenY, int16 color) {
    strStruct[6] = 0;
    strStruct[4] = screenX;
    strStruct[5] = screenY;
    strStruct[2] = color;
    gfx_drawString(strStruct, strupr((char *)text), strlen(text));
}

/* ==== seg000:0x9257 ==== */
void drawNumber(int16 value, int16 x, int16 y, int16 color) {
    char buf[20];
    itoa(value, buf, 10);
    drawStringBothPages(buf, x, y, color);
}

/* ==== seg000:0x9282 ==== */
int16 readScreenPixel(int16 screenX, int16 screenY) {
    regs.h.ah = 0x0D;
    regs.x.cx = screenX;
    regs.x.dx = screenY;
    regs.h.bh = 0;
    int86(0x10, &regs, &regs);
    return regs.h.al;
}

/* ==== seg000:0x92b1 ==== */
extern int16 g_frameRateScaling;    /* word_33D92 */
extern int16 g_hudMsgTimer;         /* word_346EC */
extern char  g_hudMessageBuf[];     /* @dseg:958C */
void hudMessage(const char *src) {
    strcpy(g_hudMessageBuf, src);
    g_hudMsgTimer = g_frameRateScaling * 3;
}

/* ==== seg000:0xa23f / 0xa26c / 0xa2c5 ==== */

extern int16 *g_pageOffscreen;   /* word_34676 */
void FAR gfx_copyRect(int16 src, int16 sx, int16 sy, int16 dst, int16 dx, int16 dy, int16 w, int16 h); /* sub_2F0FC */

void cacheScopePanel(void) {
    gfx_copyRect(*g_pageFront, 0x28, 0x7C, *g_pageOffscreen, 0x28, 0x7C, 0x69, 0x49);
}

void restoreScopePanel(void) {
    gfx_copyRect(*g_pageOffscreen, 0x28, 0x7C, *g_pageFront, 0x28, 0x7C, 0x69, 0x49);
    gfx_copyRect(*g_pageFront, 0x28, 0x7C, *g_pageBack, 0x28, 0x7C, 0x69, 0x49);
}

void captureScopePanel(void) {
    gfx_copyRect(*g_pageOffscreen, 0x28, 0x7C, g_drawPage ? *g_pageBack : *g_pageFront, 0x28, 0x7C, 0x69, 0x49);
}
