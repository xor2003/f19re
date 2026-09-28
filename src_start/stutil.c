/* START.EXE — utility helpers (tmp/linker shared/util.c+miscstub.c, strand.c,
 * stgrid.c, stinkey.c, stinit.c lineage; adapted/verified vs EN binary).
 * Driver slots: far calls into the patched jump table at para 0x1000,
 * offset = 0xAFA + 5*slot (misc 5a..5e, gfx 0..0x59, audio 0x64+). */
#include "inttype.h"

extern int16 rand(void);
extern int16 rangeApprox(int16, int16);

/* ---- driver-slot callees ---- */
extern void far gfx_setColor(int16 color);            /* slot 0x21 */
extern void far gfx_resetBlitOffset2(void);           /* slot 0x23 */
extern int16 far gfx_setFont(uint16 ch, uint16 font); /* slot 0x2f */
extern int16 far misc_jump_5a_keybuf(void);           /* slot 0x5a */
extern int16 far misc_jump_5b_getkey(void);           /* slot 0x5b */
extern int16 far misc_jump_5d_readJoy(int16);         /* slot 0x5d */
extern void far misc_jump_5e_clearKeyFlags(void);     /* slot 0x5e */
extern void far audio_jump_6b(void);                  /* slot 0x6b */
extern void far gfx_allocPage(int16 page);            /* slot 0x00 */
extern void far gfx_setPageN(uint16 n);               /* slot 0x0e */
extern void far gfx_setMode13(int16 mono);            /* slot 0x3c */
extern int16 far gfx_getModecode(void);               /* slot 0x3f */
extern void sub_140A3(void);                          /* seg000:0x40a3 seedRandom */

/* ---- skeleton callees ---- */
extern int16 allocBuffer(int16 a);                      /* seg000:0x6828 */
extern void sub_151E8(char far *dst, int16 val, int16 n); /* seg000:0x51e8 far memset */
extern void sub_141A3(void);                            /* seg000:0x41a3 clipper */
extern void sub_18B7E(int16, int16, int16, int16);      /* seg000:0x8b7e */
extern int16 getch(void);                               /* seg000:0xe200 */
extern void sub_146E3(void);                            /* seg000:0x46e3 restoreCbreak */
extern void sub_1DCAC(int16);                           /* seg000:0xdcac exit */
extern char *formatGridRef(int16, int16, int16);        /* seg000:0x8d98 */
extern void cleanup(void);

/* ---- globals ---- */
extern uint8 g_cntA, g_cntB, g_cntC, g_cntD;  /* dseg:0xa1a..0xa1d */
extern uint8 cbreakHit;                       /* byte dseg:0x12ba */

struct CommData {                             /* far ptr dseg:0xd066 */
    int8 pad24[0x24];
    int16 setupMono;                          /* 0x24 */
    int8 pad26[0x4c];
    int16 setupUseJoy;                        /* 0x72 */
    int8 pad74[0x04];
    int16 gfxModeNum;                         /* 0x78 */
};
extern struct CommData far *commData;         /* far ptr dseg:0xd066 */
extern uint16 far *gfxModeSetPtr;             /* far ptr dseg:0x98ec */
extern int16 g_gfxModeNum;                    /* dseg:0xb83e */
extern int16 g_lineX0, g_lineX1, g_lineY0, g_lineY1; /* dseg:0xd2b..0xd31 */
extern uint8 g_flagTable2CFE[];               /* dseg:0x2cfe */

typedef struct {                          /* worldObjects: stride 0x10 */
    int16 x_coord, y_coord;               /* 0x00, 0x02 */
    int16 pad4;                           /* 0x04 */
    int16 targetFlags;                    /* 0x06 */
    int16 pad8[4];                        /* 0x08 */
} WorldObject;
extern WorldObject worldObjects[];            /* dseg:0xb390 */

struct GameData { int8 pad[0x38]; int16 theater; };
extern struct GameData far *gameData;         /* far ptr dseg:0x991c */

int16 randMul(uint16 arg) {              /* seg000:0x40ae */
    return (rand() * (int32)arg) >> 0xf;
}

void mystrcpy(char *dst, const char *src) {   /* seg000:0x5120 */
    while ((*dst++ = *src++) != 0);
}

/* mystrlen (seg000:0x516d) is hand-asm — preloads s into ax pre-loop,
 * ~(s_orig - s_end) tail, zero locals; no C shape produces it. Skeleton. */

