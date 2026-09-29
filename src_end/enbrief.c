/* END.EXE — debrief/map routines (f15se2 enbrief.c lineage) */
#include "inttype.h"

/* MenuItem fields as END.EXE addresses them */
typedef struct {
    int16 hitX1;            /* 0x00 */
    int16 hitY1;            /* 0x02 */
    int16 hitX2;            /* 0x04 */
    int16 hitY2;            /* 0x06 */
    int16 colorX1;          /* 0x08 */
    int16 colorY1;          /* 0x0a */
    int16 colorX2;          /* 0x0c */
    int16 colorY2;          /* 0x0e */
    int16 colorTableIdx;    /* 0x10 */
    int16 colorPair;        /* 0x12 */
    int16 labelData1[5];    /* 0x14 */
    int16 *pagePtr;         /* 0x1e */
    int16 labelData2[4];    /* 0x20 */
    int16 spriteNormal;     /* 0x28 */
    int16 spriteBlink;      /* 0x2a */
    int16 unk_2c;           /* 0x2c */
    int16 state;            /* 0x2e */
    uint16 flags;           /* 0x30 */
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
extern int16 mapToScreenY(int16 v);
extern void drawClippedLine(int16 x1, int16 y1, int16 x2, int16 y2);
extern void mystrcpy(char *dst, const char *src);
extern void mystrcat(char *dst, const char *src);               /* seg000:0x3923 */
extern void my_ltoa(int32 value, char *buf);                    /* seg000:0x0acc textfmt.c */
extern void drawWrappedText(int16 *page, char *str, uint16 maxWidth, int16 x, int16 y, int16 lineHeight); /* seg000:0x0965 */
extern void drawStringAt(int16 *pageNum, const char *string, int16 x, int16 y); /* seg000:0x07c6 */
extern int16 stringWidth(int16 *item, uint8 *str);              /* seg000:0x0a88 */
extern void far gfx_copyRect(int16 a, int16 b, int16 c, int16 d, int16 e, int16 f, int16 g, int16 h);
extern void setTimerIrqHandler(void);       /* sub_13626 — int 21/35h+25h asm */
extern void restoreTimerIrqHandler(void);   /* sub_13664 — int 21/25h asm */

uint16 cursorX, cursorY;                    /* word_237A0, word_237A6 */
uint8 timerCounter;                         /* byte_1DF6A */
int16 colorAnimEnabled;                     /* word_22426 */
int16 selectedMenuItem;                     /* word_23512 */
uint8 inputChanged;                         /* byte_22422 */
uint8 enterPressed;                         /* byte_236AF */
uint8 joyRepeatFlag;                        /* byte_22428 */

extern void far gfx_commitPage(void);
extern void processDebriefInput(int16 *inputState, MenuItem *item, int16 *gfxPage);
extern void drawMenuItem(const MenuItem *items, uint16 index, int16 *gfxPage);
extern void blinkWidget(MenuItem *item, int16 *gfxPage);

#define MENUITEM_TYPE_MASK   0x0007
#define MENUITEM_SELECTABLE  0x0008
#define MENUITEM_ENABLED     0x0100
#define MENUITEM_HAS_SPRITE  0x0800
#define MENUITEM_SPRITE_BLINK 0x1000

#define KEYCODE_ENTER      0x000d
#define KEYCODE_ESC        0x001b
#define KEYCODE_ALTQ       0x1000
#define KEYCODE_LEFTARROW  0x4b00
#define KEYCODE_RIGHTARROW 0x4d00
#define KEYCODE_UPARROW    0x4800
#define KEYCODE_DNARROW    0x5000
#define JOY_DEADZONE_LO    0x4e
#define JOY_DEADZONE_HI    0xb2

struct CommDataEnd {                            /* far ptr word_23C66 */
    int8 pad20[0x20];
    int16 gfxInitResult;                        /* 0x20 */
    int8 pad22[0x02];
    int16 setupMono;                            /* 0x24 */
    int16 landingType;                          /* 0x26 */
    int16 bailout;                              /* 0x28 */
    int8 pad2a[0x06];
    int16 trainingFlag;                         /* 0x30 */
    int8 pad32[0x02];
    uint8 commFlags34;                          /* 0x34 */
    int8 pad35;
    uint16 commField36;                         /* 0x36 */
    uint16 slotWpn[29];                         /* 0x38 */
    int16 setupUseJoy;                          /* 0x72 */
};
extern struct CommDataEnd far *commData;

struct PilotRecEnd {                            /* far ptr word_2243E */
    int8  pad0[0x20];
    uint16 rank;                                /* 0x20 */
    uint16 flag22;                              /* 0x22 — one-shot award flag */
    uint16 award24, award26, award28, award2a;  /* 0x24-0x2a — tier counters */
    uint16 flag2c;                              /* 0x2c — 1200-pt award flag */
    int8  pad2e[2];
    uint16 awardPoints;                         /* 0x30 — award threshold pool */
    int32 totalScore;                           /* 0x32 */
    uint16 missionCount;                        /* 0x36 */
    int8  pad38[0x0c];
    uint16 flag44;                              /* 0x44 — training award flag */
    uint16 flag46, flag48, flag4a, flag4c;      /* 0x46-0x4c — mission ribbon flags */
};
extern struct PilotRecEnd far *pilotRec;        /* word_2243E — init to comm+0x120e */
extern uint8  promoScreenOpen;                  /* byte_23784 */
extern uint8  target1Scored, target2Scored;     /* byte_237AB / byte_23B6E */
extern int16  promotionDone;                    /* word_2244A */
extern int16  promotionPending;                 /* word_2244E */
extern int16  awardTrained;                     /* word_2243C */
extern int16  award6Flag;                       /* word_22C2E */
extern int16  missionRibbon;                    /* word_22444 */
extern int16  awardQueued;                      /* word_22430 */
extern int16  awardCode;                        /* word_23516 */
extern uint16 promoScoreMin[];                  /* word_20456: 300,1125,3000,7000,16000,27720 */
extern uint16 promoAvgMin[];                    /* word_20462: 100,150,200,250,280,280 */
extern uint16 promoMissionsMin[];               /* word_2046E: 2,5,10,20,40,99 */

struct BlinkSprite {
    int16 pad00;
    int16 srcX;                 /* +2 */
    int16 pad04;
    int16 pad06;
    int16 dstX;                 /* +8 */
    int16 dstY;                 /* +a */
};
struct FlightLogRec {
    uint8 mapX, mapY;
    int8 status;
    int8 unitId;
    uint8 pad4, pad5;
};

uint16 *colorTablePtr;                       /* word_22420 */
int16 colorAnimIdx;                          /* word_22424 */
uint8 timerCounter2;                         /* byte_1DF6C */
uint8 timerCounter3;                         /* byte_1DF6D */
uint8 animDone;                              /* byte_2242A */
uint8 spriteToggle;                          /* byte_22429 */
uint8 joyAxisX, joyAxisY;                    /* byte_1B6CE, byte_1B6CF */
uint8 quitFlag;                              /* byte_1B6D2 */

extern int16 curRecordIdx;                   /* word_22C36 */
extern uint16 colorStyleTable[];             /* 0x41DE */
extern struct FlightLogRec flightRecords[];  /* byte_22F14 */
extern uint8 slotInfoTable[];                /* word_2245E */
extern int16 mapWinX1, mapWinY1, mapWinX2, mapWinY2; /* word_1DF8E/90/92/94 debrief map window */
extern struct BlinkSprite *spriteAirBlink;   /* word_1F6E4 */
extern struct BlinkSprite *spriteSamBlink;   /* word_1F764 */
extern struct BlinkSprite *spriteGroundBlink;/* word_1F724 */
extern struct BlinkSprite *spriteWaypointBlink;/* word_1F7E4 */
extern struct BlinkSprite *spriteAir;        /* word_1F6C4 */
extern struct BlinkSprite *spriteGround;     /* word_1F704 */
extern struct BlinkSprite *spriteSam;        /* word_1F744 */
extern struct BlinkSprite *spriteWaypoint;   /* word_1F7C4 */

/* drawMenuItem (seg000:0x44a8) globals */
extern int16  totalFlightRecords;            /* word_22B10 dseg:0x9180 */
extern int32  missionScore;                  /* dword dseg:0x9DDA */
extern uint8  ejectedFlag;                   /* byte_23D96 dseg:0x9A06 */
extern uint8  popupVisible;                  /* byte_23DAA dseg:0x9A1A */
extern int16  popupX, popupY;                /* word_23E6E/70 dseg:0x9EDE/0x9EE0 */
extern int16  prevDrawX, prevDrawY;          /* word_23D9A/23DA8 dseg:0x9A0A/0x9A18 */
extern int16  missionResult;                 /* word_23B1C dseg:0x978C */
extern char   scoreString[];                 /* byte_22A2E dseg:0x8C9E */
extern int16  flightTimeTable[];             /* word_22B12 dseg:0x9182 */
extern char  *worldStrings[];                /* dseg:0x9A22 near-ptr table */
extern struct BlinkSprite *spriteMapArea;    /* word_?? dseg:0x58F4 */

struct WorldObjEnd {                         /* dseg:0x86C6, stride 0x10 */
    int16 unitRef;                           /* +0x00 */
    int8  pad02[0x0c];
    int16 objectIdx;                         /* +0x0e */
};
extern struct WorldObjEnd worldObjects[];

struct PlaneNameEnd { char name[0x20]; };    /* dseg:0x0198, stride 0x20 */
extern struct PlaneNameEnd planeArray[];
struct SamNameEnd { char name[0x12]; };      /* dseg:0x03F8, stride 0x12 */
extern struct SamNameEnd samWeaponTable[];
extern char wpnNames[][0x1a];                /* dseg:0x0726, stride 0x1a */

extern void sub_10E50(int16 *page, int16 x1, int16 y1, int16 x2, int16 y2); /* clearRect dup */
extern int32 sub_15666(int16 n);              /* calcMissionScore — skeleton */
extern void sub_15D1B(void);                  /* event popup — skeleton */

extern void far pollJoystick(void);          /* 9C7:2F */
extern int16 far misc_jump_5a_keybuf(void);
extern int16 far misc_jump_5b_getkey(void);
extern int16 far misc_jump_5d_readJoy(int16 a);
extern void far gfx_blitSprite(struct BlinkSprite *spr);   /* 9D9:13A5 */
extern void cleanup(void);                   /* sub_10398 */
extern void restoreCbreakHandler(void);      /* sub_11264 */
extern void exit(int16 code);                /* sub_18AD2 */
extern void drawEventSprite(uint16 rec);      /* sub_14DDE */

/* seg000:0x3ff5 */
int16 isPointInRect(MenuItem *p) {
    if (p->hitX1 <= cursorX && p->hitX2 >= cursorX &&
        p->hitY1 <= cursorY && p->hitY2 >= cursorY)
        return 1;
    else
        return 0;
}

/* seg000:0x54ba */
int16 mapToScreenX(uint8 mapCoord) {
    return ((uint16)mapCoord << 7) / 146;
}

/* seg000:0x54cf */
int16 mapToScreenY(uint8 mapCoord) {
    return ((uint16)mapCoord << 7) / 0xc3;
}

/* seg000:0x564f */
void drawMapPixel(int16 x, int16 y, int16 color) {
    (void)color;
    drawClippedLine(x, y, x, y);
}

/* seg000:0x0cd9 */
void timerWait(uint16 ticks) {
    timerCounter = 0;
    setTimerIrqHandler();
    while (ticks >= timerCounter)
        ;
    restoreTimerIrqHandler();
}

/* seg000:0x3b8c */
void processMenuItems(MenuItem *items, int16 unused, int16 itemCount, int16 cursorStartX, int16 cursorStartY, int16 *gfxPage) {
    char p[2]; char a[2]; int16 b; char c[2]; int16 d; int16 e; char f[2];
    int16 g; int16 h; int16 i; int16 j; int16 k; int16 l; int16 m; int16 n; int16 o;
    int16 w;
    register int16 *ip;
    (void)unused;
    (void)b; (void)e; (void)g; (void)h; (void)i; (void)j;
    (void)k; (void)l; (void)m; (void)n; (void)o; (void)w;
    p[0] = 0x0d; p[1] = 0;
    c[0] = 0x89; c[1] = 0;
    a[0] = 0x8d; a[1] = 0;
    f[0] = 0x80; f[1] = 0;
    d = 0;
    goto TEST;
    for (;;) {
NORM:   ip = &items[d].state;
        if (*ip != 3) *ip = 0;
STEP:   d++;
TEST:   if (d >= itemCount) break;
        ip = &items[d].state;
        if (*ip != 2) goto NORM;
        selectedMenuItem = d;
        *ip = 0;
        blinkWidget(&items[d], gfxPage);
        drawMenuItem(items, d, gfxPage);
        goto STEP;
    }
    cursorX = cursorStartX;
    cursorY = cursorStartY;
}

/* seg000:0x3c2e */
int16 selectMenuItem(MenuItem *items, int16 unused, int16 itemCount, int16 *inputState, int16 *gfxPage) {
    char p[2]; int16 a; int16 b; char c[2]; int16 d; char e[2]; int16 f;
    int16 g; char h[2]; int16 i; char j[12]; int16 k; int16 l; int16 m; int16 n; int16 o;
    (void)unused;
    (void)d; (void)g; (void)j; (void)k; (void)l; (void)m; (void)n; (void)o;
    p[0] = 0x0d; p[1] = 0;
    e[0] = 0x89; e[1] = 0;
    c[0] = 0x8d; c[1] = 0;
    h[0] = 0x80; h[1] = 0;
    gfx_commitPage();
    colorAnimEnabled = 0;
    i = 0;
    while (isPointInRect(&items[i]) == 0 && i < itemCount)
        i++;
    joyRepeatFlag = 0;
    for (;;) {
        do {
            gfx_commitPage();
            if ((items[i].flags & MENUITEM_ENABLED) == 0) {
                colorAnimEnabled = 1;
            }
            processDebriefInput(inputState, &items[i], gfxPage);
        } while (inputChanged == 0 && enterPressed == 0);
        if (enterPressed != 0) {
            if (i != selectedMenuItem) {
                i = 0;
                while (isPointInRect(&items[i]) == 0 && i < itemCount)
                    i++;
            }
            if (items[selectedMenuItem].colorTableIdx == 0) {
                b = 0x0b;
                a = 9;
                gfx_switchColor(gfxPage, items[selectedMenuItem].colorX1, items[selectedMenuItem].colorY1, items[selectedMenuItem].colorX2, items[selectedMenuItem].colorY2, 0x0b, 9);
                b = 3;
                gfx_switchColor(gfxPage, items[selectedMenuItem].colorX1, items[selectedMenuItem].colorY1, items[selectedMenuItem].colorX2, items[selectedMenuItem].colorY2, 3, a);
                b = 0x0d;
                gfx_switchColor(gfxPage, items[selectedMenuItem].colorX1, items[selectedMenuItem].colorY1, items[selectedMenuItem].colorX2, items[selectedMenuItem].colorY2, 0x0d, a);
            }
            return i;
        }
        i = 0;
        while (isPointInRect(&items[i]) == 0 && i < itemCount)
            i++;
        if (i != selectedMenuItem) {
            if ((items[i].flags & MENUITEM_SELECTABLE) != 0) {
                for (f = 0; f < itemCount; f++) {
                    if (items[f].state != 0 &&
                        items[i].unk_2c == items[f].unk_2c) {
                        blinkWidget(&items[f], gfxPage);
                    }
                }
                if (items[selectedMenuItem].colorTableIdx == 0) {
                    b = 9;
                    a = 6;
                    gfx_switchColor(gfxPage, items[selectedMenuItem].colorX1, items[selectedMenuItem].colorY1, items[selectedMenuItem].colorX2, items[selectedMenuItem].colorY2, 9, 6);
                    b = 3;
                    gfx_switchColor(gfxPage, items[selectedMenuItem].colorX1, items[selectedMenuItem].colorY1, items[selectedMenuItem].colorX2, items[selectedMenuItem].colorY2, 3, a);
                    b = 0x0d;
                    gfx_switchColor(gfxPage, items[selectedMenuItem].colorX1, items[selectedMenuItem].colorY1, items[selectedMenuItem].colorX2, items[selectedMenuItem].colorY2, 0x0d, a);
                    b = 0x0b;
                    gfx_switchColor(gfxPage, items[selectedMenuItem].colorX1, items[selectedMenuItem].colorY1, items[selectedMenuItem].colorX2, items[selectedMenuItem].colorY2, 0x0b, a);
                }
                if (items[selectedMenuItem].colorTableIdx == 1) {
                    b = 8;
                    a = 7;
                    gfx_switchColor(gfxPage, items[selectedMenuItem].colorX1, items[selectedMenuItem].colorY1, items[selectedMenuItem].colorX2, items[selectedMenuItem].colorY2, 8, 7);
                }
                blinkWidget(&items[i], gfxPage);
            }
            selectedMenuItem = i;
            drawMenuItem(items, i, gfxPage);
        }
    }
    return i;
}

/* seg000:0x401d */
void processDebriefInput(int16 *cursorBounds, MenuItem *menuItem, int16 *gfxPage) {
    int16 a;
    int16 b;
    int16 c;
    int16 d;
    int16 e;
    int8 f;
    int16 g;
    int16 h;
    int16 i;
    (void)a; (void)g; (void)i;

    colorTablePtr = (uint16 *)((uint16)menuItem->colorTableIdx * 14 + (int16)colorStyleTable);
    timerCounter2 = 0;
    d = e = 0;
    inputChanged = enterPressed = animDone = f = 0;
    if (joyRepeatFlag == 1) {
        timerCounter = 0;
        f = 1;
    }

    if (commData->setupUseJoy == 1) {
        d = misc_jump_5d_readJoy(0);
        e = misc_jump_5d_readJoy(1);
        pollJoystick();
    }

    for (;;) {
        if ((int8)misc_jump_5a_keybuf() == 0
            || d != 0
            || e != 0
            || joyAxisX < JOY_DEADZONE_LO
            || joyAxisX > JOY_DEADZONE_HI
            || joyAxisY < JOY_DEADZONE_LO
            || joyAxisY > JOY_DEADZONE_HI) {
            if (f != 1)
                goto input_done;
        }
        if (joyRepeatFlag == 1) {
            if (timerCounter > 0x0f) {
                f = 0;
                joyRepeatFlag = 0;
            }
        }

        if (commData->setupUseJoy == 1) {
            d = misc_jump_5d_readJoy(0);
            e = misc_jump_5d_readJoy(1);
            pollJoystick();
        }

        if (quitFlag != 0) {
            cleanup();
            restoreCbreakHandler();
            exit(0);
        }

        if (colorAnimEnabled == 1) {
            if (timerCounter2 > 6) {
                timerCounter2 = 0;
                c = colorTablePtr[colorAnimIdx + 1] >> 4;
                b = colorTablePtr[colorAnimIdx + 1] & 0xf;
                gfx_switchColor(gfxPage, menuItem->colorX1, menuItem->colorY1,
                                menuItem->colorX2, menuItem->colorY2, c, b);
                colorAnimIdx++;
                colorAnimIdx = (uint16)colorAnimIdx % *colorTablePtr;
            }
        }

        if (!(menuItem->flags & MENUITEM_HAS_SPRITE)) continue;
        if (!(menuItem->flags & MENUITEM_SPRITE_BLINK)) continue;
        if (timerCounter3 <= 0x12) continue;
        timerCounter3 = 0;
        if (spriteToggle != 0) {
            switch (flightRecords[curRecordIdx].status & 0x3f) {
            case 1:
            case 12:
                spriteAirBlink->dstX = mapToScreenX(flightRecords[curRecordIdx].mapX) + mapWinX1 - 2;
                spriteAirBlink->dstY = mapToScreenY(flightRecords[curRecordIdx].mapY) + mapWinY1 - 2;
                if (slotInfoTable[(flightRecords[curRecordIdx].unitId & 0x7f) << 4] & 8) {
                    spriteAirBlink->srcX = 0x11e;
                } else {
                    spriteAirBlink->srcX = 0x12d;
                }
                gfx_blitSprite(spriteAirBlink);
                break;
            case 2:
                spriteSamBlink->dstX = mapToScreenX(flightRecords[curRecordIdx].mapX) + mapWinX1 - 2;
                spriteSamBlink->dstY = mapToScreenY(flightRecords[curRecordIdx].mapY) + mapWinY1 - 2;
                gfx_blitSprite(spriteSamBlink);
                break;
            case 4:
            case 5:
            case 6:
            case 7:
            case 10:
            case 11:
                spriteWaypointBlink->dstX = mapToScreenX(flightRecords[curRecordIdx].mapX) + mapWinX1;
                spriteWaypointBlink->dstY = mapToScreenY(flightRecords[curRecordIdx].mapY) + mapWinY1;
                gfx_blitSprite(spriteWaypointBlink);
                break;
            case 3:
            case 8:
                spriteGroundBlink->dstX = mapToScreenX(flightRecords[curRecordIdx].mapX) + mapWinX1 - 2;
                spriteGroundBlink->dstY = mapToScreenY(flightRecords[curRecordIdx].mapY) + mapWinY1 - 2;
                gfx_blitSprite(spriteGroundBlink);
                break;
            case 9:
            default:
                break;
            }
        } else {
            drawEventSprite(curRecordIdx);
        }
        spriteToggle = (spriteToggle == 0);
    }

input_done:
    if ((int8)misc_jump_5a_keybuf() == 0) {
        h = misc_jump_5b_getkey();
    } else {
        if (d == 1) {
            h = KEYCODE_ENTER;
        } else if (e == 1) {
            h = KEYCODE_ESC;
        } else if (joyAxisX < JOY_DEADZONE_LO) {
            h = KEYCODE_LEFTARROW;
            joyRepeatFlag = 1;
        } else if (joyAxisX > JOY_DEADZONE_HI) {
            h = KEYCODE_RIGHTARROW;
            joyRepeatFlag = 1;
        } else if (joyAxisY < JOY_DEADZONE_LO) {
            h = KEYCODE_UPARROW;
            joyRepeatFlag = 1;
        } else if (joyAxisY > JOY_DEADZONE_HI) {
            h = KEYCODE_DNARROW;
            joyRepeatFlag = 1;
        }
    }

    if ((int8)h == KEYCODE_ENTER) {
        enterPressed = 1;
    }
    if (h == KEYCODE_ALTQ) {
        quitFlag = 1;
        enterPressed = 1;
    }
    if (h == KEYCODE_UPARROW) {
        cursorY -= cursorBounds[1];
        if (cursorBounds[4] > (int16)cursorY) {
            cursorY = cursorBounds[4];
        }
        inputChanged = 1;
    }
    if (h == KEYCODE_DNARROW) {
        cursorY += cursorBounds[1];
        if (cursorY > (uint16)cursorBounds[5]) {
            cursorY = cursorBounds[5];
        }
        inputChanged = 1;
    }
    if (h == KEYCODE_RIGHTARROW) {
        cursorX += cursorBounds[0];
        if (cursorX > (uint16)cursorBounds[3]) {
            cursorX = cursorBounds[3];
        }
        inputChanged = 1;
    }
    if (h == KEYCODE_LEFTARROW) {
        cursorX -= cursorBounds[0];
        if (cursorBounds[2] > (int16)cursorX) {
            cursorX = cursorBounds[2];
        }
        if (cursorBounds[4] > (int16)cursorY) {
            cursorX += cursorBounds[0];
        }
        inputChanged = 1;
    }

    if (menuItem->flags & MENUITEM_HAS_SPRITE) {
        if (menuItem->flags & MENUITEM_SPRITE_BLINK) {
            drawEventSprite(curRecordIdx);
        }
    }
}

/* ==== seg000:0x4dde drawEventSprite — position + blit the always-on sprite
 * for flightRecords[rec].status&0x3f into the debrief map window. Jump-table
 * switch; the gfx_blitSprite tail is shared across cases. ==== */
void drawEventSprite(uint16 rec) {
    switch (flightRecords[rec].status & 0x3f) {
    case 1:
    case 12:
        spriteAir->dstX = mapToScreenX(flightRecords[rec].mapX) + mapWinX1 - 2;
        spriteAir->dstY = mapToScreenY(flightRecords[rec].mapY) + mapWinY1 - 2;
        if (slotInfoTable[(flightRecords[curRecordIdx].unitId & 0x7f) << 4] & 8) {
            spriteAir->srcX = 0x11e;
        } else {
            spriteAir->srcX = 0x12d;
        }
        gfx_blitSprite(spriteAir);
        break;
    case 2:
        spriteSam->dstX = mapToScreenX(flightRecords[rec].mapX) + mapWinX1 - 2;
        spriteSam->dstY = mapToScreenY(flightRecords[rec].mapY) + mapWinY1 - 2;
        gfx_blitSprite(spriteSam);
        break;
    case 4:
    case 5:
    case 6:
    case 7:
    case 10:
    case 11:
        spriteWaypoint->dstX = mapToScreenX(flightRecords[rec].mapX) + mapWinX1;
        spriteWaypoint->dstY = mapToScreenY(flightRecords[rec].mapY) + mapWinY1;
        gfx_blitSprite(spriteWaypoint);
        break;
    case 3:
    case 8:
        spriteGround->dstX = mapToScreenX(flightRecords[rec].mapX) + mapWinX1 - 2;
        spriteGround->dstY = mapToScreenY(flightRecords[rec].mapY) + mapWinY1 - 2;
        gfx_blitSprite(spriteGround);
        break;
    case 9:
    default:
        break;
    }
}

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
        (uint16)sx >= (uint16)mapWinX1 &&
        (uint16)sx < (uint16)mapWinX2 &&
        (uint16)sy >= (uint16)mapWinY1 &&
        (uint16)sy < (uint16)mapWinY2) {
        drawMapPixel(sx, sy, color);
    }
}

