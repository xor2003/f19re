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
    int8 pad2a[0x02];
    uint16 field2c;                             /* 0x2c */
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

/* seg000:7248 — missionResult = terrain type at the aircraft's grid tile;
 * returns it (the sub_10010 dispatch tests ax). */
int16 sub_17248(void)
{
    int16 tx, ty;
    tx = commData->posX >> 11;
    ty = commData->posY >> 11;
    return (missionResult = gridFlags[ty][tx] & 3);
}

struct PilotRecEnd {                            /* far ptr word_2243E */
    int8  pad0[0x20];
    uint16 rank;                                /* 0x20 */
    int8  pad22[0x0c];
    uint16 bestScore;                           /* 0x2e */
    uint16 awardPoints;                         /* 0x30 */
    int32  totalScore;                          /* 0x32 */
    uint16 missionCount;                        /* 0x36 */
    uint16 field38;                             /* 0x38 — res-name tbl idx */
};
extern struct PilotRecEnd far *pilotRec;        /* word_2243E */

extern int8   byte_199F4;                       /* dseg:0x99f4 — display flag */
extern int16 *word_19704;                       /* dseg:0x5704 — page/font ptr */
extern int16  rankNames[];                      /* dseg:0x56e0 — per-rank name tbl */

extern uint8  target1Scored, target2Scored;     /* byte_237AB / byte_23B6E */
extern int16  promotionDone;                    /* word_2244A */
extern int16  awardCode;                        /* word_23516 dseg:0x9786 */
extern int16 *word_2379E;                       /* dseg:0x9a0e — string-table ptr */
extern int16 *word_1ED12;                       /* dseg:0x4f82 — window struct ptr */
extern int16  word_1E2A4, word_1E7BA, word_1E51A,
              word_1E8D4, word_1EA42;           /* random-msg counts */
extern int16  randomRange(int16 maxVal);        /* seg000:0x0cfe */
extern void   far gfx_setDac(int16 n);          /* 9D9:14A4 */
extern void   drawWrappedText(int16 *page, char *str, uint16 maxWidth,
                              int16 x, int16 y, int16 lineHeight); /* 0x965 */

/* MenuItem fields as END.EXE addresses them (0x32 stride) */
typedef struct {
    int16 hitX1, hitY1, hitX2, hitY2;           /* 0x00-0x06 */
    int16 colorX1, colorY1, colorX2, colorY2;   /* 0x08-0x0e */
    int16 colorTableIdx;                        /* 0x10 */
    int16 colorPair;                            /* 0x12 */
    int16 labelData1[5];                        /* 0x14 */
    int16 *pagePtr;                             /* 0x1e */
    int16 labelData2[4];                        /* 0x20 */
    int16 spriteNormal;                         /* 0x28 */
    int16 spriteBlink;                          /* 0x2a */
    int16 unk_2c;                               /* 0x2c */
    int16 state;                                /* 0x2e */
    uint16 flags;                               /* 0x30 */
} MenuItem;

extern int16  allocBuffer(int16 seg);           /* seg000:0x2e52 */
extern void   openDecodeClosePic(const char *name, int16 page); /* 0x1615 */
extern void   loadPicFromFileAt();              /* 0x15a6 — called w/ 2 args */
extern void   sub_10E50(int16 *page, int16 x1, int16 y1, int16 x2, int16 y2); /* clearRect dup */
extern int16  far gfx_getBufSize(void);         /* 9D9:13C3 */
extern void   far gfx_blitSprite(int16 *spr);   /* 9D9:13A5 */
extern int16  far misc_jump_5d_readJoy(int16 a);/* 9D9:1521 */
extern void   processMenuItems(MenuItem *items, int16 unused, int16 itemCount,
                               int16 cursorStartX, int16 cursorStartY, int16 *gfxPage);
extern int16  selectMenuItem(MenuItem *items, int16 unused, int16 itemCount,
                             int16 *inputState, int16 *gfxPage);