void getTimeOfDay(void) {                 /* seg000:0x40c8 */
    g_cntB++;
    g_cntC++;
    g_cntA++;
    g_cntD++;
    audio_jump_6b();
}

/* seg000:0x5ad2 — NOT drawStringCentered: callers push (n, 0) and store the
 * result as a page segment; body is allocBuffer(n) + zero-fill(seg:0, n).
 * i.e. alloc-zeroed-buffer. Map name kept for mzdiff lookup. */
union FarWords { char far *p; struct { uint16 off; uint16 seg; } w; };
int16 drawStringCentered(int16 size, int16 unused) {
    int16 seg;
    union FarWords u;
    seg = allocBuffer(size);
    u.w.seg = seg;
    u.w.off = 0;
    sub_151E8(u.p, 0, size);
    return seg;
}

int16 showMsgWaitKey(const char *msg) {   /* seg000:0x7518 */
    sub_18B7E((int16)msg, 0, 0x60, 0xf);
    return getch();
}

int16 itemDistance(int16 idx1, int16 idx2) {  /* seg000:0x8746 */
    return rangeApprox(worldObjects[idx1].x_coord - worldObjects[idx2].x_coord,
                       worldObjects[idx1].y_coord - worldObjects[idx2].y_coord);
}

char *getItemCoordStr(int16 idx) {        /* seg000:0x8d74 */
    return formatGridRef(worldObjects[idx].x_coord, worldObjects[idx].y_coord,
                         gameData->theater);
}

int16 readInputKey(void) {                /* seg000:0x8c8 */
    int16 key;
    if (commData->setupUseJoy == 1) {
        do {
            if (misc_jump_5a_keybuf() == 0) break;
        } while (misc_jump_5d_readJoy(0) == 0);
        if (misc_jump_5a_keybuf() != 0)
            goto checkKey;              /* key left unassigned — asm reads it */
    }
    key = misc_jump_5b_getkey();
checkKey:
    if (key == 0x1000) {
        cleanup();
        if (cbreakHit != 0)
            sub_146E3();
        sub_1DCAC(0);
    }
    /* no return stmt — ax already holds the key (garbage on the goto path) */
}

void initGraphics(void) {                 /* seg000:0x827 */
    uint8 unused[0xe];
    sub_140A3();                          /* seedRandom */
    gfx_setPageN(0);
    gfx_allocPage(0);
    if (*gfxModeSetPtr == 0) {
        gfx_setMode13(commData->setupMono);
        *gfxModeSetPtr = 1;
    }
    commData->gfxModeNum = g_gfxModeNum = gfx_getModecode();
    misc_jump_5e_clearKeyFlags();
}

int16 stringWidth(int16 *page, const uint8 *str) {   /* seg000:0x3e38 */
    const uint8 *l;
    int16 j, n;
    l = str;
    j = page[6];
    n = 0;
    while (*l != 0)
        n += gfx_setFont(*l++, j);
    return n;
}

void drawLine(int16 x0, int16 y0, int16 x1, int16 y1, int16 color) { /* seg000:0xc083 */
    gfx_setColor(color);
    g_lineX0 = x0;
    g_lineY0 = y0;
    g_lineX1 = x1;
    g_lineY1 = y1;
    sub_141A3();
    gfx_resetBlitOffset2();
}

/* seg000:0xc0b3 drawClippedLineEx — drawLine with a clip window; START's
 * variant stores raw coords and re-states the driver clip bounds. */
extern int16 g_clipMaxX, g_clipMaxY;    /* dseg:0xd25/0xd27 */
extern int16 *g_vpParms;                /* dseg:0x6572 — vpParms->f[2] = color */
extern int16 far gfx_calcRowAddr(int16 a, int16 b);  /* slot 0x3e */
extern void far gfx_setBlitOffset(int16 a);          /* slot 0x1a */
extern void far gfx_setOvlVal1(int16 v);             /* slot 0x40 */
extern void far gfx_setOvlVal2(int16 v);             /* slot 0x41 */

