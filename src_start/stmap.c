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

/* ==== seg000:0x0924 drawUnitList — per-row unit list renderer.
 * Iterates `count` TileEntry rows; state 2 = selected row (toggle rect,
 * sub_12754/sub_13218 redraws, icon + optional name text via the near/far
 * word-wrap pair), state 3 kept, others cleared. ==== */
extern int16 selRowIdx;                            /* dseg:0xc7cc */
extern int16 flag2CA4C;                            /* dseg:0xca4c */
extern uint8 byte_2C9E0;                           /* dseg:0xc9e0 */
extern int16 flag2C7CE;                            /* dseg:0xc7ce */
extern int16 selAvailTab[];                        /* dseg:0x9924 */
extern char *namePtrTab[];                         /* dseg:0x717e */
extern int16 typeIdxTab[];                         /* dseg:0xd2d0 */
extern char *typeNameTab[];                        /* dseg:0x0256 */
extern char  strB96A[];                            /* dseg:0xb96a */
extern char  str282[];                             /* dseg:0x0282 */
struct TileEntry {
    int16 pad[0x10];
    int16 x0, y0, x1, y1;
    int16 spr1, spr2;
    int16 f2C, f2E;
    int8  flag;
    int8  pad31;
};
struct ObjF06;
extern int16 flag_29948;                           /* dseg:0x9948 */
extern void drawTileIcon(struct TileEntry *t, uint16 idx, struct ObjF06 *o);
extern void sub_12754(struct TileEntry *t, int16 i, int16 *pd);
extern void sub_13218(struct TileEntry *t, int16 i, int16 *pd);
extern void wrapUnitText(int16 a, char *s, uint16 w, int16 x, int16 y,
                         int16 b);
extern void wrapUnitTextFar(int16 a, char far *s, uint16 w, int16 x, int16 y,
                            int16 b);
extern void mystrcpy(char *d, const char *s);
extern void mystrcat(char *d, char *s);

void drawUnitList(struct TileEntry *tab, char far **names, int16 count,
                  int16 outA, int16 outB, int16 *pd) {
    char a[2], b[2], d[2], k[2];
    int16 i, c, j;
    char buf[0x16];
    a[0] = 0x0D; a[1] = 0;
    d[0] = 0x89; d[1] = 0;
    b[0] = 0x8D; b[1] = 0;
    k[0] = 0x80; k[1] = 0;
    for (i = 0; i < count; i++) {
        if (tab[i].f2E == 2) {
            selRowIdx = i;
            tab[i].f2E = 0;
            if (flag2CA4C != 1 || byte_2C9E0 != 0)
                toggleSelRect((struct SelEnt *)&tab[i], pd);
            sub_12754(tab, i, pd);
            sub_13218(tab, i, pd);
            drawTileIcon(tab, i, (struct ObjF06 *)pd);
            if (tab[i].flag & 0x40) {
                sub_14622(pd, tab[i].pad[0xA], tab[i].pad[0xB],
                          tab[i].pad[0xC], tab[i].pad[0xD]);
                if (flag2C7CE == 1 && selAvailTab[i] == 1) {
                    mystrcpy(strB96A, str282);
                    mystrcat(strB96A, namePtrTab[i]);
                    mystrcat(strB96A, typeNameTab[typeIdxTab[i]]);
                    wrapUnitText(tab[i].pad[0xF], strB96A,
                                 tab[i].pad[0xC] - tab[i].pad[0xA] - 1,
                                 tab[i].pad[0xA], tab[i].pad[0xB],
                                 tab[i].pad[0xE]);
                } else {
                    wrapUnitTextFar(tab[i].pad[0xF], names[i],
                                    tab[i].pad[0xC] - tab[i].pad[0xA] - 1,
                                    tab[i].pad[0xA], tab[i].pad[0xB],
                                    tab[i].pad[0xE]);
                }
            }
        } else if (tab[i].f2E != 3) {
            tab[i].f2E = 0;
        }
    }
    if (flag_29948 == 1)
        drawRiskPanel((struct ObjF04 *)pd, 0, 0);
    word_2CA60 = outA;
    word_2CA64 = outB;
}


/* ==== seg000:0x0AE8 sub_10AE8 — interactive row-select widget.
 * Companion to drawUnitList: after it renders the rows, this polls input
 * (ovlCall_c8a + sub_11366), repaints selection panels, and returns the
 * picked row (OR'd with 0x200 when byte_27E5A is set). ==== */
struct SelRow {
    int16 pad[4];                /* 0x00..0x07 hit-test rect */
    int16 rx0, ry0, rx1, ry1;    /* 0x08..0x0e inner rect */
    int16 mode;                  /* 0x10 */
    int16 pad2;                  /* 0x12 */
    int16 x0, y0, x1, y1;        /* 0x14..0x1a text rect */
    int16 spr1, spr2;            /* 0x1c,0x1e */
    int16 pad3[4];               /* 0x20..0x27 */
    int16 *sprA, *sprB;          /* 0x28,0x2a blink-phase sprite records */
    int16 f2C, f2E;              /* 0x2c group tag / 0x2e state */
    union { int16 w; int8 b; } flag;  /* 0x30 */
};
extern void sub_11366(int16 *view, struct SelRow *r, int16 *pd);
extern void drawLine(int16 x0, int16 y0, int16 x1, int16 y1, int16 c);
extern void far ovlCall_b9f(int16 v);
extern void far ovlCall_c8a(void);
extern void far ovlCall_bc7(int16 *pd, int16 x0, int16 y0, int16 x1,
                            int16 y1, int16 m, int16 n);
