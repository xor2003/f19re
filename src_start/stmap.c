/* START.EXE — tactical-map line wrapper (same role as EGAME's drawMapLine) */
#include "inttype.h"

extern void drawClippedLineEx(int16 x1, int16 y1, int16 x2, int16 y2,
                              int16 cx1, int16 cy1, int16 cx2, int16 cy2, int16 flag);

int16 mapClipX1, mapClipX2, mapClipY1, mapClipY2;  /* ds:0x653c/0x6540/0x653e/0x6542 */

/* ==== seg000:0xbe8a / 0xbe99 — world-coord → map-cell scalers ==== */
int16 mapToScreenX(int16 v) { return ((uint16)v) / 0x92; }
int16 mapToScreenY(int16 v) { return ((uint16)v) / 0xC3; }

void drawMapLine(int16 x1, int16 y1, int16 x2, int16 y2) {
    drawClippedLineEx(mapToScreenX(x1), mapToScreenY(y1), mapToScreenX(x2), mapToScreenY(y2),
                      mapClipX1, mapClipX2, mapClipY1, mapClipY2, 1);
}

/* ==== seg000:0xc058 — screen-coord line clipped to the map window ==== */
void drawClippedMapLine(int16 x1, int16 y1, int16 x2, int16 y2) {
    drawClippedLineEx(x1, y1, x2, y2, mapClipX1, mapClipX2, mapClipY1, mapClipY2, 1);
}

/* ==== seg000:0xc164 — single-point marker (color arg dropped) ==== */
void drawMapPoint(int16 x, int16 y, int16 color) {
    drawClippedMapLine(x, y, x, y);
}

/* ==== seg000:0xbea8 — project map coords; plot if inside the map window ==== */
void plotMapPoint(int16 mx, int16 my, int16 color, int16 unused) {
    int16 sx, sy;
    sx = mapToScreenX(mx);
    sy = mapToScreenY(my);
    if (color != -1 && sx > 0 && (uint16)(mapClipX2 - mapClipX1) > sx &&
        sy > 0 && (uint16)(mapClipY2 - mapClipY1) > sy)
        drawMapPoint(sx, sy, color);
}

/* ==== seg000:0xc17b / 0xc193 — fixed-point sin/cos scalers ==== */
extern int16 sine(int16 angle);           /* sub_14559 — asm interp */
extern int16 fixedMulQ14(int16 a, int16 b); /* sub_144F2 — asm */
int16 sinMul(int16 angle, int16 value) { return fixedMulQ14(sine(angle), value); }
int16 cosMul(int16 angle, int16 value) { return sinMul(angle + 0x4000, value); }

/* seg000:0x12a8 — toggles a selection rect's packed 4bpp colors
 * (flag word at +0x2e marks on/off state; hi/lo nibbles swap). */
struct SelEnt {
    int16 pad[4];
    int16 x1, y1, x2, y2;
    int16 pad10;
    uint16 packed;
    int16 pad14[13];
    int16 flag;
};
extern void far gfx_switchColor(int16 *pd, int16 x1, int16 y1, int16 x2,
                                int16 y2, int16 oldC, int16 newC); /* slot 0x29 */

void toggleSelRect(struct SelEnt *e, int16 *pd) {
    int16 a, b, c, d;   /* a@-2 = lo nibble, c@-6 = hi nibble; b,d pad frame */
    if (e->flag == 0) {
        e->flag = 1;
        c = e->packed >> 4;
        a = e->packed & 0xF;
        if (e->packed != 0)
            gfx_switchColor(pd, e->x1, e->y1, e->x2, e->y2, c, a);
    } else {
        e->flag = 0;
        c = e->packed & 0xF;
        a = e->packed >> 4;
    }
    if (e->packed != 0)
        gfx_switchColor(pd, e->x1, e->y1, e->x2, e->y2, c, a);
}

/* seg000:0x3914 — draws the unit "Risk: n" info panel.  The unit record is a
 * far pointer; four stat tables (0x06b6..0x06cc) are indexed by its fields,
 * with param k substituting one position per unit type. */
struct UnitRec38 {
    int16 pad[0x1c];
    int16 f38, f3A, f3C, f3E, f40;
};
extern struct UnitRec38 far *unitRec;         /* dseg:0x991c */
extern uint16 esTabBase;                      /* dseg:0xbb76 — holds OFFSET esTable */
extern int16 far *esTable[];                  /* dseg:0x09f2 far-ptr table */
extern int16 statT1[], statT2[], statT3[], statT4[];  /* dseg:0x06b6..0x06cc */
extern char scrStr[];                         /* dseg:0xb96a */
extern void sub_15120(char *d, char *s);      /* seg000:0x5120 mystrcpy */
extern void sub_13FB3(int16 n, char *buf);    /* seg000:0x3fb3 numToStr */
extern void sub_13B76(void *o, char *s, int16 x, int16 y); /* drawObjString */
extern void sub_14622(void *o, int16 x0, int16 y0, int16 x1, int16 y1);
extern void mystrcat(char *d, char *s);
struct ObjF04 { int16 pad[2]; int16 f04; int16 f06, f08, f0A; };