void drawClippedLineEx(int16 x1, int16 y1, int16 x2, int16 y2,
                       int16 clipL, int16 clipR, int16 clipT, int16 clipB,
                       int16 both) {
    int16 clipH, clipW;
    clipW = clipR - clipL;
    clipH = clipB - clipT;
    gfx_setBlitOffset(gfx_calcRowAddr(clipL, clipT));
    g_clipMaxX = clipW - 1;
    g_clipMaxY = clipH - 1;
    gfx_setOvlVal1(clipH - 1);
    gfx_setOvlVal2(g_clipMaxX);
    gfx_setColor(g_vpParms[2]);
    g_lineX0 = x1;
    g_lineY0 = y1;
    g_lineX1 = x2;
    g_lineY1 = y2;
    sub_141A3();
    gfx_resetBlitOffset2();
    g_clipMaxX = 0x13F;
    g_clipMaxY = 0xC7;
    gfx_setOvlVal1(0xC7);
    gfx_setOvlVal2(g_clipMaxX);
    gfx_setBlitOffset(0);
}

/* seg000:0x3886 — draws a tile's icon sprites while flag bit 0x80 is set;
 * forces obj.f06 = 3 (palette bank) around the blit. */
struct TileEntry {
    int16 pad[0x10];
    int16 x0, y0, x1, y1;              /* +0x20..+0x27 span rect for sub_14622 */
    int16 spr1;                        /* +0x28 sprite handle */
    int16 spr2;                        /* +0x2A optional second sprite */
    int16 f2C, f2E;
    int8  flag;                        /* +0x30 bit 0x80 = visible */
    int8  pad31;
};
extern int16 flag_29948;                              /* dseg:0x9948 */
extern void far gfx_blitSprite(int16 spr);            /* slot 0x11 */
extern void sub_14622(void *o, int16 x0, int16 y0, int16 x1, int16 y1);
struct ObjF06 { int16 pad[3]; int16 f06; };

void drawTileIcon(struct TileEntry *t, uint16 idx, struct ObjF06 *o) {
    int16 save;
    if (t[idx].flag & 0x80) {
        save = o->f06;
        o->f06 = 3;
        if (flag_29948 == 0) {
            register struct TileEntry *e = &t[idx];
            sub_14622(o, e->x0, e->y0, e->x1, e->y1);
        }
        gfx_blitSprite(t[idx].spr1);
        o->f06 = save;
        if (t[idx].spr2 != 0)
            gfx_blitSprite(t[idx].spr2);
    }
}

void mystrcat(char *d, char *s) {         /* seg000:0x5189 */
    for (;;) {
        if (*d == 0) break;
        d++;
    }
    for (; (*d = *s++) != 0; d++) ;
}

/* seg000:0x51b1 mystrchr — hand-asm (push si never used, ch hoisted to ax
 * before loop); skeleton stays. */

/* seg000:0x262c/0x2672/0x2706 — cyclic next/prev selectors over worldObjects */
extern int16 selCursor;                     /* dseg:0x2c144 */
extern int16 objCursor;                     /* dseg:0x2c968 */
extern int16 objectCount;                   /* dseg:0x2c978 */
extern int8  objectActive[];                /* dseg:0x2d278 */

void selectNextObject(void) {
    int8 again;
    again = 1;
    do {
        objCursor++;
        if (objCursor > objectCount) objCursor = 0;
        if (worldObjects[objCursor].pad4 != 0 || objectActive[objCursor] != 0)
            again = 0;
    } while (again != 0);
}

void selectNextUnit(void) {
    int8 again;
    again = 1;
    do {
        selCursor++;
        if (selCursor > objectCount) selCursor = 0;
        if ((worldObjects[selCursor].targetFlags & 1) != 0 ||
            (worldObjects[selCursor].targetFlags & 0x200) != 0)
            if ((worldObjects[selCursor].targetFlags & 0x800) == 0)
                again = 0;
    } while (again != 0);
}

void selectPrevUnit(void) {
    int8 again;
    again = 1;
    do {
        selCursor--;
        if (selCursor < 0) selCursor = objectCount - 1;
        if ((worldObjects[selCursor].targetFlags & 1) != 0 ||
            (worldObjects[selCursor].targetFlags & 0x200) != 0)
            if ((worldObjects[selCursor].targetFlags & 0x800) == 0)
                again = 0;
    } while (again != 0);
}

void selectPrevObject(void) {
    int8 again;
    again = 1;
    do {
        objCursor--;
        if (objCursor < 0) objCursor = objectCount - 1;
        if (worldObjects[objCursor].pad4 != 0 || objectActive[objCursor] != 0)
            again = 0;
    } while (again != 0);
}

/* seg000:0xbd7e / 0xbe09 — map overlay markers: per-unit blip & 16x16 tile grid.
 * dstX/dstY written via byte offset into the SpriteParams pool at word_20000. */