extern int16 word_27E56;
extern uint8 byte_27E52, byte_27E58, byte_27E5A;
extern uint8 byte_2C970, byte_298F7;

int16 sub_10AE8(struct SelRow *tab, char far **names, int16 count,
                int16 a4, int16 *pd, int16 a6) {
    char a[2];  int16 b;    int16 c;
    char d[2];  int16 e;    char f[2];
    int16 g;    int16 h;    char i[2];
    int16 j;    int8  typ;  char  buf[0x16];

    a[0] = 0x0D; a[1] = 0;
    f[0] = 0x89; f[1] = 0;
    d[0] = 0x8D; d[1] = 0;
    i[0] = 0x80; i[1] = 0;
    ovlCall_c8a();
    word_27E56 = 0;
    j = 0;
    goto L1t;
L1b:
    if (j >= count)
        goto L1d;
    j++;
L1t:
    if (!rectInView((uint16 *)&tab[j]))
        goto L1b;
L1d:
    byte_27E58 = 0;
    for (;;) {
        ovlCall_c8a();
        if (!(tab[j].flag.w & 0x100))
            word_27E56 = 1;
        sub_11366((int16 *)a4, &tab[j], pd);
        if (byte_27E52 == 0 && byte_2C970 == 0)
            continue;
        if (byte_2C970 != 0) {
            if (j != selRowIdx) {
                j = 0;
                goto L2t;
L2b:
                if (j >= count)
                    goto L2d;
                j++;
L2t:
                if (!rectInView((uint16 *)&tab[j]))
                    goto L2b;
L2d: ;
            }
            if (tab[selRowIdx].mode == 0) {
                c = 0xB; b = 9;
                ovlCall_bc7(pd, tab[selRowIdx].rx0, tab[selRowIdx].ry0,
                            tab[selRowIdx].rx1, tab[selRowIdx].ry1, c, b);
                c = 3;
                ovlCall_bc7(pd, tab[selRowIdx].rx0, tab[selRowIdx].ry0,
                            tab[selRowIdx].rx1, tab[selRowIdx].ry1, c, b);
                c = 0xD;
                ovlCall_bc7(pd, tab[selRowIdx].rx0, tab[selRowIdx].ry0,
                            tab[selRowIdx].rx1, tab[selRowIdx].ry1, c, b);
            }
            if (flag2CA4C == 1 && byte_2C9E0 == 1) {
                c = 0xB; b = 5;
                ovlCall_bc7(pd, tab[selRowIdx].rx0, tab[selRowIdx].ry0,
                            tab[selRowIdx].rx1, tab[selRowIdx].ry1, c, b);
                c = 3;
                ovlCall_bc7(pd, tab[selRowIdx].rx0, tab[selRowIdx].ry0,
                            tab[selRowIdx].rx1, tab[selRowIdx].ry1, c, b);
                c = 0xD;
                ovlCall_bc7(pd, tab[selRowIdx].rx0, tab[selRowIdx].ry0,
                            tab[selRowIdx].rx1, tab[selRowIdx].ry1, c, b);
                c = 9;
                ovlCall_bc7(pd, tab[selRowIdx].rx0, tab[selRowIdx].ry0,
                            tab[selRowIdx].rx1, tab[selRowIdx].ry1, c, b);
            } else if (flag2CA4C == 1 && byte_2C9E0 == 0) {
                ovlCall_b9f(0xF);
                drawLine(tab[selRowIdx].rx0 - 1, tab[selRowIdx].ry0 - 1,
                         tab[selRowIdx].rx1 + 1, tab[selRowIdx].ry0 - 1, 0xF);
                drawLine(tab[selRowIdx].rx1 + 1, tab[selRowIdx].ry0 - 1,
                         tab[selRowIdx].rx1 + 1, tab[selRowIdx].ry1 + 1, 0xF);
                drawLine(tab[selRowIdx].rx1 + 1, tab[selRowIdx].ry1 + 1,
                         tab[selRowIdx].rx0 - 1, tab[selRowIdx].ry1 + 1, 0xF);
                drawLine(tab[selRowIdx].rx0 - 1, tab[selRowIdx].ry1 + 1,
                         tab[selRowIdx].rx0 - 1, tab[selRowIdx].ry0, 0xF);
            }
            if (byte_27E5A == 1)
                ((int8 *)&j)[1] |= 2;
            return j;
        }
        j = 0;
        goto L3t;
L3b:
        if (j >= count)
            goto L3d;
        j++;
L3t:
        if (!rectInView((uint16 *)&tab[j]))
            goto L3b;
L3d:
        if (j == selRowIdx) {
            typ = tab[j].flag.b & 7;
            if (typ != 2 && typ != 3 && typ != 4)
                continue;
        }
        if (flag_29948 == 1)
            drawRiskPanel((struct ObjF04 *)pd, a6, j);
        if (tab[j].flag.b & 8) {
            for (g = 0; g < count; g++) {
                if (tab[g].f2E != 0 && tab[g].f2C == tab[j].f2C)
                    toggleSelRect((struct SelEnt *)&tab[g], pd);
            }
            if (tab[selRowIdx].mode == 0 && flag2CA4C == 0) {
                c = 9; b = 6;
                ovlCall_bc7(pd, tab[selRowIdx].rx0, tab[selRowIdx].ry0,
                            tab[selRowIdx].rx1, tab[selRowIdx].ry1, c, b);
                c = 3;
                ovlCall_bc7(pd, tab[selRowIdx].rx0, tab[selRowIdx].ry0,
                            tab[selRowIdx].rx1, tab[selRowIdx].ry1, c, b);
                c = 0xD;
                ovlCall_bc7(pd, tab[selRowIdx].rx0, tab[selRowIdx].ry0,
                            tab[selRowIdx].rx1, tab[selRowIdx].ry1, c, b);
                c = 0xB;
                ovlCall_bc7(pd, tab[selRowIdx].rx0, tab[selRowIdx].ry0,
                            tab[selRowIdx].rx1, tab[selRowIdx].ry1, c, b);
            }
            if (tab[selRowIdx].mode == 1) {
                c = 8; b = 7;
                ovlCall_bc7(pd, tab[selRowIdx].rx0, tab[selRowIdx].ry0,
                            tab[selRowIdx].rx1, tab[selRowIdx].ry1, c, b);
            }
            if (flag2CA4C == 1 && byte_2C9E0 == 1) {
                c = 9; b = 5;
                ovlCall_bc7(pd, tab[selRowIdx].rx0, tab[selRowIdx].ry0,
                            tab[selRowIdx].rx1, tab[selRowIdx].ry1, c, b);
                c = 3;
                ovlCall_bc7(pd, tab[selRowIdx].rx0, tab[selRowIdx].ry0,
                            tab[selRowIdx].rx1, tab[selRowIdx].ry1, c, b);
                c = 0xD;
                ovlCall_bc7(pd, tab[selRowIdx].rx0, tab[selRowIdx].ry0,
                            tab[selRowIdx].rx1, tab[selRowIdx].ry1, c, b);
                c = 0xB;
                ovlCall_bc7(pd, tab[selRowIdx].rx0, tab[selRowIdx].ry0,
                            tab[selRowIdx].rx1, tab[selRowIdx].ry1, c, b);
            } else if (flag2CA4C == 1 && byte_2C9E0 == 0) {
                ovlCall_b9f(0xF);
                drawLine(tab[selRowIdx].rx0 - 1, tab[selRowIdx].ry0 - 1,
                         tab[selRowIdx].rx1 + 1, tab[selRowIdx].ry0 - 1, 0xF);
                drawLine(tab[selRowIdx].rx1 + 1, tab[selRowIdx].ry0 - 1,
                         tab[selRowIdx].rx1 + 1, tab[selRowIdx].ry1 + 1, 0xF);
                drawLine(tab[selRowIdx].rx1 + 1, tab[selRowIdx].ry1 + 1,
                         tab[selRowIdx].rx0 - 1, tab[selRowIdx].ry1 + 1, 0xF);
                drawLine(tab[selRowIdx].rx0 - 1, tab[selRowIdx].ry1 + 1,
                         tab[selRowIdx].rx0 - 1, tab[selRowIdx].ry0, 0xF);
            }
            if (flag2CA4C != 1 || byte_2C9E0 != 0)
                toggleSelRect((struct SelEnt *)&tab[j], pd);
        }
        selRowIdx = j;
        if (tab[j].flag.b & 0x40) {
            sub_14622(pd, tab[j].x0, tab[j].y0, tab[j].x1, tab[j].y1);
            if (flag2C7CE == 1 && selAvailTab[j] == 1) {
                sub_15120(strB96A, typeNameTab[typeIdxTab[j]]);
                wrapUnitText(tab[j].spr2, strB96A,
                             tab[j].x1 - tab[j].x0 - 1, tab[j].x0,
                             tab[j].y0 + (j == 0xC ? 0x28 : 0), tab[j].spr1);
            } else if (flag2C7CE == 1 && byte_298F7 == 1 && j == 0x10) {
                wrapUnitTextFar(tab[j].spr2, names[0x13],
                                tab[j].x1 - tab[j].x0 - 1, tab[j].x0,
                                tab[j].y0, tab[j].spr1);
            } else {
                wrapUnitTextFar(tab[j].spr2, names[j],
                                tab[j].x1 - tab[j].x0 - 1, tab[j].x0,
                                tab[j].y0, tab[j].spr1);
            }
        }
        drawTileIcon((struct TileEntry *)tab, j, (struct ObjF06 *)pd);
        sub_13218((struct TileEntry *)tab, j, pd);
        sub_12754((struct TileEntry *)tab, j, pd);
    }
}