void drawClippedLineEx(int16 x1, int16 y1, int16 x2, int16 y2, int16 wx1, int16 wx2, int16 wy1, int16 wy2, int16 flag) {
    int16 w, h;
    (void)flag;
    w = wx2 - wx1;
    h = wy2 - wy1;
    gfx_setBlitOffset(gfx_calcRowAddr(wx1, wy1));
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
    drawClippedLineEx(x1, y1, x2, y2, mapWinX1, mapWinX2, mapWinY1, mapWinY2, 1);
}

void drawFlightLine(int16 p1, int16 p2, int16 p3, int16 p4) {
    drawClippedLineEx(mapToScreenX(p1), mapToScreenY(p2), mapToScreenX(p3), mapToScreenY(p4), mapWinX1, mapWinX2, mapWinY1, mapWinY2, 1);
}

/* ==== seg000:0x52f9 drawFlightPath — draw the mission route polyline over
 * flightRecords (loop1: lines), then the event sprites (loop2).
 * Returns final index - 1. ==== */
extern void far gfx_setColor(uint8 c);          /* 9D9:13F5 (slot 0x21) */

uint16 drawFlightPath(int16 gfxPage, uint16 maxRecord) {
    int16 p, a, b, c, d, e, f, g, h, i, j, k, l, m, n;
    (void)e; (void)f; (void)g; (void)h; (void)i;
    (void)j; (void)k; (void)l; (void)m; (void)n;
    a = -1;
    while ((flightRecords[++a].status & 0x3f) != 0) {
        if ((uint16)a > maxRecord)
            goto drawSprites;
        gfx_setColor(0);
        if (a == 0) {
            plotMapPoint(flightRecords[0].mapX, flightRecords[0].mapY, 0, 0);
            b = flightRecords[0].mapX;
            d = flightRecords[0].mapY;
        } else {
            p = flightRecords[a].mapX;
            c = flightRecords[a].mapY;
            drawFlightLine(p, c, b, d);
            b = p;
            d = c;
        }
    }
drawSprites:
    a = -1;
    while ((flightRecords[++a].status & 0x3f) != 0) {
        if ((uint16)a > maxRecord)
            goto done;
        if ((flightRecords[a].status & 0x3f) != 9)
            drawEventSprite(a);
    }
done:
    a--;
    return a;
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

/* ==== seg000:0x27f6 — set the map view origin (screen cell col/row via
 * mapViewX1/mapViewY1) then run the big map view dispatcher runMapView(sel);
 * return its status as 1/0. ==== */
extern int16 runMapView(int16 sel);                     /* 0x2192 */

int16 drawMapView(int16 viewY, int16 viewX, int16 sel) {
    mapViewX1 = viewX;
    mapViewY1 = viewY;
    if (runMapView(sel))
        return 1;
    return 0;
}

/* ==== seg000:0x281b serviceTick — every-7th-tick record animation gate.
 * Naked fn (no bp frame): no args, no pushes. START sub_161F1 twin. ==== */
extern uint8 tickByte;                                  /* byte_1DF6B */
extern int16 tickArm;                                   /* word_1C6EC */
extern void tickRecords(void);                          /* 0x2832 */

void serviceTick(void) {
    if (tickByte > 6) {
        tickByte = 0;
        if (tickArm != 0)
            tickRecords();
    }
}

/* ==== seg000:0x2832 — walk 0x5C-stride record table at dseg+0x295e;
 * for flagged records sum bytes +8/+9, wrap >0xff by -0x100, call the
 * per-record worker sub_1288B(recOff), store sum back at +9. ==== */
extern uint8 recActive[];                               /* byte_19DEA = rec+0x5a */
extern uint8 recField9[];                               /* word_19D99 = rec+9 */
extern uint8 recField8[];                               /* word_19D98 = rec+8 */
extern int16 recCount;                                  /* word_3CB4 */
extern void sub_1288B(int16 recOff);

void tickRecords(void) {
    int16 rp, total, i;
    for (i = 0; i < recCount; i++) {
        rp = i * 0x5C + 0x295E;
        if (recActive[rp] != 0) {
            total = recField9[rp] + recField8[rp];
            if (total > 0xFF)
                total -= 0x100;
            sub_1288B(rp);
            recField9[rp] = (uint8)total;
        }
    }
}

/* ==== seg000:0x2cc9 — parse a '(…|…)'/':N' command char stream via the
 * cursor pointer pp (read col idx, advance *pp each step).
 * Returns 0 on ')', 1 on '|', N-1 after ':N', skips nested parens. ==== */
int16 parseCmd(uint8 **pp, int16 idx) {
    int8 c, ch2;
    int16 dig, depth, i;
    i = 0;
    for (;;) {
        c = (*pp)[idx];
        (*pp)++;
        if (c == ')')
            return 0;
        if (c == '|')
            return 1;
        if (c == ':') {
            dig = 0;
            goto test;
        body:
            if (ch2 > '9')
                goto done;
            dig = dig * 0xA + ch2 - '0';
            (*pp)++;
        test:
            if ((ch2 = (*pp)[idx]) >= '0')
                goto body;
        done:
            return dig - 1;
        }
        if (c != '(')
            continue;
        depth = 1;
        do {
            depth += ((*pp)[idx] == '(');
            depth -= ((*pp)[idx] == ')');
            (*pp)++;
        } while (depth > 0);
    }
}

/* ==== seg000:0x2d8d — deactivate records whose bbox intersects the
 * (x,y,w,h) rect shifted by the map view origin ==== */
struct MapRect {
    int16 rx, ry, rw, rh;                       /* +0,+2,+4,+6 */
    uint8 pad[0x52];
    uint8 active;                               /* +0x5a */
    uint8 padEnd;
};
extern struct MapRect recTable[];               /* record table at 0x295e */

void clearActiveInRect(int16 x, int16 y, int16 w, int16 h) {
    int16 c, d, e, i, xmax;
    struct MapRect *rp;
    x -= mapViewX1;
    y -= mapViewY1;
    for (i = 0; i < recCount; i++) {
        rp = &recTable[i];
        if (rp->rx > x)
            xmax = recTable[i].rx;
        else
            xmax = x;
        if (rp->ry > y)
            e = rp->ry;
        else
            e = y;
        d = (rp->rw + rp->rx <= x + w) ? rp->rw + rp->rx : x + w;
        c = (rp->ry + rp->rh <= y + h) ? rp->ry + rp->rh : y + h;
        if (xmax < d && e < c)
            rp->active = 0;
    }
}

/* ==== seg000:0x2e27 — clear field9 of all 30 records ==== */
void resetRecField9(void) {
    int16 r, i;
    for (i = 0; i < 0x1E; i++) {
        r = i * 0x5C + 0x295E;
        recField9[r] = 0;
    }
}

/* ==== seg000:0x8532 — promotion eligibility scan ====
 * Scans rank thresholds top-down while i >= rank; on qualification either
 * defers (award screen open) or commits the rank bump. Rank 5 (General)
 * requires exactly 99 missions. The cold gate block sits in the loop's
 * init-jump gap — reached only by backward jumps, hence the for(;;) label. */
void checkPromotion(void) {
    int16 i, j;
    if (commData->trainingFlag == 1)
        return;
    i = 5;
    goto test;
    for (;;) {
gate:
        if (pilotRec->rank >= 5)
            return;
        if (promoScreenOpen != 1)
            goto promote;
        promotionPending = 1;
        return;
step:
        i--;
test:
        if ((int16)pilotRec->rank <= i) {
            if ((uint32)promoScoreMin[i] <= (uint32)pilotRec->totalScore &&
                promoMissionsMin[i] <= pilotRec->missionCount &&
                (uint32)pilotRec->totalScore / (uint32)pilotRec->missionCount >=
                (uint32)promoAvgMin[i])
                goto qual;
            goto step;
        }
        return;
    }
qual:
    if (pilotRec->rank != 5)
        goto gate;
    if (pilotRec->missionCount == 99)
        goto promote;
    goto gate;
promote:
    promotionDone = 1;
    pilotRec->rank++;
}

/* ==== seg000:0x85e3 — award/medal eligibility check ====
 * Frameless: no locals, no args. Runs after checkPromotion in the debrief
 * flow. Mission ribbons (5-9/10+/30-59/60+ missions) only outside training;
 * the point-tier awards (codes 1-5) and the code-6 flag award queue via
 * awardQueued when the promo screen is up or in training, else awardCode. */
void checkAwardCodes(void) {
    if (pilotRec->flag44 == 0 && commData->trainingFlag == 1 &&
        (target1Scored == 1 || target2Scored == 1)) {
        pilotRec->flag44 = 1;
        awardTrained = 1;
    }
    if (commData->trainingFlag == 0)
        goto ribbons;
    goto noRibbons;
ribbons:
    if (pilotRec->missionCount >= 5 && pilotRec->missionCount < 10 &&
        pilotRec->flag46 == 0) {
        pilotRec->flag46 = 1;
        missionRibbon = 1;
    }
    if (pilotRec->missionCount >= 10 && pilotRec->flag48 == 0) {
        pilotRec->flag46 = 0;
        pilotRec->flag48 = 1;
        missionRibbon = 2;
    }
    if (pilotRec->missionCount >= 30 && pilotRec->missionCount < 60 &&
        pilotRec->flag4a == 0) {
        pilotRec->flag4a = 1;
        missionRibbon = 3;
    }
    if (pilotRec->missionCount >= 60 && pilotRec->flag4c == 0) {
        pilotRec->flag4a = 0;
        pilotRec->flag4c = 1;
        missionRibbon = 4;
    }
noRibbons:
    if (pilotRec->flag22 == 0 && commData->commField36 >= 7 &&
        (commData->commFlags34 & 8) != 0 && (commData->commFlags34 & 4) != 0) {
        pilotRec->flag22 = 1;
        award6Flag = 1;
        awardCode = 6;
    }
    if (pilotRec->flag2c == 0 && pilotRec->awardPoints > 0x4B0) {
        if (promoScreenOpen == 1 || commData->trainingFlag == 1) {
            awardQueued = 5;
            return;
        }
        awardCode = 5;
        pilotRec->flag2c = 1;
        return;
    }
    if (900 * pilotRec->award2a + 900 <= pilotRec->awardPoints) {
        if (promoScreenOpen == 1 || commData->trainingFlag == 1) {
            awardQueued = 4;
            return;
        }
        pilotRec->award2a++;
        awardCode = 4;
        return;
    }
    if (600 * pilotRec->award28 + 600 <= pilotRec->awardPoints) {
        if (promoScreenOpen == 1 || commData->trainingFlag == 1) {
            awardQueued = 3;
            return;
        }
        pilotRec->award28++;
        awardCode = 3;
        return;
    }
    if (300 * pilotRec->award26 + 300 <= pilotRec->awardPoints) {
        if (promoScreenOpen == 1 || commData->trainingFlag == 1) {
            awardQueued = 2;
            return;
        }
        pilotRec->award26++;
        awardCode = 2;
        return;
    }
    if (pilotRec->award24 < 9 && 100 * pilotRec->award24 + 100 <= pilotRec->awardPoints) {
        if (promoScreenOpen == 1 || commData->trainingFlag == 1) {
            awardQueued = 1;
            return;
        }
        pilotRec->award24++;
        awardCode = 1;
        return;
    }
}

/* ==== seg000:0x44a8 drawMenuItem — debrief detail panel. Type-7 items draw
 * the mission-complete summary (route replay + overall rating); blink items
 * draw the current flightRecords event text (switch on status&0x3f), the
 * PRIMARY/SECNDRY objective tags and the cumulative rating, then the "next
 * mission event" prompt. f15 enbrief.c drawMenuItem lineage. Locals
 * c,e,f,g,h,i,j,k,l are unused frame fillers matching the original frame. ==== */
void drawMenuItem(const MenuItem *items, uint16 index, int16 *gfxPage) {
    char p[2];
    char a[2];
    char b[2];
    char d[2];
    int16 c, e, f, g, h, i, j, k, l;
    uint16 m;
    char numBuf[4];
    uint16 n;

    p[0] = 0x0a;
    p[1] = 0;
    b[0] = 0x89;
    b[1] = 0;
    a[0] = 0x8d;
    a[1] = 0;
    d[0] = 0x80;
    d[1] = 0;
    (void)c; (void)e; (void)f; (void)g; (void)h; (void)i; (void)j; (void)k; (void)l;

    if ((items[index].flags & MENUITEM_HAS_SPRITE) != 0) {
        if ((items[index].flags & MENUITEM_TYPE_MASK) == 7) {
            sub_10E50(gfxPage, 0xeb, 0xa, 0x13f, 0x95);
            gfxPage[2] = 0;
            mystrcpy(scoreString, b);
            mystrcat(scoreString, "Press Selector to exit Debriefing");
            drawWrappedText(gfxPage, scoreString, 80, 240, 130, 8);
            sub_10E50(gfxPage, 0xf0, 0x64, 0x12c, 0x7e);
            if (popupVisible == 1) {
                gfx_copyRect(1, 0, 0x96, 0, popupX, popupY, 0x30, 0x28);
                popupVisible = 0;
            }
            curRecordIdx = 0;
            totalFlightRecords = drawFlightPath(gfxPage, 0x270f);
            missionScore = sub_15666(totalFlightRecords);
            mystrcpy(scoreString, "\x8d");
            mystrcat(scoreString, "OVERALL");
            n = stringWidth(gfxPage, scoreString);
            drawStringAt(gfxPage, scoreString, 0xe8 + (0x57 - n) / 2, 0x64);
            mystrcpy(scoreString, "MISSION RATING");
            n = stringWidth(gfxPage, scoreString);
            drawStringAt(gfxPage, scoreString, 0xe8 + (0x57 - n) / 2, 0x6c);
            mystrcpy(scoreString, "\x80");
            my_ltoa(missionScore, numBuf);
            mystrcat(scoreString, numBuf);
            n = stringWidth(gfxPage, scoreString);
            drawStringAt(gfxPage, scoreString, 0xe8 + (0x57 - n) / 2, 0x74);
            ejectedFlag = 1;
        }
        if ((items[index].flags & MENUITEM_SPRITE_BLINK) == 0)
            return;
        if (ejectedFlag == 1) {
            ejectedFlag = 0;
            popupVisible = 0;
            gfx_blitSprite(spriteMapArea);
            curRecordIdx = prevDrawX = prevDrawY = 0;
            sub_10E50(gfxPage, 0xeb, 0xa, 0x13f, 0x95);
            missionScore = sub_15666(0x100);
            mystrcpy(scoreString, "\x8d");
            mystrcat(scoreString, "OVERALL");
            n = stringWidth(gfxPage, scoreString);
            drawStringAt(gfxPage, scoreString, 0xe8 + (0x57 - n) / 2, 0x64);
            mystrcpy(scoreString, "MISSION RATING");
            n = stringWidth(gfxPage, scoreString);
            drawStringAt(gfxPage, scoreString, 0xe8 + (0x57 - n) / 2, 0x6c);
            mystrcpy(scoreString, "\x80");
            my_ltoa(missionScore, numBuf);
            mystrcat(scoreString, numBuf);
            n = stringWidth(gfxPage, scoreString);
            drawStringAt(gfxPage, scoreString, 0xe8 + (0x57 - n) / 2, 0x74);
        }
        sub_10E50(gfxPage, 0xeb, 0xa, 0x13f, 0x63);
        gfxPage[2] = 0x0d;
        mystrcpy(scoreString, "MISSION EVENT");
        n = stringWidth(gfxPage, scoreString);
        drawStringAt(gfxPage, scoreString, 0xe8 + (0x57 - n) / 2, 0x14);
        mystrcpy(scoreString, "TIME: \x80");
        mystrcat(scoreString, formatFlightTime(flightTimeTable[curRecordIdx * 3], numBuf));
        drawStringAt(gfxPage, scoreString, 0xf0, 0x1e);
        m = flightRecords[curRecordIdx].unitId & 0x7f;
        switch (flightRecords[curRecordIdx].status & 0x3f) {
        case 1:
        case 12:
            if (worldObjects[m].unitRef != 0) {
                mystrcpy(scoreString, worldStrings[worldObjects[m].unitRef]);
                mystrcat(scoreString, " ");
                mystrcat(scoreString, worldStrings[worldObjects[m].objectIdx & 0x7f]);
                mystrcat(scoreString, " destroyed");
            } else {
                mystrcpy(scoreString, worldStrings[worldObjects[m].objectIdx & 0x7f]);
                mystrcat(scoreString, " destroyed");
            }
            break;
        case 3:
            mystrcpy(scoreString, planeArray[m].name);
            mystrcat(scoreString, " ");
            mystrcat(scoreString, &planeArray[m].name[7]);
            mystrcat(scoreString, " shot down");
            break;
        case 2:
            mystrcpy(scoreString, worldStrings[m]);
            mystrcat(scoreString, " destroyed");
            break;
        case 11:
            mystrcpy(scoreString, "Cargo delivered");
            break;
        case 10:
            if (worldObjects[m].unitRef != 0) {
                mystrcpy(scoreString, worldStrings[worldObjects[m].unitRef]);
                mystrcat(scoreString, " ");
                mystrcat(scoreString, worldStrings[worldObjects[m].objectIdx & 0x7f]);
                mystrcat(scoreString, " photographed");
            } else {
                mystrcpy(scoreString, worldStrings[worldObjects[m].objectIdx & 0x7f]);
                mystrcat(scoreString, " photographed");
            }
            break;
        case 5:
            mystrcpy(scoreString, "Hit by ");
            mystrcat(scoreString, samWeaponTable[m].name);
            mystrcat(scoreString, " missile");
            break;
        case 7:
            if (worldObjects[m].unitRef != 0) {
                mystrcpy(scoreString, worldStrings[worldObjects[m].unitRef]);
                mystrcat(scoreString, " ");
                mystrcat(scoreString, worldStrings[worldObjects[m].objectIdx & 0x7f]);
                mystrcat(scoreString, " Track");
            } else {
                mystrcpy(scoreString, worldStrings[worldObjects[m].objectIdx & 0x7f]);
                mystrcat(scoreString, " Radar Track");
            }
            break;
        case 6:
            mystrcpy(scoreString, planeArray[m].name);
            mystrcat(scoreString, " ");
            mystrcat(scoreString, &planeArray[m].name[7]);
            mystrcat(scoreString, " Visual ID");
            break;
        case 4:
            mystrcpy(scoreString, wpnNames[commData->slotWpn[m]]);
            mystrcat(scoreString, " ");
            mystrcat(scoreString, wpnNames[commData->slotWpn[m]] + 0x0a);
            mystrcat(scoreString, " released");
            break;
        case 8:
            if (curRecordIdx == 0) {
                mystrcpy(scoreString, "Takeoff point:");
                if (worldObjects[targetBlock.waypointData].unitRef != 0) {
                    mystrcat(scoreString, worldStrings[worldObjects[targetBlock.waypointData].unitRef]);
                } else {
                    mystrcat(scoreString, worldStrings[(uint8)worldObjects[targetBlock.waypointData].objectIdx]);
                }
            } else {
                mystrcpy(scoreString, "Mission end:\n");
                switch (commData->landingType) {
                case 1:
                    mystrcat(scoreString, "Crashed");
                    break;
                case 2:
                    if (commData->bailout == 0 && missionResult != 0) {
                        mystrcat(scoreString, "Good Bailout");
                    } else if (commData->bailout == 0 && missionResult == 0) {
                        mystrcat(scoreString, "Captured");
                    } else {
                        mystrcat(scoreString, "Bailed & Died");
                    }
                    break;
                case 3:
                    mystrcat(scoreString, "Good Landing");
                    break;
                }
            }
            break;
        }
        drawWrappedText(gfxPage, scoreString, 80, 240, 0x26, 8);
        if ((uint8)flightRecords[curRecordIdx].status & 0x80) {
            mystrcpy(scoreString, "\x8c" "PRIMARY OBJECTIVE");
            n = stringWidth(gfxPage, scoreString);
            drawStringAt(gfxPage, scoreString, 0xe8 + (0x57 - n) / 2, gfxPage[5]);
        }
        if ((uint8)flightRecords[curRecordIdx].status & 0x40) {
            mystrcpy(scoreString, "\x8c" "SECNDRY OBJECTIVE");
            n = stringWidth(gfxPage, scoreString);
            drawStringAt(gfxPage, scoreString, 0xe8 + (0x57 - n) / 2, gfxPage[5]);
        }
        missionScore = sub_15666(curRecordIdx);
        mystrcpy(scoreString, "\x8d");
        mystrcat(scoreString, "CUMULATIVE");
        n = stringWidth(gfxPage, scoreString);
        drawStringAt(gfxPage, scoreString, 0xe8 + (0x57 - n) / 2, 0x46);
        mystrcpy(scoreString, "MISSION RATING");
        n = stringWidth(gfxPage, scoreString);
        drawStringAt(gfxPage, scoreString, 0xe8 + (0x57 - n) / 2, 0x4e);
        mystrcpy(scoreString, "\x80");
        my_ltoa(missionScore, numBuf);
        mystrcat(scoreString, numBuf);
        n = stringWidth(gfxPage, scoreString);
        drawStringAt(gfxPage, scoreString, 0xe8 + (0x57 - n) / 2, 0x56);
        sub_15D1B();
        mystrcpy(scoreString, b);
        mystrcat(scoreString, "Press Selector for next mission event");
        drawWrappedText(gfxPage, scoreString, 80, 240, 130, 8);
    }
}