extern uint8 unitMarksOn;                  /* dseg:0x98e8 */
extern uint8 tileMarksOn;                  /* dseg:0xbe48 */
extern int8  tileMarkMap[];                /* dseg:0xb842 — 16x16, bit 0x10 */
extern int16 unitSprOff, gridSprOff;       /* dseg:0x6612/0x6652 — byte offs into pool */
extern int8  sprParmsTab[];                /* dseg:0x0008 (word_20000) */
extern int16 mapClipX1, mapClipY1;         /* stmap.c — map origin for markers */
extern void far gfx_blitSprite(int16 sprOff);
extern int16 mapToScreenX(int16 v);
extern int16 mapToScreenY(int16 v);

void drawUnitMarkers(void) {
    uint16 i;
    if (unitMarksOn == 0) return;
    for (i = 0; i < (uint16)objectCount; i++) {
        if ((worldObjects[i].targetFlags & 1) != 0 ||
            (worldObjects[i].targetFlags & 0x200) != 0)
            if ((worldObjects[i].targetFlags & 0x800) == 0) {
                *(int16 *)(sprParmsTab + unitSprOff + 8) =
                    mapToScreenX(worldObjects[i].x_coord) + mapClipX1 - 2;
                *(int16 *)(sprParmsTab + unitSprOff + 0xA) =
                    mapToScreenY(worldObjects[i].y_coord) + mapClipY1 - 2;
                gfx_blitSprite(unitSprOff);
            }
    }
}

void drawTileMarkers(void) {
    uint16 i, j;
    if (tileMarksOn == 0) return;
    for (i = 0; i < 16; i++) {
        for (j = 0; j < 16; j++) {
            if ((tileMarkMap[i + j * 16] & 0x10) != 0) {
                *(int16 *)(sprParmsTab + gridSprOff + 8) =
                    mapToScreenX(i * 0x7FF) + mapClipX1;
                *(int16 *)(sprParmsTab + gridSprOff + 0xA) =
                    mapToScreenY(j * 0x7FF) + mapClipY1;
                gfx_blitSprite(gridSprOff);
            }
        }
    }
}


/* seg000:0x25ea — RTC sync when enabled: int 0x1a read, stash tick, then 10
 * settle ticks via sub_16208 */
extern int16 rtcEnabled;                    /* dseg:0xbb74 */
extern int8  rtcFlagByte;                   /* dseg:0x3e1f */
extern int16 rtcTickBuf;                    /* dseg:0x3e24 */
extern int16 rtcTickSaved;                  /* dseg:0x9920 */
extern void  sub_167FD(void);
extern void  sub_16208(void);
extern void  intDispatch(int16 n, uint8 *a, uint8 *b);

void rtcSync(void) {
    uint16 i;
    if (rtcEnabled == 1) {
        sub_167FD();
        rtcFlagByte = 0;
        intDispatch(0x1a, (uint8*)0x3e1e, (uint8*)0x3e1e);
        rtcTickSaved = rtcTickBuf;
        sub_16208();
        i = 0;
        do {
            i++;
            sub_16208();
        } while (i < 0xa);
    }
}

/* seg000:0x669f — script expression evaluator: walks *pp (a near cursor the
 * routine advances) at fixed index; ')'→0, '|'→1, ':N'→N-1, '('→skip to the
 * matching ')'.  Called by the briefing-choice interpreter sub_16261. */
int16 evalChoiceExpr(uint8 **pp, int16 idx) {
    int8  a, uz;                  /* ch -> [bp-2], digit ch -> [bp-0a] */
    int16 f, i, res;              /* n -> [bp-4], paren depth -> [bp-6] */
    res = 0;
    for (;;) {
        a = (*pp)[idx];
        (*pp)++;
        if (a == 0x29) return 0;
        if (a == 0x7C) return 1;
        if (a == 0x3A) {
            f = 0;
            goto t;
b:          if (uz > 0x39) goto r;
            f = f * 10 + uz - 0x30;
            (*pp)++;
t:          uz = (*pp)[idx];
            if (uz >= 0x30) goto b;
r:          return f - 1;
        }
        if (a == 0x28) {
            i = 1;
            do {
                i += ((*pp)[idx] == 0x28) ? 1 : 0;
                i -= ((*pp)[idx] == 0x29) ? 1 : 0;
                (*pp)++;
            } while (i > 0);
            continue;
        }
    }
}