extern void   animateFlightPath(int16 *gfxPage);/* 0x4f7b */
extern int32  calcMissionScore(int16 n);        /* 0x5666 */

struct EvtItem {                                /* item/page recs, 0x20 stride */
    int16 *win;                                 /* +0x00 — window ptr */
    int8  pad02[0x18];
    int16 page;                                 /* +0x1a */
    int8  pad1c[4];
};
extern struct EvtItem evtItems[12];             /* dseg:0x58bc — word_1F64C */
extern int16 *word_1F664;                       /* dseg:0x58d4 — gfxPage */
extern int16  word_1F684, word_1F6A4;           /* dseg:0x58f4/0x5914 sprites */
extern int16 *word_1F856;                       /* dseg:0x5ac6 — inputState */
extern int16  word_1F858[];                     /* dseg:0x5ac8 — res-name tbl */
extern int16  word_1F860[];                     /* dseg:0x5ad0 — str tbl */
extern int16  word_2377E;                       /* dseg:0x99ee */
extern int8   byte_23796;                       /* dseg:0x9a06 */
extern int16  word_18EA6;                       /* dseg:0x8ea6 */
extern int8   byte_22450;                       /* dseg:0x86c0 — joy flag */
extern uint8  byte_1DF6A;                       /* dseg:0x41da — tick countdown */
extern int32  word_23B6A;                       /* dseg:0x9dda — score result */
extern int16  word_22F10;                       /* dseg:0x9180 */
extern int16  word_23C72;                       /* dseg:0x9ee2 — alloc'd seg */
extern MenuItem menuItems[2];                   /* dseg:0x5a56 — tally menu */

/* seg000:7334 — score-tally/menu screen: alloc res page, draw item labels,
 * menu loop driving animateFlightPath, then update pilot stats */
void sub_17334(void)
{
    char  m3[2], w1[3], m2[2], z2[2];
    int16 pos, cont, u1, y1, k3;

    m3[0] = 0xd;  m3[1] = 0;
    w1[0] = 9;    w1[1] = 0xa; w1[2] = 0;
    m2[0] = 0x8e; m2[1] = 0;
    z2[0] = 0x8f; z2[1] = 0;
    gfx_setFadeSteps(9);
    openDecodeClosePic((const char *)word_1F858[pilotRec->field38],
                       word_23C72 = allocBuffer(gfx_getBufSize()));
    pos = word_23C72;
    gfx_setFadeSteps(8);
    loadPicFromFileAt((const char *)0x5853, 1);
    evtItems[0].page = pos;  evtItems[1].page = pos;
    evtItems[2].page = pos;  evtItems[3].page = pos;
    evtItems[4].page = pos;  evtItems[5].page = pos;
    evtItems[6].page = pos;  evtItems[7].page = pos;
    evtItems[8].page = pos;  evtItems[9].page = pos;
    evtItems[10].page = pos; evtItems[11].page = pos;
    gfx_waitRetrace();
    sub_10E50(evtItems[0].win, 0, 0, 0x13f, 0xc7);
    gfx_blitSprite((int16 *)word_1F684);
    gfx_blitSprite((int16 *)word_1F6A4);
    evtItems[0].win[2] = 0xc;
    drawStringAt(evtItems[0].win, (const char *)0x585f, 0x1e, 1);
    evtItems[0].win[2] = 0;
    drawStringAt(evtItems[0].win, (const char *)0x5891, 0x6a, 1);
    evtItems[0].win[2] = 6;
    y1 = 0x96;
    u1 = 0;
    do {
        drawStringAt(evtItems[0].win, (const char *)word_1F860[u1], 0xec, y1);
        y1 += 0xa;
        u1++;
    } while (u1 < 2);
    k3 = 0;
    byte_23796 = 1;
    word_18EA6 = 0;
    gfx_commitPage();
    gfx_flipPage();
    setTimerIrqHandler();
    cont = 1;
    do {
        menuItems[k3].state = 2;
        processMenuItems(menuItems, word_2377E, 2, 0xfa,
                         0x97 + k3 * 0xa, word_1F664);
        k3 = selectMenuItem(menuItems, word_2377E, 2,
                             word_1F856, word_1F664);
        switch (k3) {
        case 0:
            animateFlightPath(word_1F664);
            if (byte_22450 == 1)
                k3 = 1;
            break;
        case 1:
            cont = 0;
            break;
        }
        if (commData->setupUseJoy == 1) {
            while (misc_jump_5d_readJoy(0) != 0)
                ;
            byte_1DF6A = 0;
            while (byte_1DF6A <= 5)
                ;
            while (misc_jump_5d_readJoy(0) != 0)
                ;
        }
    } while (cont != 0);
    restoreTimerIrqHandler();
    word_23B6A = calcMissionScore(word_22F10);
    if (commData->trainingFlag == 0) {
        pilotRec->awardPoints = word_23B6A;
        if (pilotRec->bestScore < (int16)word_23B6A)
            pilotRec->bestScore = (int16)word_23B6A;
        pilotRec->totalScore += word_23B6A;
    } else
        pilotRec->awardPoints = 0;
    freeBuffer(word_23C72);
}

