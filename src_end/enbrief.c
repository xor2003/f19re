/* END.EXE — debrief/map routines (f15se2 enbrief.c lineage) */
#include "inttype.h"

/* MenuItem fields as END.EXE addresses them */
typedef struct {
    int16 pad00[4];         /* 0x00 */
    int16 colorX1;          /* 0x08 */
    int16 colorY1;          /* 0x0a */
    int16 colorX2;          /* 0x0c */
    int16 colorY2;          /* 0x0e */
    int16 pad10;            /* 0x10 */
    int16 colorPair;        /* 0x12 */
    int16 pad14[13];        /* 0x14 */
    int16 state;            /* 0x2e */
} MenuItem;

#pragma pack(1)
typedef struct {
    int16 target1Type[2];     /* 0x00 */
    int16 waypointData;       /* 0x04 */
    int16 pad06;              /* 0x06 */
    int16 target1MiscBits[5]; /* 0x08 */
    int16 target2Type[4];     /* 0x12 */
    int16 target2MiscBits[5]; /* 0x1a */
} TargetBlock;
#pragma pack()

int16 mapViewX1, mapViewX2, mapViewY1, mapViewY2;
int16 clipMaxX, clipMaxY;
int16 lineX1, lineY1, lineX2, lineY2;
int16 nightMission;
TargetBlock targetBlock;

extern void far gfx_switchColor(int16 *page, int16 x1, int16 y1, int16 x2, int16 y2, int16 fromColor, int16 toColor);
extern int16 far gfx_calcRowAddr(int16 x, int16 y);
extern void far gfx_setBlitOffset(int16 v);
extern void far gfx_setOvlVal1(int16 v);
extern void far gfx_setOvlVal2(int16 v);
extern void far gfx_nop23(void);
extern void drawLineWrapper(void);
extern int16 mapToScreenX(int16 v);
extern int16 mapToScreenY(int16 v);
extern void drawMapPixel(int16 x, int16 y, int16 color);
extern void mystrcpy(char *dst, const char *src);

void blinkWidget(MenuItem *item, int16 *gfxPage) {
    int16 toColor;
    int16 fromColor;
    if (item->state == 0) {
        item->state = 1;
        fromColor = (uint16)item->colorPair >> 4;
        toColor = item->colorPair & 0xF;
        if (item->colorPair != 0) {
            gfx_switchColor(gfxPage, item->colorX1, item->colorY1, item->colorX2, item->colorY2, fromColor, toColor);
        }
    } else {
        item->state = 0;
        fromColor = item->colorPair & 0xF;
        toColor = (uint16)item->colorPair >> 4;
    }
    if (item->colorPair != 0) {
        gfx_switchColor(gfxPage, item->colorX1, item->colorY1, item->colorX2, item->colorY2, fromColor, toColor);
    }
}

void plotMapPoint(int16 x, int16 y, int16 color, int16 unused) {
    int16 sx, sy;
    (void)unused;
    sx = mapToScreenX(x);
    sy = mapToScreenY(y);
    if (color != -1 &&
        (uint16)sx >= (uint16)mapViewX1 &&
        (uint16)sx < (uint16)mapViewX2 &&
        (uint16)sy >= (uint16)mapViewY1 &&
        (uint16)sy < (uint16)mapViewY2) {
        drawMapPixel(sx, sy, color);
    }
}

void drawClippedLineEx(int16 x1, int16 y1, int16 x2, int16 y2, int16 cx1, int16 cy1, int16 cx2, int16 cy2, int16 flag) {
    int16 w, h;
    (void)flag;
    w = cy1 - cx1;
    h = cy2 - cx2;
    gfx_setBlitOffset(gfx_calcRowAddr(cx1, cx2));
    clipMaxX = w - 1;
    clipMaxY = h - 1;
    gfx_setOvlVal1(clipMaxY);
    gfx_setOvlVal2(clipMaxX);
    lineX1 = x1;
    lineY1 = y1;
    lineX2 = x2;
    lineY2 = y2;
    drawLineWrapper();
    gfx_nop23();
    clipMaxX = 319;
    clipMaxY = 199;
    gfx_setOvlVal1(199);
    gfx_setOvlVal2(clipMaxX);
    gfx_setBlitOffset(0);
}

void drawClippedLine(int16 x1, int16 y1, int16 x2, int16 y2) {
    drawClippedLineEx(x1, y1, x2, y2, mapViewX1, mapViewX2, mapViewY1, mapViewY2, 1);
}

void drawFlightLine(int16 p1, int16 p2, int16 p3, int16 p4) {
    drawClippedLineEx(mapToScreenX(p1), mapToScreenY(p2), mapToScreenX(p3), mapToScreenY(p4), mapViewX1, mapViewX2, mapViewY1, mapViewY2, 1);
}

char *formatFlightTime(int16 timeValue, char *buffer) {
    int16 hours, miscBits, minutes, seconds;

    miscBits = targetBlock.target1MiscBits[0] + targetBlock.target2MiscBits[0];
    nightMission = ((char)miscBits & 3) == 0;
    if (targetBlock.target1Type[0] == 1 || targetBlock.target2Type[0] == 1) {
        nightMission = 0;
    }
    if (targetBlock.target1Type[0] == 4 || targetBlock.target2Type[0] == 4) {
        nightMission = 1;
    }
    timeValue += (miscBits & 0xF) << 8;
    mystrcpy(buffer, "00:00:00");
    hours = (uint16)timeValue / 1800;
    buffer[0] += nightMission + 1;
    buffer[1] += hours % 10;
    minutes = ((uint16)timeValue / 30) % 60;
    buffer[3] += minutes / 10;
    buffer[4] += minutes % 10;
    seconds = ((uint16)timeValue * 2) % 60;
    buffer[6] += seconds / 10;
    buffer[7] += seconds % 10;
    return buffer;
}