/* ==== seg000:0x1366 sub_11366 — row-select input/animation pump.
 * Polls keyboard + joystick while blinking the selected row (palette-cycle
 * or border flash by flag2CA4C/byte_2C9E0), animates per-row-type markers
 * (unit icon, threat ring, site radius, tile radar sweep, waypoint labels),
 * then dispatches ENTER/ESC/arrow keys (scrolling the view window via
 * word_2CA60/64 or cycling object selection) and redraws the markers. ==== */
typedef struct {                            /* worldObjects: stride 0x10 */
    int16 x_coord, y_coord;                 /* 0x00, 0x02 */
    int16 pad4;                             /* 0x04 */
    int16 targetFlags;                      /* 0x06 */
    int16 pad8[4];                          /* 0x08 */
} WorldObject;
extern WorldObject worldObjects[];          /* dseg:0xb390 */
typedef struct { int16 f0, f1;              /* ringTypes: dseg:0x3e26 stride 0xe */
                 uint8 flag, padT[9]; } RingType;
extern RingType ringTypes[];
extern int16   siteTypeParms[];             /* dseg:0x41c8, stride 0x12 */
extern int8    tileMarkMap[];               /* dseg:0xb842, 16x16 bit 0x10 */
extern int8    objectActive[];              /* dseg:0x2d278 */
extern int16   word_2C968, word_2C144;      /* obj/sel cursor indices */
extern uint8   unitMarksOn, tileMarksOn, siteMarksOn, ringMode;
extern uint8   byte_216AA, byte_216AB;      /* joystick axis centers */
extern uint8   byte_27E59;                  /* blink phase flag */
extern uint8   byte_20A1A, byte_20A1D;      /* dseg:0xa1a/0xa1d timers */
extern uint8   blinkTimer;                  /* dseg:0xa1c */
extern uint8   byte_212BA, byte_2D06B;      /* cbreak / exit flags */
extern int16   word_2B38A;                  /* roster-active flag */
extern int16   flag_29948;                  /* rtc-enabled flag */
extern uint16 *word_27E50;                  /* cycle table ptr (dseg:0x266+) */
extern uint16  word_27E54;                  /* cycle index */
extern int16   pathWpA, pathWpB, pathWpC, pathWpD; /* dseg:0xb94a/48/5a/5c */
struct CommJoy { int8 pad[0x72]; int16 joyPresent; };
extern struct CommJoy far *commData;        /* far ptr dseg:0xd066 */
extern int8  far ovlCall_cbc(void);         /* overlay 1000:0cbc key-ready (al) */
extern int16 far ovlCall_cc1(void);         /* overlay 1000:0cc1 getch */
extern int16 far ovlCall_ccb(int16 v);      /* overlay 1000:0ccb joy btn */
extern void  far ovlFee_23(void);           /* overlay 0fee:0x23 joy settle */
extern void  far ovlCall_b4f(int16 spr);    /* overlay 1000:0b4f sprite blit */
extern void drawMapArc(int16 cx, int16 cy, int16 radius, int16 color,
                       int16 connect, int16 a1, int16 a2);