/* seg000:0x6763 — marks clipTable[i].flag = 0 for entries whose rect
 * intersects the (x,y,w,h) window translated by the view origin.
 * The x0 else-arm reads clipTable[iu].x0 (not e->x0): that keeps MSC
 * from binding e->x0 to si, so the dispatch emits cmp [bx],ax plus a
 * plain [bx] reload and the second-written arm sinks to body top. */
struct ClipEntry {
    int16 x0, y0, w, h;           /* +0,+2,+4,+6 */
    int8  pad[0x52];
    int8  flag;                   /* +0x5a */
};
extern int16 viewOriginX, viewOriginY;      /* dseg:0x2ca4e/0x2ca50 */
extern int16 clipEntryCount;                /* dseg:0x2367c */
extern struct ClipEntry clipTable[];        /* dseg:0x2326, stride 0x5c */

void clipEntries(int16 x, int16 y, int16 w, int16 h) {
    struct ClipEntry *e;
    int16 f, i, m, iu, uz;
    x -= viewOriginX;
    y -= viewOriginY;
    for (iu = 0; iu < clipEntryCount; iu++) {
        e = &clipTable[iu];
        if (e->x0 > x) uz = clipTable[iu].x0; else uz = x;
        if (e->y0 > y) m = e->y0; else m = y;
        i = (e->x0 + e->w <= x + w) ? e->x0 + e->w : x + w;
        f = (e->y0 + e->h <= y + h) ? e->y0 + e->h : y + h;
        if (uz < i && m < f)
            e->flag = 0;
    }
}

/* ==== seg000:0xbf03 drawMapArc — angle-swept arc/ring on the map.
 * /Os module: under /Ot the step-size ternary emits a stray relax-pad nop.
 * Local slots: q@-2, x@-4, i@-6, j(prevX)@-8, k(y)@-A, l(step)@-C,
 * m(spare)@-E, n(prevY)@-0x10 — names chosen for the hash buckets. ==== */
extern int16 sinMul(int16 angle, int16 value);
extern int16 cosMul(int16 angle, int16 value);
extern void  plotMapPoint(int16 x, int16 y, int16 color, int16 unused);
extern void  drawMapLine(int16 x1, int16 y1, int16 x2, int16 y2);

void drawMapArc(int16 cx, int16 cy, int16 radius, int16 color,
                int16 connect, int16 a1, int16 a2) {
    int16 q, x, i, j, k, l, m, n;

    if (a2 < a1)
        a1 += 0x100;
    g_vpParms[2] = color;
    l = connect ? 8 : 0x10;
    if (!connect && radius >= 0xBB8)
        l = 8;
    if (!connect && radius >= 0x1B58)
        l = 4;
    i = a1;
    goto test;
body:
    q = i << 8;
    x = cx + sinMul(q, radius);
    k = cy - cosMul(q, radius);
    if ((uint16)x > 0xC000)
        x = 0;
    if ((uint16)k > 0xC000)
        k = 0;
    if (x && k && j && n) {
        if (i != a1 && connect)
            drawMapLine(x, k, j, n);
        else
            plotMapPoint(x, k, color, 0);
    }
    j = x;
    n = k;
    i += l;
test:
    if (i <= a2)
        goto body;
}

/* seg000:0x47e8 — thin CDECL wrapper over the int21h raw-read routine
 * (seg000:0x48fe: bx=handle, cx=count, dx=buf). */
extern int16 sub_148FE(int16 handle, int16 count, int16 buf);

int16 dosRead(int16 handle, int16 count, int16 buf) {
    return sub_148FE(handle, count, buf);
}

/* seg000:0x5b02 — advances the scratch-buffer cursor word_22322 by n after
 * handing (pos, n) to sub_15B22; returns the pre-advance position. */
extern int16 sub_15B22(int16 pos, int16 n);
extern int16 word_22322;

int16 advanceBufPos(int16 n) {
    sub_15B22(word_22322, n);
    word_22322 += n;
    return word_22322 - n;
}

/* seg000:0x61cc — stores the view origin then redraws via sub_15B68. */
extern int16 sub_15B68(int16 flag);

int16 setViewOrigin(int16 a, int16 b, int16 flag) {
    viewOriginX = b;
    viewOriginY = a;
    if (sub_15B68(flag) != 0)
        return 1;
    return 0;
}