void drawRiskPanel(struct ObjF04 *o, int16 type, int16 k) {
    char tmp[10];
    uint16 n;
    esTabBase = (uint16)esTable;
    switch (type) {
    case 0:
        n = *(((int16 far **)esTabBase)[unitRec->f38])
            + statT1[unitRec->f3A] + statT2[unitRec->f3C]
            + statT3[unitRec->f3E] + statT4[unitRec->f40];
        break;
    case 1:
        n = *(((int16 far **)esTabBase)[k])
            + statT1[unitRec->f3A] + statT2[unitRec->f3C]
            + statT3[unitRec->f3E] + statT4[unitRec->f40];
        break;
    case 2:
        n = *(((int16 far **)esTabBase)[unitRec->f38])
            + statT2[unitRec->f3C] + statT3[unitRec->f3E]
            + statT4[unitRec->f40] + statT1[k];
        break;
    case 3:
        n = *(((int16 far **)esTabBase)[unitRec->f38])
            + statT1[unitRec->f3A] + statT3[unitRec->f3E]
            + statT4[unitRec->f40] + statT2[k];
        break;
    case 4:
        n = *(((int16 far **)esTabBase)[unitRec->f38])
            + statT1[unitRec->f3A] + statT2[unitRec->f3C]
            + statT4[unitRec->f40] + statT3[k];
        break;
    case 5:
        n = *(((int16 far **)esTabBase)[unitRec->f38])
            + statT1[unitRec->f3A] + statT2[unitRec->f3C]
            + statT3[unitRec->f3E] + statT4[k];
        break;
    }
    n /= 3;
    n++;
    if (n > 0xA)
        n = 0xA;
    sub_15120(scrStr, "Risk: ");
    sub_13FB3(n, tmp);
    mystrcat(scrStr, tmp);
    o->f04 = 6;
    sub_14622(o, 0xB2, 0xBE, 0xD7, 0xC5);
    sub_13B76(o, scrStr, 0xB2, 0xBE);
}

/* seg000:0x133e — rect-in-bounds test against the map window globals:
 * o->f0 <= maxX, o->f4 >= maxX is wrong; reads as point-in-rect on a
 * {x,y,x1,y1} struct: o[0]<=v1 && o[2]>=v1 && o[1]<=v2 && o[3]>=v2. */
extern uint16 word_2CA60, word_2CA64;

int16 rectInView(uint16 *o) {
    if (o[0] <= word_2CA60 && o[2] >= word_2CA60 &&
        o[1] <= word_2CA64 && o[3] >= word_2CA64)
        return 1;
    else
        return 0;
}


/* seg000:0x7072 — 32-bit shift-by-mode: v>>6/4/2, v, v<<1 — result dx:ax */
uint32 shiftByMode(int16 mode, uint32 v) {
    switch (mode) {
    case 4: return v >> 6;
    case 3: return v >> 4;
    case 2: return v >> 2;
    case 1: return v;
    case 0: return v << 1;
    }
}

/* seg000:0xd8aa — score panel: count scoreRec->f38[]==0x11, print total score */
extern struct ObjF04 *objParms;             /* dseg:0x70ac — current obj parms */
extern struct UnitRec38 far *scoreRec;      /* dseg:0xd066 */
extern char str7964[];                      /* dseg:0x7964 — score suffix */

void drawScorePanel(void) {
    char buf[20];
    int16 saveo, score3;
    uint16 n1, i6;
    objParms->f06 = 9;
    sub_14622(objParms, 0x5E, 0x6C, 0xB4, 0x72);
    objParms->f06 = 0xF;
    saveo = objParms->f04;
    objParms->f04 = 0xF;
    n1 = 0;
    for (i6 = 0; i6 < 4; i6++)
        if ((&scoreRec->f38)[i6] == 0x11) n1++;
    score3 = 0x76C * n1 + 0x2710;
    sub_15120(scrStr, "Risk: ");
    sub_13FB3(score3, buf);
    mystrcat(scrStr, buf);
    mystrcat(scrStr, str7964);
    sub_13B76(objParms, scrStr, 0x5E, 0x6C);
    objParms->f04 = saveo;
}
