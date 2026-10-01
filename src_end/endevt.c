/* END.EXE — debrief event-dispatch cluster (seg000:5d1b-8531 family + the
 * sub_10010 master driver). These are the per-event-type debrief renderers
 * the original END.EXE calls from its mission-result dispatch. */
#include "inttype.h"

struct CommDataEnd {                            /* far ptr word_23C66 */
    int8 pad20[0x20];
    int16 gfxInitResult;                        /* 0x20 */
    int8 pad22[0x02];
    int16 setupMono;                            /* 0x24 */
    int16 landingType;                          /* 0x26 */
    int16 bailout;                              /* 0x28 */
    int8 pad2a[0x04];
    uint16 missionTime;                         /* 0x2e */
    int16 trainingFlag;                         /* 0x30 */
    int8 pad32[0x02];
    uint8 commFlags34;                          /* 0x34 */
    int8 pad35;
    uint16 commField36;                         /* 0x36 */
    uint16 slotWpn[29];                         /* 0x38 */
    int16 setupUseJoy;                          /* 0x72 */
    uint16 posX;                                /* 0x74 — fixed-point (>>11 = tile) */
    uint16 posY;                                /* 0x76 — fixed-point (>>11 = tile) */
};
extern struct CommDataEnd far *commData;

extern int16  missionResult;                    /* word_2351C dseg:0x978c */
extern int8   gridFlags[][16];                  /* byte_2290A */

extern int16  word_18EA2;                       /* dseg:0x8ea2 — gfx init flag */
extern int16  word_23C6C;                       /* dseg:0x9edc — debrief res page */
extern int16  word_23C76, word_23C78, word_23C7A;/* dseg:0x9ee6/8/a */
extern int16  word_1295C;                       /* dseg:0x295c */
extern int16 *word_19806;                       /* dseg:0x5806 — window struct ptr */
extern int16  word_226BC;                       /* dseg:0x86bc — alloc'd seg */

extern void   openBlitClosePic(const char *name, int16 page);   /* seg000:0x15df */
extern int16  loadFileSection(const char *name, int16 b, int16 c); /* 0x12c6 */
extern int16  drawMapView(int16 viewY, int16 viewX, int16 sel);    /* 0x27f6 */
extern void   drawStringAt(int16 *pageNum, const char *str, int16 x, int16 y); /* 0x7c6 */
extern void   setTimerIrqHandler(void);         /* sub_13626 */
extern void   restoreTimerIrqHandler(void);     /* sub_13664 */
extern void   waitForKeyOrJoy2(void);           /* seg000:0x0702 */
extern void   freeBuffer(uint16 segment);       /* seg000:0x2e86 */
extern void   far gfx_setFadeSteps(int16 n);    /* 9D9:1481 */
extern void   far gfx_waitRetrace(void);        /* 9D9:14A9 */
extern int16  far gfx_blitToCurrent(int16 p);   /* 9D9:1440 */
extern void   far gfx_commitPage(void);         /* 9D9:14E0 */
extern void   far gfx_flipPage(void);           /* 9D9:14AE */

/* seg000:7248 — missionResult = terrain type at the aircraft's grid tile */
void sub_17248(void)
{
    int16 tx, ty;
    tx = commData->posX >> 11;
    ty = commData->posY >> 11;
    missionResult = gridFlags[ty][tx] & 3;
}

struct PilotRecEnd {                            /* far ptr word_2243E */
    int8  pad0[0x20];
    uint16 rank;                                /* 0x20 */
};
extern struct PilotRecEnd far *pilotRec;        /* word_2243E */

extern int8   byte_199F4;                       /* dseg:0x99f4 — display flag */
extern int16 *word_19704;                       /* dseg:0x5704 — page/font ptr */
extern int16  rankNames[];                      /* dseg:0x56e0 — per-rank name tbl */

extern void   mystrcpy(char *dst, const char *src);             /* 0x38ba */
extern void   mystrcat(char *dst, const char *src);             /* 0x3923 */
extern void   farStrcpy(char *dst, char far *src);              /* 0x38ec */
extern int16  stringWidth(int16 *item, uint8 *str);             /* 0x0a88 */
extern void   waitForKeyOrJoy(void);            /* seg000:0x067b */

/* seg000:7094 — draw "<rank><name>" centered; pilot-name debrief screen */
void sub_17094(void)
{
    int16 width;
    char  nameBuf[0x64];
    int16 page;
    char  tmpStr[0x14];

    byte_199F4 = 1;
    gfx_setFadeSteps(3);
    openBlitClosePic((const char *)0x56d6, word_23C6C);
    page = word_23C6C;
    gfx_waitRetrace();
    gfx_blitToCurrent(page);
    mystrcpy(nameBuf, (const char *)rankNames[pilotRec->rank]);
    farStrcpy(tmpStr, (char far *)pilotRec + 2);
    mystrcat(nameBuf, tmpStr);
    width = stringWidth(word_19704, (uint8 *)nameBuf);
    drawStringAt(word_19704, nameBuf, (int16)(((uint16)(0x84 - width)) >> 1) + 0xb9, 0xc1);
    gfx_commitPage();
    gfx_flipPage();
    waitForKeyOrJoy();
}

extern int16 *word_1981E;                       /* dseg:0x57ae — window struct ptr */
extern void   drawWrappedText(int16 *page, char *str, uint16 maxWidth,
                              int16 x, int16 y, int16 lineHeight); /* 0x965 */

/* seg000:714c — draw wrapped mission-text panel and wait for input */
void sub_1714C(void)
{
    int16 page;
    char  buf[0xc6];

    gfx_setFadeSteps(7);
    openBlitClosePic((const char *)0x5706, word_23C6C);
    page = word_23C6C;
    gfx_waitRetrace();
    if (word_18EA2 == 1) {
        loadFileSection((const char *)0x5711, word_23C78, word_23C7A);
        word_1295C = drawMapView(0x1c, 0x21, word_23C76);
    }
    gfx_blitToCurrent(page);
    mystrcpy(buf, (const char *)0x571a);
    mystrcat(buf, (const char *)0x5755);
    word_1981E[2] = 0xf;
    drawWrappedText(word_1981E, buf, 0xf0, 0x27, 0xa8, 7);
    word_1981E[2] = 9;
    drawStringAt(word_1981E, (const char *)0x577d, 0x64, 0xc1);
    word_1981E[2] = 0;
    gfx_commitPage();
    gfx_flipPage();
    setTimerIrqHandler();
    waitForKeyOrJoy2();
    restoreTimerIrqHandler();
    if (word_18EA2 == 1)
        freeBuffer(word_226BC);
}

/* seg000:7280 — draw the event-type map/scene panel and wait for input */
void sub_17280(void)
{
    int16 saved;
    gfx_setFadeSteps(2);
    openBlitClosePic((const char *)0x57c0, word_23C6C);
    saved = word_23C6C;
    gfx_waitRetrace();
    if (word_18EA2 == 1) {
        loadFileSection((const char *)0x57c9, word_23C78, word_23C7A);
        word_1295C = drawMapView(0x10, 0x5a, word_23C76);
    }
    gfx_blitToCurrent(saved);
    word_19806[2] = 1;
    drawStringAt(word_19806, (const char *)0x57d4, 0x87, 0xc1);
    word_19806[2] = 0;
    gfx_commitPage();
    gfx_flipPage();
    setTimerIrqHandler();
    waitForKeyOrJoy2();
    restoreTimerIrqHandler();
    if (word_18EA2 == 1)
        freeBuffer(word_226BC);
}