/* seg000:6076 — end-of-mission summary panel: pick picture + random message
 * table by outcome flags, then the shared draw/input tail */
void sub_16076(void)
{
    int16 a0, a1, a2, a3, a4, a5, a6;

    if (commData->bailout != 0 || commData->trainingFlag == 1)
        return;
    gfx_setFadeSteps(0xa);
    gfx_waitRetrace();
    if (promotionDone == 0 && awardCode == 0 && target1Scored == 0
        && target2Scored == 0 && pilotRec->missionCount != 0x63) {
        openBlitClosePic((const char *)0x4eba, word_23C6C);
        if (word_18EA2 == 1) {
            loadFileSection((const char *)0x4ec6, word_23C78, word_23C7A);
            word_1295C = drawMapView(0x19, 2, word_23C76);
        }
        gfx_blitToCurrent(word_23C6C);
        word_2379E = (int16 *)0x4516;
        word_1ED12[2] = 0xf;
        drawWrappedText(word_1ED12, (char *)word_2379E[randomRange(word_1E2A4)],
                        0x10e, 0x28, 0xa8, 8);
    } else if ((target1Scored == 1 || target2Scored == 1)
               && missionResult == 0 && promotionDone == 0 && awardCode == 0) {
        openBlitClosePic((const char *)0x4ed0, word_23C6C);
        if (word_18EA2 == 1) {
            loadFileSection((const char *)0x4edc, word_23C78, word_23C7A);
            word_1295C = drawMapView(0x19, 2, word_23C76);
        }
        gfx_blitToCurrent(word_23C6C);
        word_2379E = (int16 *)0x4a2c;
        word_1ED12[2] = 0xf;
        drawWrappedText(word_1ED12, (char *)word_2379E[randomRange(word_1E7BA)],
                        0x10e, 0x28, 0xa8, 8);
    } else if (pilotRec->missionCount == 0x63 && pilotRec->rank == 6) {
        openBlitClosePic((const char *)0x4ee6, word_23C6C);
        if (word_18EA2 == 1) {
            loadFileSection((const char *)0x4ef2, word_23C78, word_23C7A);
            word_1295C = drawMapView(0x11, 1, word_23C76);
        }
        gfx_blitToCurrent(word_23C6C);
        word_2379E = (int16 *)0x4663;
        word_1ED12[2] = 0xf;
        drawWrappedText(word_1ED12, (char *)*word_2379E, 0x10e, 0x28, 0xa0, 8);
    } else if (pilotRec->missionCount == 0x63 && pilotRec->rank != 6) {
        openBlitClosePic((const char *)0x4efc, word_23C6C);
        if (word_18EA2 == 1) {
            loadFileSection((const char *)0x4f08, word_23C78, word_23C7A);
            word_1295C = drawMapView(0x11, 1, word_23C76);
        }
        gfx_blitToCurrent(word_23C6C);
        word_2379E = (int16 *)0x46f6;
        word_1ED12[2] = 0xf;
        drawWrappedText(word_1ED12, (char *)*word_2379E, 0x10e, 0x28, 0xa0, 8);
    } else if (promotionDone == 1 || awardCode != 0) {
        openBlitClosePic((const char *)0x4f12, word_23C6C);
        if (word_18EA2 == 1) {
            loadFileSection((const char *)0x4f1e, word_23C78, word_23C7A);
            word_1295C = drawMapView(0x11, 1, word_23C76);
        }
        gfx_blitToCurrent(word_23C6C);
        word_2379E = (int16 *)0x478c;
        word_1ED12[2] = 0xf;
        drawWrappedText(word_1ED12, (char *)word_2379E[randomRange(word_1E51A)],
                        0x10e, 0x28, 0xa0, 8);
    } else if (commData->field2c < 3) {
        openBlitClosePic((const char *)0x4f28, word_23C6C);
        if (word_18EA2 == 1) {
            loadFileSection((const char *)0x4f33, word_23C78, word_23C7A);
            word_1295C = drawMapView(0x19, 1, word_23C76);
        }
        gfx_blitToCurrent(word_23C6C);
        word_2379E = (int16 *)0x4b46;
        word_1ED12[2] = 0xf;
        drawWrappedText(word_1ED12, (char *)word_2379E[randomRange(word_1E8D4)],
                        0x10e, 0x28, 0xa8, 8);
    } else {
        openBlitClosePic((const char *)0x4f3c, word_23C6C);
        if (word_18EA2 == 1) {
            loadFileSection((const char *)0x4f47, word_23C78, word_23C7A);
            word_1295C = drawMapView(0x19, 1, word_23C76);
        }
        gfx_blitToCurrent(word_23C6C);
        word_2379E = (int16 *)0x4cb4;
        word_1ED12[2] = 0xf;
        drawWrappedText(word_1ED12, (char *)word_2379E[randomRange(word_1EA42)],
                        0x10e, 0x28, 0xa8, 8);
    }
    gfx_setDac(1);
    word_1ED12[2] = 9;
    drawStringAt(word_1ED12, (const char *)0x4f50, 0x64, 0xc1);
    gfx_commitPage();
    setTimerIrqHandler();
    waitForKeyOrJoy2();
    restoreTimerIrqHandler();
    if (word_18EA2 == 1)
        freeBuffer(word_226BC);
}

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