extern void drawStringAt(int16 *pageNum, const char *string, int16 x, int16 y);
extern void rtcSync(void);                  /* seg000:0x25ea */
extern void sub_161F1(void);                /* seg000:0x61f1 frame tick */
extern void cleanup(void);                  /* seg000:0x0882 */
extern void sub_146E3(void);                /* seg000:0x46e3 restoreCbreak */
extern void sub_1DCAC(int16 a);             /* seg000:0xdcac exit */
extern void selectNextObject(void);
extern void selectPrevObject(void);
extern void selectNextUnit(void);
extern void selectPrevUnit(void);

void sub_11366(int16 *view, struct SelRow *row, int16 *pd) {
    int16 p;                                /* -0x02 arc color */
    int16 a;                                /* -0x04 lo nibble */
    int16 b;                                /* -0x06 hi nibble */
    int16 c;                                /* -0x08 joy0 */
    int16 d;                                /* -0x0A joy1 */
    uint8 e;                                /* -0x0C hold flag */
    uint16 f;                               /* -0x0E radar outer */
    uint16 g;                               /* -0x10 radar inner */
    int16 h;                                /* -0x12 */
    int16 i;                                /* -0x14 key */
    int16 j;                                /* -0x16 objCursor */
    int16 k;                                /* -0x18 */
    int16 l;                                /* -0x1A border color */
    int16 m;                                /* -0x1C selCursor */
    int8  n;                                /* -0x1E row type */

    word_27E50 = (uint16 *)(row->mode * 14 + 0x266);
    blinkTimer = 0;
    c = d = 0;
    byte_27E52 = byte_2C970 = byte_27E5A = e = 0;
    if (byte_27E58 == 1) {
        byte_20A1A = 0;
        e = 1;
    }
    if (commData->joyPresent == 1) {
        c = ovlCall_ccb(0);
        d = ovlCall_ccb(1);
        ovlFee_23();
    }
    while ((ovlCall_cbc() != 0 && c == 0 && d == 0 &&
            byte_216AA >= 0x4E && byte_216AA <= 0xB2 &&
            byte_216AB >= 0x4E && byte_216AB <= 0xB2) || e == 1) {
        if (byte_27E58 == 1 && byte_20A1A > 0xF) {
            e = 0;
            byte_27E58 = 0;
        }
        if (commData->joyPresent == 1) {
            c = ovlCall_ccb(0);
            d = ovlCall_ccb(1);
            ovlFee_23();
        }
        if (byte_212BA != 0) {
            if (word_2B38A == 1) {
                byte_2C970 = 1;
                return;
            }
            cleanup();
            sub_146E3();
            sub_1DCAC(0);
        }
        if (word_27E56 == 1 && blinkTimer > 6) {
            blinkTimer = 0;
            if (flag2CA4C == 1 && byte_2C9E0 == 0) {
                if (byte_27E59 != 0)
                    l = 0;
                else
                    l = 0xF;
                byte_27E59 = (byte_27E59 == 0);
                drawLine(row->rx0 - 1, row->ry0 - 1, row->rx1 + 1,
                         row->ry0 - 1, l);
                drawLine(row->rx1 + 1, row->ry0 - 1, row->rx1 + 1,
                         row->ry1 + 1, l);
                drawLine(row->rx1 + 1, row->ry1 + 1, row->rx0 - 1,
                         row->ry1 + 1, l);
                drawLine(row->rx0 - 1, row->ry1 + 1, row->rx0 - 1,
                         row->ry0, l);
            } else {
                b = word_27E50[word_27E54 + 1] >> 4;
                a = word_27E50[word_27E54 + 1] & 0xF;
                ovlCall_bc7(pd, row->rx0, row->ry0, row->rx1, row->ry1, b, a);
                word_27E54++;
                word_27E54 = (uint16)word_27E54 % *word_27E50;
            }
        }
        if ((row->flag.b & 0x10) && byte_20A1D > 0x12) {
            byte_20A1D = 0;
            if ((row->flag.b & 7) == 4 && unitMarksOn == 1) {
                if (byte_27E59 != 0) {
                    row->sprB[4] = mapToScreenX(worldObjects[word_2C144].x_coord)
                                   + mapClipX1 - 2;
                    row->sprB[5] = mapToScreenY(worldObjects[word_2C144].y_coord)
                                   + mapClipY1 - 2;
                    ovlCall_b4f((int16)row->sprB);
                } else {
                    row->sprA[4] = mapToScreenX(worldObjects[word_2C144].x_coord)
                                   + mapClipX1 - 2;
                    row->sprA[5] = mapToScreenY(worldObjects[word_2C144].y_coord)
                                   + mapClipY1 - 2;
                    ovlCall_b4f((int16)row->sprA);
                }
            }
            if ((row->flag.b & 7) == 2 && ringMode != 0) {
                if (byte_27E59 != 0) {
                    row->sprB[4] = mapToScreenX(worldObjects[word_2C968].x_coord)
                                   + mapClipX1 - 2;
                    row->sprB[5] = mapToScreenY(worldObjects[word_2C968].y_coord)
                                   + mapClipY1 - 2;
                    if (worldObjects[word_2C968].targetFlags & 8)
                        row->sprB[1] = 0x11E;
                    else
                        row->sprB[1] = 0x12D;
                    ovlCall_b4f((int16)row->sprB);
                    if (ringTypes[worldObjects[word_2C968].pad4].flag & 1) {
                        if (ringMode == 1)
                            drawMapArc(worldObjects[word_2C968].x_coord,
                                worldObjects[word_2C968].y_coord,
                                (ringTypes[worldObjects[word_2C968].pad4].f0 *
                                 ringTypes[worldObjects[word_2C968].pad4].f1)
                                / 16 << 6, 0xF, 1, 0, 0x100);
                        else
                            drawMapArc(worldObjects[word_2C968].x_coord,
                                worldObjects[word_2C968].y_coord,
                                ringTypes[worldObjects[word_2C968].pad4].f0 << 6,
                                0xF, 1, 0, 0x100);
                    } else {
                        if (ringMode == 1)
                            drawMapArc(worldObjects[word_2C968].x_coord,
                                worldObjects[word_2C968].y_coord,
                                (ringTypes[worldObjects[word_2C968].pad4].f0 *
                                 ringTypes[worldObjects[word_2C968].pad4].f1)
                                / 16 << 6, 0xF, 0, 0, 0x100);
                        else
                            drawMapArc(worldObjects[word_2C968].x_coord,
                                worldObjects[word_2C968].y_coord,
                                ringTypes[worldObjects[word_2C968].pad4].f0 << 6,
                                0xF, 0, 0, 0x100);
                    }
                } else {
                    row->sprA[4] = mapToScreenX(worldObjects[word_2C968].x_coord)
                                   + mapClipX1 - 2;
                    row->sprA[5] = mapToScreenY(worldObjects[word_2C968].y_coord)
                                   + mapClipY1 - 2;
                    if (worldObjects[word_2C968].targetFlags & 8)
                        row->sprA[1] = 0x11E;
                    else
                        row->sprA[1] = 0x12D;
                    ovlCall_b4f((int16)row->sprA);
                    if (ringTypes[worldObjects[word_2C968].pad4].flag & 1) {
                        if (ringMode == 1)
                            drawMapArc(worldObjects[word_2C968].x_coord,
                                worldObjects[word_2C968].y_coord,
                                (ringTypes[worldObjects[word_2C968].pad4].f0 *
                                 ringTypes[worldObjects[word_2C968].pad4].f1)
                                / 16 << 6, 1, 1, 0, 0x100);
                        else
                            drawMapArc(worldObjects[word_2C968].x_coord,
                                worldObjects[word_2C968].y_coord,
                                ringTypes[worldObjects[word_2C968].pad4].f0 << 6,
                                1, 1, 0, 0x100);
                    } else {
                        if (ringMode == 1)
                            drawMapArc(worldObjects[word_2C968].x_coord,
                                worldObjects[word_2C968].y_coord,
                                (ringTypes[worldObjects[word_2C968].pad4].f0 *
                                 ringTypes[worldObjects[word_2C968].pad4].f1)
                                / 16 << 6, 0, 0, 0, 0x100);
                        else
                            drawMapArc(worldObjects[word_2C968].x_coord,
                                worldObjects[word_2C968].y_coord,
                                ringTypes[worldObjects[word_2C968].pad4].f0 << 6,
                                0, 0, 0, 0x100);
                    }
                }
            }
            if ((row->flag.b & 7) == 3 && siteMarksOn == 1) {
                if (byte_27E59 != 0) {
                    row->sprB[4] = mapToScreenX(worldObjects[word_2C968].x_coord)
                                   + mapClipX1 - 2;
                    row->sprB[5] = mapToScreenY(worldObjects[word_2C968].y_coord)
                                   + mapClipY1 - 2;
                    if (worldObjects[word_2C968].targetFlags & 8)
                        row->sprB[1] = 0x11E;
                    else
                        row->sprB[1] = 0x12D;
                    ovlCall_b4f((int16)row->sprB);
                    drawMapArc(worldObjects[word_2C968].x_coord,
                               worldObjects[word_2C968].y_coord,
                               siteTypeParms[worldObjects[word_2C968].pad4 * 9]
                               << 6, 0xF, 1, 0, 0x100);
                } else {
                    row->sprA[4] = mapToScreenX(worldObjects[word_2C968].x_coord)
                                   + mapClipX1 - 2;
                    row->sprA[5] = mapToScreenY(worldObjects[word_2C968].y_coord)
                                   + mapClipY1 - 2;
                    if (worldObjects[word_2C968].targetFlags & 8)
                        row->sprA[1] = 0x11E;
                    else
                        row->sprA[1] = 0x12D;
                    ovlCall_b4f((int16)row->sprA);
                    drawMapArc(worldObjects[word_2C968].x_coord,
                               worldObjects[word_2C968].y_coord,
                               siteTypeParms[worldObjects[word_2C968].pad4 * 9]
                               << 6, 4, 1, 0, 0x100);
                }
            }
            if ((row->flag.b & 7) == 6 && tileMarksOn == 1) {
                if (byte_27E59 != 0) {
                    for (f = 0; f < 16; f++)
                        for (g = 0; g < 16; g++)
                            if (tileMarkMap[f + g * 16] & 0x10) {
                                row->sprB[4] = mapToScreenX(f * 0x7FF)
                                               + mapClipX1;
                                row->sprB[5] = mapToScreenY(g * 0x7FF)
                                               + mapClipY1;
                                ovlCall_b4f((int16)row->sprB);
                            }
                } else {
                    for (f = 0; f < 16; f++)
                        for (g = 0; g < 16; g++)
                            if (tileMarkMap[f + g * 16] & 0x10) {
                                row->sprA[4] = mapToScreenX(f * 0x7FF)
                                               + mapClipX1;
                                row->sprA[5] = mapToScreenY(g * 0x7FF)
                                               + mapClipY1;
                                ovlCall_b4f((int16)row->sprA);
                            }
                }
            }
            if ((row->flag.b & 7) == 1) {
                if (byte_27E59 != 0)
                    pd[2] = 0xF;
                else
                    pd[2] = 1;
                drawStringAt(pd, "P",
                    mapToScreenX(worldObjects[pathWpB].x_coord) + mapClipX1,
                    mapToScreenY(worldObjects[pathWpB].y_coord - 2)
                    + mapClipY1);
                drawStringAt(pd, "S",
                    mapToScreenX(worldObjects[pathWpC].x_coord) + mapClipX1,
                    mapToScreenY(worldObjects[pathWpC].y_coord - 2)
                    + mapClipY1);
            }
            if ((row->flag.b & 7) == 5) {
                if (byte_27E59 != 0) {
                    pd[2] = 0xF;
                    plotMapPoint(worldObjects[pathWpA].x_coord,
                                 worldObjects[pathWpA].y_coord, 0xF, 0);
                    drawMapLine(worldObjects[pathWpA].x_coord,
                                worldObjects[pathWpA].y_coord,
                                worldObjects[pathWpB].x_coord,
                                worldObjects[pathWpB].y_coord);
                    drawMapLine(worldObjects[pathWpB].x_coord,
                                worldObjects[pathWpB].y_coord,
                                worldObjects[pathWpC].x_coord,
                                worldObjects[pathWpC].y_coord);
                    drawMapLine(worldObjects[pathWpC].x_coord,
                                worldObjects[pathWpC].y_coord,
                                worldObjects[pathWpD].x_coord,
                                worldObjects[pathWpD].y_coord);
                    pd[2] = 0xF;
                    if (pathWpA == pathWpD)
                        drawStringAt(pd, "T,L",
                            mapToScreenX(worldObjects[pathWpA].x_coord)
                            + mapClipX1,
                            mapToScreenY(worldObjects[pathWpA].y_coord - 2)
                            + mapClipY1);
                    else {
                        drawStringAt(pd, "T",
                            mapToScreenX(worldObjects[pathWpA].x_coord)
                            + mapClipX1,
                            mapToScreenY(worldObjects[pathWpA].y_coord - 2)
                            + mapClipY1);
                        drawStringAt(pd, "L",
                            mapToScreenX(worldObjects[pathWpD].x_coord)
                            + mapClipX1,
                            mapToScreenY(worldObjects[pathWpD].y_coord - 2)
                            + mapClipY1);
                    }
                } else {
                    pd[2] = 0;
                    plotMapPoint(worldObjects[pathWpA].x_coord,
                                 worldObjects[pathWpA].y_coord, 0xF, 0);
                    drawMapLine(worldObjects[pathWpA].x_coord,
                                worldObjects[pathWpA].y_coord,
                                worldObjects[pathWpB].x_coord,
                                worldObjects[pathWpB].y_coord);
                    drawMapLine(worldObjects[pathWpB].x_coord,
                                worldObjects[pathWpB].y_coord,
                                worldObjects[pathWpC].x_coord,
                                worldObjects[pathWpC].y_coord);
                    drawMapLine(worldObjects[pathWpC].x_coord,
                                worldObjects[pathWpC].y_coord,
                                worldObjects[pathWpD].x_coord,
                                worldObjects[pathWpD].y_coord);
                    pd[2] = 1;
                    if (pathWpA == pathWpD)
                        drawStringAt(pd, "T,L",
                            mapToScreenX(worldObjects[pathWpA].x_coord)
                            + mapClipX1,
                            mapToScreenY(worldObjects[pathWpA].y_coord - 2)
                            + mapClipY1);
                    else {
                        drawStringAt(pd, "T",
                            mapToScreenX(worldObjects[pathWpA].x_coord)
                            + mapClipX1,
                            mapToScreenY(worldObjects[pathWpA].y_coord - 2)
                            + mapClipY1);
                        drawStringAt(pd, "L",
                            mapToScreenX(worldObjects[pathWpD].x_coord)
                            + mapClipX1,
                            mapToScreenY(worldObjects[pathWpD].y_coord - 2)
                            + mapClipY1);
                    }
                }
            }
            byte_27E59 = (byte_27E59 == 0);
        }
        sub_161F1();
    }
    if (ovlCall_cbc() == 0)
        i = ovlCall_cc1();
    else if (c == 1)
        i = 0x0D;
    else if (d == 1)
        i = 0x1B;
    else if (byte_216AA < 0x4E) {
        i = 0x4B00;
        byte_27E58 = 1;
    } else if (byte_216AA > 0xB2) {
        i = 0x4D00;
        byte_27E58 = 1;
    } else if (byte_216AB < 0x4E) {
        i = 0x4800;
        byte_27E58 = 1;
    } else if (byte_216AB > 0xB2) {
        i = 0x5000;
        byte_27E58 = 1;
    }
    j = word_2C968;
    m = word_2C144;
    if ((int8)i == 0x0D)
        byte_2C970 = 1;
    if (i == 0x1000) {
        byte_212BA = 1;
        byte_2C970 = 1;
        byte_2D06B = 1;
    }
    if ((int8)i == 0x1B && (row->flag.w & 0x200)) {
        byte_2C970 = 1;
        byte_27E5A = 1;
    }
    if (i == 0x4800) {          /* UP */
        register int16 h;
        word_2CA64 -= view[1];
        if (flag2C7CE == 1 && view[3] == (int16)word_2CA60 &&
            ((h = view[4] - view[1]) || 1) && (int16)word_2CA64 <= h)
            word_2CA64 = h;
        else if (view[4] > (int16)word_2CA64)
            word_2CA64 = view[4];
        byte_27E52 = 1;
        if (flag_29948 == 1)
            rtcSync();
    }
    if (i == 0x5000) {          /* DOWN */
        word_2CA64 += view[1];
        if (view[5] < word_2CA64)
            word_2CA64 = view[5];
        byte_27E52 = 1;
        if (flag_29948 == 1)
            rtcSync();
    }
    if (i == 0x4D00) {          /* RIGHT */
        n = row->flag.b & 7;
        if (n == 2 || n == 3)
            selectNextObject();
        else if ((row->flag.b & 7) == 4)
            selectNextUnit();
        else {
            word_2CA60 += view[0];
            if (view[3] < word_2CA60)
                word_2CA60 = view[3];
        }
        byte_27E52 = 1;
    }
    if (i == 0x4B00) {          /* LEFT */
        n = row->flag.b & 7;
        if (n == 2 || n == 3)
            selectPrevObject();
        else if ((row->flag.b & 7) == 4)
            selectPrevUnit();
        else {
            word_2CA60 -= view[0];
            if (view[2] > (int16)word_2CA60)
                word_2CA60 = view[2];
            if (view[4] > (int16)word_2CA64)
                word_2CA60 += view[0];
        }
        byte_27E52 = 1;
    }
    if (row->flag.b & 0x10) {
        if ((row->flag.b & 7) == 4 && unitMarksOn == 1) {
            row->sprA[4] = mapToScreenX(worldObjects[m].x_coord)
                           + mapClipX1 - 2;
            row->sprA[5] = mapToScreenY(worldObjects[m].y_coord)
                           + mapClipY1 - 2;
            ovlCall_b4f((int16)row->sprA);
        }
        if ((row->flag.b & 7) == 2 && ringMode != 0) {
            row->sprA[4] = mapToScreenX(worldObjects[j].x_coord)
                           + mapClipX1 - 2;
            row->sprA[5] = mapToScreenY(worldObjects[j].y_coord)
                           + mapClipY1 - 2;
            if (worldObjects[j].targetFlags & 8)
                row->sprA[1] = 0x11E;
            else
                row->sprA[1] = 0x12D;
            ovlCall_b4f((int16)row->sprA);
            if (ringTypes[worldObjects[j].pad4].flag & 1) {
                if ((uint8)objectActive[j] > 1)
                    p = 0xF;
                else
                    p = 1;
                if (ringMode == 1)
                    drawMapArc(worldObjects[j].x_coord,
                               worldObjects[j].y_coord,
                               (ringTypes[worldObjects[j].pad4].f0 *
                                ringTypes[worldObjects[j].pad4].f1) / 16 << 6,
                               p, 1, 0, 0x100);
                else
                    drawMapArc(worldObjects[j].x_coord,
                               worldObjects[j].y_coord,
                               ringTypes[worldObjects[j].pad4].f0 << 6,
                               p, 1, 0, 0x100);
            } else {
                if ((uint8)objectActive[j] > 1)
                    p = 0xF;
                else
                    p = 0;
                if (ringMode == 1)
                    drawMapArc(worldObjects[j].x_coord,
                               worldObjects[j].y_coord,
                               (ringTypes[worldObjects[j].pad4].f0 *
                                ringTypes[worldObjects[j].pad4].f1) / 16 << 6,
                               p, 0, 0, 0x100);
                else
                    drawMapArc(worldObjects[j].x_coord,
                               worldObjects[j].y_coord,
                               ringTypes[worldObjects[j].pad4].f0 << 6,
                               p, 0, 0, 0x100);
            }
        }
        if ((row->flag.b & 7) == 3 && siteMarksOn == 1) {
            row->sprA[4] = mapToScreenX(worldObjects[j].x_coord)
                           + mapClipX1 - 2;
            row->sprA[5] = mapToScreenY(worldObjects[j].y_coord)
                           + mapClipY1 - 2;
            if (worldObjects[j].targetFlags & 8)
                row->sprA[1] = 0x11E;
            else
                row->sprA[1] = 0x12D;
            ovlCall_b4f((int16)row->sprA);
            drawMapArc(worldObjects[j].x_coord, worldObjects[j].y_coord,
                       siteTypeParms[worldObjects[j].pad4 * 9] << 6,
                       4, 1, 0, 0x100);
        }
        if ((row->flag.b & 7) == 1) {
            pd[2] = 1;
            drawStringAt(pd, "P",
                mapToScreenX(worldObjects[pathWpB].x_coord) + mapClipX1,
                mapToScreenY(worldObjects[pathWpB].y_coord - 2) + mapClipY1);
            drawStringAt(pd, "S",
                mapToScreenX(worldObjects[pathWpC].x_coord) + mapClipX1,
                mapToScreenY(worldObjects[pathWpC].y_coord - 2) + mapClipY1);
        }
        if ((row->flag.b & 7) == 6 && tileMarksOn == 1) {
            for (f = 0; f < 16; f++)
                for (g = 0; g < 16; g++)
                    if (tileMarkMap[f + g * 16] & 0x10) {
                        row->sprA[4] = mapToScreenX(f * 0x7FF) + mapClipX1;
                        row->sprA[5] = mapToScreenY(g * 0x7FF) + mapClipY1;
                        ovlCall_b4f((int16)row->sprA);
                    }
        }
        if ((row->flag.b & 7) == 5) {
            pd[2] = 0;
            plotMapPoint(worldObjects[pathWpA].x_coord,
                         worldObjects[pathWpA].y_coord, 0xF, 0);
            drawMapLine(worldObjects[pathWpA].x_coord,
                        worldObjects[pathWpA].y_coord,
                        worldObjects[pathWpB].x_coord,
                        worldObjects[pathWpB].y_coord);
            drawMapLine(worldObjects[pathWpB].x_coord,
                        worldObjects[pathWpB].y_coord,
                        worldObjects[pathWpC].x_coord,
                        worldObjects[pathWpC].y_coord);
            drawMapLine(worldObjects[pathWpC].x_coord,
                        worldObjects[pathWpC].y_coord,
                        worldObjects[pathWpD].x_coord,
                        worldObjects[pathWpD].y_coord);
            pd[2] = 1;
            if (pathWpA == pathWpD)
                drawStringAt(pd, "T,L",
                    mapToScreenX(worldObjects[pathWpA].x_coord) + mapClipX1,
                    mapToScreenY(worldObjects[pathWpA].y_coord - 2)
                    + mapClipY1);
            else {
                drawStringAt(pd, "T",
                    mapToScreenX(worldObjects[pathWpA].x_coord) + mapClipX1,
                    mapToScreenY(worldObjects[pathWpA].y_coord - 2)
                    + mapClipY1);
                drawStringAt(pd, "L",
                    mapToScreenX(worldObjects[pathWpD].x_coord) + mapClipX1,
                    mapToScreenY(worldObjects[pathWpD].y_coord - 2)
                    + mapClipY1);
            }
        }
    }
}