/* seg000:0x4089 — waits n RTC ticks: arms the tick counter byte_20A1A via
 * sub_14E9C (PIT/vector install), spins until it reaches n, then restores
 * via sub_14EDA. */
extern void sub_14E9C(void);
extern void sub_14EDA(void);
extern uint8 byte_20A1A;

void delayTicks(int16 n) {
    byte_20A1A = 0;
    sub_14E9C();
    while (n >= (uint8)byte_20A1A)
        ;
    sub_14EDA();
}

/* seg000:0x5b22 — drains n bytes from the 0x200 file buffer at dseg:0x12c2;
 * refills via sub_16C0E and resets the read pos when it passes 0x1ff. */
extern void sub_16C0E(void);
extern int16 word_21714;
extern int8 byte_212C2[];

int16 bufReadBytes(int8 *dst, int16 n) {
    int16 c;
    for (c = 0; c < n; c++) {
        if (word_21714 > 0x1FF) {
            sub_16C0E();
            word_21714 = 0;
        }
        *dst++ = byte_212C2[word_21714++];
    }
    return c;
}

/* seg000:0x67fd — clears byte +9 on 30 entries of the 0x5c-stride table
 * at dseg:0x2326. */
extern int8 byte_22326[];

void resetTableFlags(void) {
    int16 i;
    int16 e;
    for (i = 0; i < 0x1E; i++) {
        e = 0x2326 + i * 0x5C;
        ((int8 *)e)[9] = 0;
    }
}

/* seg000:0x5414 — same 0x200-byte file-buffer drain as bufReadBytes, but
 * refills via sub_149A1(handle) — used while reading a specific file. */
extern void sub_149A1(int16 h);

int16 bufReadFile(int8 *dst, int16 n, int16 h) {
    int16 c;
    for (c = 0; c < n; c++) {
        if (word_21714 > 0x1FF) {
            sub_149A1(h);
            word_21714 = 0;
        }
        *dst++ = byte_212C2[word_21714++];
    }
    return c;
}

/* seg000:0x6208 — per-tick effect table walk (0x5c-stride, word_2367C
 * entries): if flag +0x5a set, f09 += f08; on wrap past 0xff fires
 * sub_16261(entry) and stores the wrapped byte. */
extern void sub_16261(int16 e);
extern int16 word_2367C;

struct TickEnt { char _p[8]; uint8 f08, f09; char _q[0x52]; int8 f5A; };

void tickEffectTable(void) {
    int16 i;
    int16 e;
    int16 t;
    for (i = 0; i < word_2367C; i++) {
        e = 0x2326 + i * 0x5C;
        if (((struct TickEnt *)e)->f5A != 0) {
            t = ((struct TickEnt *)e)->f09 + ((struct TickEnt *)e)->f08;
            if (t > 0xFF) {
                t -= 0x100;
                sub_16261(e);
            }
            ((struct TickEnt *)e)->f09 = t;
        }
    }
}

/* seg000:0x68fd — dispatch to overlay draw proc selected by byte_2B83E (0-3).
 * pa/pd are object pointers dereferenced for the call's w1/w4 args. */
extern uint8 drawModeSel;                    /* dseg:0xb83e */
extern void far ovl_47B(int16 w1, int16 w2, int16 w3, int16 w4,
                        int16 w5, int16 w6, int16 w7, int16 w8);
extern void far ovl_766(int16 w1, int16 w2, int16 w3, int16 w4,
                        int16 w5, int16 w6, int16 w7, int16 w8);
extern void far ovl_169(int16 w1, int16 w2, int16 w3, int16 w4,
                        int16 w5, int16 w6, int16 w7, int16 w8);
extern void far ovl_A65(int16 w1, int16 w2, int16 w3, int16 w4,
                        int16 w5, int16 w6, int16 w7, int16 w8);

void dispatchDrawMode(int16 *pa, int16 b, int16 c, int16 *pd,
                    int16 e, int16 f, int16 g, int16 h) {
    if (g == 0) return;
    if (h == 0) return;
    switch (drawModeSel) {
    case 0: ovl_47B(*pa, b, c, *pd, e, f, g, h); break;
    case 1: ovl_766(*pa, b, c, *pd, e, f, g, h); break;
    case 2: ovl_169(*pa, b, c, *pd, e, f, g, h); break;
    case 3: ovl_A65(*pa, b, c, *pd, e, f, g, h); break;
    }
}