/* ==== seg000:5d1b — debrief map event popup: erase the previous icon, remap
 * flightRecords[cur]'s status to an icon kind, clamp the popup into the map
 * window quadrants, then blit the icon sprite. Called from drawMenuItem. ==== */
struct FlightLogRec {
    uint8 mapX, mapY;
    int8  status;
    int8  unitId;
    uint8 pad4, pad5;
};
extern struct FlightLogRec flightRecords[];      /* byte_22F14 */
extern int16  mapToScreenX(int16 v);             /* seg000:0x54ba */
extern int16  mapToScreenY(int16 v);             /* seg000:0x54cf */
extern int16  mapWinX1, mapWinY1;                /* word_1DF8E/90 */
extern uint8  popupVisible;                      /* byte_237AA dseg:0x9a1a */
extern int16  popupX, popupY;                    /* word_23C6E/70 */
extern uint8  slotInfoTable[];                   /* word_2245E */
extern int16  word_1E25C[], word_1E280[];        /* icon param tables */
struct UnitInfo { int16 w0; int16 pad02[8]; };
extern struct UnitInfo word_22A12[];             /* dseg:0x8c82 — 0x12 stride */
struct OrdType  { int16 f0; int16 pad02[5]; };
extern struct OrdType  word_1AAD8[];             /* dseg:0x0d48 — 0xc stride */
extern void   far gfx_copyRect(int16 a, int16 b, int16 c, int16 d,
                               int16 e, int16 f, int16 g, int16 h); /* 9D9:1422 */

void sub_15D1B(void)
{
    int16  n;
    uint16 m;

    if (popupVisible == 1) {
        gfx_copyRect(1, 0, 0x96, 0, popupX, popupY, 0x30, 0x28);
        popupVisible = 0;
    }
    n = flightRecords[word_18EA6].status & 0x3f;
    switch (n) {
    case 1:
        if (slotInfoTable[(flightRecords[word_18EA6].unitId & 0x7f) << 4] & 8)
            n = 0xf;
        else
            n = 0;
        break;
    case 2:
    case 12:
        n = 2;
        break;
    case 3:
        n = 1;
        break;
    case 4:
        n = 6;
        break;
    case 5:
        n = 3;
        break;
    case 6:
        n = 4;
        break;
    case 7:
        n = 5;
        break;
    case 8:
        if (word_18EA6 == 0) {
            n = 8;
        } else if (commData->landingType == 3) {
            byte_23796 = 1;
            n = 7;
        } else if (commData->landingType == 1) {
            byte_23796 = 1;
            n = 0xe;
        } else if (missionResult == 0) {
            byte_23796 = 1;
            n = 0xb;
        } else {
            byte_23796 = 1;
            n = 0xd;
        }
        break;
    case 9:
        break;
    case 10:
        n = 0xa;
        break;
    case 11:
        if (flightRecords[word_18EA6].status & 0x80) {
            m = 0;
        } else if (flightRecords[word_18EA6].status & 0x40) {
            m = 1;
        }
        if (word_1AAD8[word_22A12[m].w0].f0 == 3)
            n = 9;
        else
            n = 0xc;
        break;
    }
    if (mapToScreenX(flightRecords[word_18EA6].mapX) + mapWinX1 < 0x73 &&
        mapToScreenY(flightRecords[word_18EA6].mapY) + mapWinY1 < 0x59) {
        popupX = mapToScreenX(flightRecords[word_18EA6].mapX) + mapWinX1 + 0xa;
        popupY = mapToScreenY(flightRecords[word_18EA6].mapY) + mapWinY1 + 0xa;
    } else if (mapToScreenX(flightRecords[word_18EA6].mapX) + mapWinX1 >= 0x73 &&
               mapToScreenY(flightRecords[word_18EA6].mapY) + mapWinY1 < 0x59) {
        popupX = mapToScreenX(flightRecords[word_18EA6].mapX) + mapWinX1 - 0x3a;
        popupY = mapToScreenY(flightRecords[word_18EA6].mapY) + mapWinY1 + 0xa;
    } else if (mapToScreenX(flightRecords[word_18EA6].mapX) + mapWinX1 >= 0x73 &&
               mapToScreenY(flightRecords[word_18EA6].mapY) + mapWinY1 >= 0x59) {
        popupX = mapToScreenX(flightRecords[word_18EA6].mapX) + mapWinX1 - 0x3a;
        popupY = mapToScreenY(flightRecords[word_18EA6].mapY) + mapWinY1 - 0x28;
    } else {
        popupX = mapToScreenX(flightRecords[word_18EA6].mapX) + mapWinX1 + 0xa;
        popupY = mapToScreenY(flightRecords[word_18EA6].mapY) + mapWinY1 - 0x28;
    }
    gfx_copyRect(0, popupX, popupY, 1, 0, 0x96, 0x30, 0x28);
    gfx_copyRect(1, word_1E280[n], word_1E25C[n], 0, popupX, popupY, 0x30, 0x28);
    popupVisible = 1;
}
