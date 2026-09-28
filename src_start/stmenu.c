/* START.EXE — theater/mission menu screens (seg000:0xa68c+). f15 stmissn.c
 * lineage: draws a vertical item list via sub_13B50 rows, highlights the
 * current selection with sub_10924, then sub_10AE8 runs the select widget
 * and the result lands in gameData->theater; byte_2C160 picks the next page. */
#include "inttype.h"

struct GD { int8 pad[0x38]; int16 theater; int16 isCampaignMission;
            int16 flags3c; int16 flags3e; int16 flags40; };
extern struct GD far *gameData;             /* far ptr dseg:0x991c */

extern void sub_108B7(void);                /* seg000:0x08b7 screen reset */
extern void sub_14622(void *o, int16 a, int16 b, int16 c, int16 d); /* clearRect */
extern void sub_13B76(void *o, char *s, int16 x, int16 y); /* drawStringAt */
struct MenuRow { int16 name, yoff; };
extern void sub_13B50(int16 *page, struct MenuRow r, int16 c, int16 d);
extern void sub_10924();                    /* draw menu box + highlight */
extern int16 sub_10AE8();                   /* menu select widget */
extern void sub_14E9C(void);
extern void sub_125EA(void);
extern void sub_14EDA(void);
extern void sub_14584(void *o, int16 a, int16 b, int16 c, int16 d);
extern void sub_14A5F(char *s, int16 v);
extern void sub_14746(char *s, int16 a, int16 b);
extern int16 far gfx_unknown2b(int16 v);        /* overlay slot 0x2b */

extern struct MenuRow *word_2B386;          /* menu row-table near ptr */
extern uint8 far **word_2CA46;              /* ptr→far menu string table */
extern int16 word_2CA48;                    /* second table selector */
extern uint8 far *word_207EA;               /* far ptr→item count byte */
extern int16 *word_25F7C;                   /* selection page record */
extern int16 *word_25D30;                   /* page1 record handle */
extern int16 *word_25D48;                   /* page2 record handle */
extern int16 word_2D26E;                    /* sel-init broadcast value */
extern int16 selInitTab[10 * 15];           /* dseg:0x25d4a, stride 30 */
struct MenuSelBlk { int16 sel; int16 pad[24]; };
extern struct MenuSelBlk menuSelTab[6];     /* dseg:0x25ea4, stride 0x32 */
extern struct MenuSelBlk menuSelTab2[6];    /* dseg:0x25fdc, stride 0x32 */
extern int16 flag_29948;                    /* dseg:0x9948 */
extern uint8 byte_2C160;                    /* next-screen selector */
extern struct MenuRow *word_2D276;          /* second row-table near ptr */
extern int16 word_2C7D2;                    /* row-draw y cursor */
extern uint8 far *word_20822;               /* far ptr→item count byte */
extern int16 *word_26050;                   /* selection page record */
extern int16 *word_25F94, *word_25FAC;      /* page record handles */
extern struct MenuSelBlk menuSelTab3[6];    /* dseg:0x26202, stride 0x32 */
extern int16 word_2D272;                    /* initTab2 broadcast value */
extern int16 initTab2[10 * 16];             /* dseg:0x26094, stride 0x20 */
extern uint8 byte_2D060;                    /* briefing-block active flag */
extern int16 word_2D06C, word_2D2CA, word_2D2CC; /* briefing string/rect */
extern uint8 far *word_20862;               /* far ptr→item count byte */
extern int16 *word_262A8, *word_2607A, *word_26092; /* page record handles */
extern struct MenuSelBlk menuSelTab4[6];    /* dseg:0x26308, stride 0x32 */
extern uint8 far *word_20896;               /* far ptr→item count byte */
extern int16 *word_263AE, *word_262C0, *word_262D8; /* page record handles */
extern struct MenuSelBlk menuSelTab5[6];    /* dseg:0x2640c, stride 0x32 */
extern uint8 far *word_208BA;               /* far ptr→item count byte */
extern int16 *word_26480, *word_263C6;      /* page record handles */
extern int16 rtcEnabled;                    /* dseg:0xbb74 */
extern int16 word_2A0C4;                    /* rtc-bail arg */
extern void sub_1685C(int16 v);

/* sub_1B452 (mission-setup screen) externs */
extern int16 word_2BE4A;                    /* scratch index */
extern char far *word_209B6[];              /* theater name far-ptr table */
extern int8  byte_2B388;                    /* last theater */
extern uint8 byte_298F0, byte_2CA62, byte_2CA6A;
extern int16 word_2C7D4;                    /* mission kind selector */
extern int16 word_2D2C8;                    /* second sel-init value */
extern int16 *word_26572, *word_2655A;      /* page record handles */
extern int16 word_26574[];                  /* dseg:0x6574, stride 0x20 */
extern int16 word_26592, word_265B2;        /* 0b4f-handler args */
extern int16 word_265B6, word_265D6;
extern int16 word_26676, word_26696;
extern int16 *word_2681E;                   /* selp arg */
extern char *word_26820[];                  /* list-label string table */
extern struct MenuSelBlk menuSelTab6[];     /* dseg:0x66e2, stride 0x32 */
extern int16 word_2CA6C;                    /* mode-out code */
extern int16 word_2B948, word_2B95A;        /* excluded object indices */
extern uint8 byte_2C976, byte_2C977;
extern uint8 ringMode;                      /* dseg:0x9922 */
extern uint8 byte_20A1A;                    /* input-wait counter */
extern uint8 unitMarksOn, tileMarksOn;      /* dseg:0x98e8/0xbe48 */
extern int8  tileMarkMap[];                 /* dseg:0xb842 — 16x16, bit 0x10 */
extern int16 objectCount;                   /* dseg:0x2c978 */
extern int8  objectActive[];                /* dseg:0x2d278 */
typedef struct {                            /* worldObjects: stride 0x10 */
    int16 x_coord, y_coord;                 /* 0x00, 0x02 */
    int16 pad4;                             /* 0x04 */
    int16 targetFlags;                      /* 0x06 */
    int16 pad8[4];                          /* 0x08 */
} WorldObject;
extern WorldObject worldObjects[];          /* dseg:0xb390 */
struct PgParms { int8 pad[0x72]; int16 f72; };
extern struct PgParms far *word_2D066;      /* far page parm record */
extern void sub_15152(char *d, const char far *s);  /* far-src strcpy */
extern void loadSpriteRes(char *n, int16 sel);
extern int16 randMul(uint16 n);
extern void selectNextObject(void);
extern void selectNextUnit(void);
extern void missionGenerate(void);
extern void drawRoutePath(void);
extern void drawThreatRings(void);
extern void drawSiteMarkers(void);
extern void drawUnitMarkers(void);
extern void drawTileMarkers(void);
extern void far ovlCall_b4f(int16 h);       /* overlay 1000:0b4f */
extern int16 far ovlCall_c53(void);         /* overlay 1000:0c53 */
extern void far ovlCall_c58(void);          /* overlay 1000:0c58 */
extern void far ovlCall_c8a(void);          /* overlay 1000:0c8a */
extern int16 far ovlCall_ccb(int16 v);      /* overlay 1000:0ccb poll */

/* seg000:0xaca0 — single-column theater list: draws items, highlights the
 * current theater row, runs the select widget. */
void sub_1ACA0(void) {
    int16 unused;
    int16 row;
    int16 y;
    int16 result;

    sub_108B7();
    word_2B386 = (struct MenuRow *)0x7EE;
    word_2CA46 = (uint8 far **)0x7EA;
    word_2CA48 = 0x802;
    word_25F7C[5] = ((int16)*word_207EA - 1) * word_25F7C[1] + word_25F7C[4];
    sub_14622(word_25D30, 0x96, 0x82, 0x104, 0xBE);
    selInitTab[0] = selInitTab[15] = selInitTab[30] = selInitTab[45] =
        selInitTab[60] = selInitTab[75] = selInitTab[90] = selInitTab[105] =
        selInitTab[120] = selInitTab[135] = word_2D26E;
    flag_29948 = 1;
    word_25D30[2] = 6;
    y = 0x82;
    row = 0;
    while (row < **word_2CA46) {
        sub_13B50(word_25D30, word_2B386[row], 0xA6, y);
        y += 0xB;
        row++;
    }
    word_25D48[2] = 9;
    sub_13B76(word_25D48, (char *)0x5CF8, 0x98, 0x3C);
    word_25D48[2] = 0;
    word_25D30[2] = 0xF;
    menuSelTab[(uint16)gameData->theater].sel = 2;
    sub_10924((char *)0x5E76, word_2CA48, **word_2CA46, 0xC8,
              (uint16)gameData->theater * word_25F7C[1] + 0x84, word_25D30, 1);
    sub_14E9C();
    result = sub_10AE8((char *)0x5E76, word_2CA48, **word_2CA46,
                       word_25F7C, word_25D30, 1);
    gameData->theater = result;
    sub_125EA();
    sub_14EDA();
    byte_2C160 = 9;
}

/* seg000:0xae22 — campaign-mission list: same skeleton as sub_1ACA0 plus a
 * second row-table stamp for the chosen entry; result → isCampaignMission. */
void sub_1AE22(void) {
    int16 unused;
    int16 row;
    int16 y;
    int16 result;

    sub_108B7();
    word_2B386 = (struct MenuRow *)0x826;
    word_2CA46 = (uint8 far **)0x822;
    word_2CA48 = 0x832;
    word_2D276 = (struct MenuRow *)0x816;
    word_2C7D2 = 0x1A;
    word_26050[5] = ((int16)*word_20822 - 1) * word_26050[1] + word_26050[4];
    word_25F94[2] = 0xF;
    sub_14584(word_25F94, 0xA6, 0x82, 0xF0, 0xC7);
    word_25F94[2] = 6;
    y = 0x82;
    row = 0;
    while (row < **word_2CA46) {
        sub_13B50(word_25F94, word_2B386[row], 0xA6, y);
        y += 0xB;
        row++;
    }
    word_25F94[2] = 0xF;
    menuSelTab2[gameData->isCampaignMission].sel = 2;
    sub_10924((char *)0x5FAE, word_2CA48, **word_2CA46, 0xC8,
              (uint16)gameData->isCampaignMission * word_26050[1] + 0x84,
              word_25F94, 2);
    sub_14E9C();
    result = sub_10AE8((char *)0x5FAE, word_2CA48, **word_2CA46,
                       word_26050, word_25F94, 2);
    gameData->isCampaignMission = result;
    word_25FAC[2] = 9;
    sub_13B50(word_25FAC, word_2D276[result], word_2C7D2, 0x49);
    word_2C7D2 = word_25FAC[4] + 4;
    word_25FAC[2] = 0;
    sub_125EA();
    sub_14EDA();
    byte_2C160 = 3;
}

/* seg000:0xafa8 — difficulty/tension list: same skeleton plus a briefing-
 * string block (guarded by byte_2D060) and a second stamp row; result →
 * gameData->flags3c (dseg:0x3c). */
void sub_1AFA8(void) {
    int16 row;
    int16 y;
    int16 result;

    sub_108B7();
    initTab2[0] = initTab2[16] = initTab2[32] = initTab2[48] =
        initTab2[64] = initTab2[80] = initTab2[96] = initTab2[112] =
        initTab2[128] = initTab2[144] = word_2D272;
    if (byte_2D060 != 0) {
        gfx_unknown2b(6);
        sub_14A5F((char *)0x6052, word_2D06C);
        sub_14746((char *)0x605B, word_2D2CA, word_2D2CC);
        byte_2D060 = 0;
    }
    word_2B386 = (struct MenuRow *)0x866;
    word_2CA46 = (uint8 far **)0x862;
    word_2CA48 = 0x876;
    word_2D276 = (struct MenuRow *)0x852;
    word_262A8[5] = ((int16)*word_20862 - 1) * word_262A8[1] + word_262A8[4];
    word_2607A[2] = 0xF;
    sub_14584(word_2607A, 0x96, 0x82, 0x104, 0xBE);
    word_2607A[2] = 6;
    y = 0x82;
    row = 0;
    while (row < **word_2CA46) {
        sub_13B50(word_2607A, word_2B386[row], 0xA0, y);
        y += 0xB;
        row++;
    }
    word_2607A[2] = 0xF;
    menuSelTab3[(uint16)gameData->flags3c].sel = 2;
    sub_10924((char *)0x61D4, word_2CA48, **word_2CA46, 0xC8,
              (uint16)gameData->flags3c * word_262A8[1] + 0x84,
              word_2607A, 3);
    sub_14E9C();
    result = sub_10AE8((char *)0x61D4, word_2CA48, **word_2CA46,
                       word_262A8, word_2607A, 3);
    word_26092[2] = 9;
    sub_13B50(word_26092, word_2D276[result], word_2C7D2, 0x49);
    word_2C7D2 = word_26092[4] + 4;
    word_26092[2] = 0;
    gameData->flags3c = result;
    sub_125EA();
    sub_14EDA();
    byte_2C160 = 0xA;
}

/* seg000:0xb184 — next menu level: same skeleton as sub_1AE22; result →
 * gameData->flags3e (dseg:0x3e). */
void sub_1B184(void) {
    int16 unused;
    int16 row;
    int16 y;
    int16 result;

    sub_108B7();
    word_2B386 = (struct MenuRow *)0x89A;
    word_2CA46 = (uint8 far **)0x896;
    word_2CA48 = 0x8AA;
    word_2D276 = (struct MenuRow *)0x886;
    word_263AE[5] = ((int16)*word_20896 - 1) * word_263AE[1] + word_263AE[4];
    word_262C0[2] = 0xF;
    sub_14584(word_262C0, 0x96, 0x82, 0x104, 0xBE);
    word_262C0[2] = 6;
    y = 0x82;
    row = 0;
    while (row < **word_2CA46) {
        sub_13B50(word_262C0, word_2B386[row], 0xA0, y);
        y += 0xB;
        row++;
    }
    word_262C0[2] = 0xF;
    menuSelTab4[gameData->flags3e].sel = 2;
    sub_10924((char *)0x62DA, word_2CA48, **word_2CA46, 0xC8,
              (uint16)gameData->flags3e * word_263AE[1] + 0x84,
              word_262C0, 4);
    sub_14E9C();
    result = sub_10AE8((char *)0x62DA, word_2CA48, **word_2CA46,
                       word_263AE, word_262C0, 4);
    gameData->flags3e = result;
    word_262D8[2] = 9;
    sub_13B50(word_262D8, word_2D276[result], word_2C7D2, 0x49);
    word_2C7D2 = word_262D8[4] + 4;
    word_262D8[2] = 0;
    sub_125EA();
    sub_14EDA();
    byte_2C160 = 0xB;
}

/* seg000:0xb304 — final menu level: same skeleton, no stamp row; result →
 * gameData->flags40 (dseg:0x40); rtc bail-out when rtcEnabled==1. */
void sub_1B304(void) {
    int16 unused;
    int16 row;
    int16 y;
    int16 result;

    sub_108B7();
    word_2B386 = (struct MenuRow *)0x8BE;
    word_2CA46 = (uint8 far **)0x8BA;
    word_2CA48 = 0x8CA;
    word_26480[5] = ((int16)*word_208BA - 1) * word_26480[1] + word_26480[4];
    word_263C6[2] = 0xF;
    sub_14584(word_263C6, 0x96, 0x82, 0x104, 0xBE);
    word_263C6[2] = 6;
    y = 0x82;
    row = 0;
    while (row < **word_2CA46) {
        sub_13B50(word_263C6, word_2B386[row], 0xA0, y);
        y += 0xB;
        row++;
    }
    word_263C6[2] = 0xF;
    menuSelTab5[(uint16)gameData->flags40].sel = 2;
    sub_10924((char *)0x63DE, word_2CA48, **word_2CA46, 0xC8,
              (uint16)gameData->flags40 * word_26480[1] + 0x84,
              word_263C6, 5);
    sub_14E9C();
    result = sub_10AE8((char *)0x63DE, word_2CA48, **word_2CA46,
                       word_26480, word_263C6, 5);
    gameData->flags40 = result;
    if (rtcEnabled == 1)
        sub_1685C(word_2A0C4);
    sub_125EA();
    sub_14EDA();
    byte_2C160 = 4;
}

/* seg000:0xb452 — mission-setup screen: builds the theater subtitle, marks
 * objects for the pick lists, draws the 7-entry option list, then runs the
 * select widget loop toggling flags; byte_2C160 picks the next page. */
void sub_1B452(void) {
    int16 redraw;               /* [bp-4]  */
    int16 v;                    /* [bp-6]  */
    int16 i;                    /* [bp-8]  */
    int16 j;                    /* [bp-0c] */
    int16 loop;                 /* [bp-0a] */
    char a[2];                  /* [bp-2]  dead init */
    int16 selrow;               /* [bp-18] */
    char c9[2];                 /* [bp-16] dead init */
    int16 nn;                   /* [bp-14] */
    char esc[3];                /* [bp-12] dead init */
    int16 k;                    /* [bp-0e] */
    char vv2[2];                /* [bp-1a] dead init */

    sub_108B7();
    a[0] = 0xD; a[1] = 0;
    esc[0] = 9; esc[1] = 0xA; esc[2] = 0;
    vv2[0] = 0x8E; vv2[1] = 0;
    c9[0] = 0x8F; c9[1] = 0;
    word_2BE4A = 0x9B6;
    sub_15152((char *)0xB96A, word_209B6[gameData->theater]);
    if (gameData->theater != byte_2B388)
        byte_298F0 = 1;
    gfx_unknown2b(9);
    if (byte_2CA62 == 1) {
        selrow = 6;
        ovlCall_c53();
        sub_14622(word_26572, 8, 0, 0x13F, 0xB1);
        switch (word_2C7D4) {
        case 0:  v = word_2D26E; break;
        case 1:  v = word_2D2C8; break;
        case 2:  v = word_2D2C8; break;
        }
    } else {
        selrow = 0;
        byte_2CA62 = 0;
        switch (word_2C7D4) {
        case 0:
            loadSpriteRes((char *)0xB96A, word_2D26E);
            v = word_2D26E;
            ovlCall_c53();
            sub_14622(word_2655A, 0, 0, 0x13F, 0xC7);
            break;
        case 1:
            if (byte_298F0 == 1) {
                loadSpriteRes((char *)0xB96A, word_2D2C8);
                byte_298F0 = 0;
            }
            v = word_2D2C8;
            ovlCall_c53();
            sub_14622(word_2655A, 0, 0, 0x13F, 0xC7);
            break;
        case 2:
            if (byte_298F0 == 1) {
                loadSpriteRes((char *)0xB96A, word_2D2C8);
                byte_298F0 = 0;
            }
            v = word_2D2C8;
            ovlCall_c53();
            sub_14622(word_2655A, 0, 0, 0x13F, 0xC7);
            break;
        }
    }
    word_26574[0] = word_26574[16] = word_26574[32] = word_26574[48] =
        word_26574[64] = word_26574[80] = word_26574[96] = word_26574[112] =
        word_26574[128] = word_26574[144] = v;
    flag_29948 = 0;
    ovlCall_b4f(word_26592);
    ovlCall_b4f(word_265B2);
    word_2655A[2] = 0xC;
    sub_13B76(word_2655A, (char *)0x64E3, 0x1E, 1);
    word_2655A[2] = 0;
    sub_13B76(word_2655A, (char *)0x6515, 0x6F, 1);
    ovlCall_c8a();
    ovlCall_c58();
    if (byte_2CA6A == 1) {
        byte_2C977 = tileMarksOn = ringMode = unitMarksOn = 0;
        sub_13B76(word_26572, (char *)0x652B, 0xF5, 0x1E);
        ovlCall_c8a();
        missionGenerate();
        byte_2CA6A = 0;
        byte_2C976 = 0;
        for (i = 0; i < 0x10; i++) {
            for (j = 0; j < 0x10; j++)
                if ((tileMarkMap[j * 16 + i] & 0x10) != 0)
                    byte_2C976 = 1;
        }
        for (k = 0; k <= objectCount; k++)
            objectActive[k] = 0;
        k = randMul(3);
        while (k != 0) {
            nn = randMul(objectCount);
            if (nn == word_2B948) continue;
            if (nn == word_2B95A) continue;
            if (worldObjects[nn].pad4 != 0 &&
                (worldObjects[nn].targetFlags & 0x500) == 0) {
                objectActive[nn] = 1;
                k--;
            }
        }
        for (k = 0; k <= objectCount; k++) {
            if (objectActive[k] == 1) {
                objectActive[k] = randMul(3) + 1;
                if (objectActive[k] == 1)
                    worldObjects[k].pad4 = 0;
            }
        }
    }
    word_2655A[2] = 6;
    nn = 0x6E;
    for (k = 0; k < 7; k++) {
        sub_13B76(word_2655A, word_26820[k], 0xEC, nn);
        nn += 0xA;
    }
    selectNextObject();
    selectNextUnit();
    redraw = 1;
    ovlCall_c8a();
    sub_14E9C();
    loop = 1;
    do {
        if (redraw == 1) {
            word_265B6 = 0x12D;
            word_265D6 = 0x12D;
            word_26676 = 0x11E;
            word_26696 = 0x11E;
            ovlCall_b4f(word_26592);
            drawRoutePath();
            drawThreatRings();
            drawSiteMarkers();
            drawUnitMarkers();
            drawTileMarkers();
        }
        menuSelTab6[selrow].sel = 2;
        sub_10924((char *)0x66B4, word_2CA48, 7, 0xFA, selrow * 0xA + 0x6F,
                  word_26572);
        selrow = sub_10AE8((char *)0x66B4, word_2CA48, 7, word_2681E,
                           word_26572);
        redraw = 1;
        switch (selrow + 1) {
        case 1:
            loop = 0;
            byte_2C160 = 5;
            word_2CA6C = 1;
            break;
        case 2:
            ringMode = ++ringMode % 3;
            break;
        case 3:
            byte_2C977 = (byte_2C977 == 0);
            break;
        case 4:
            unitMarksOn = (unitMarksOn == 0);
            break;
        case 5:
            loop = 0;
            byte_2C160 = 5;
            word_2CA6C = 2;
            break;
        case 6:
            tileMarksOn = (tileMarksOn == 0);
            break;
        case 7:
            byte_2C160 = 6;
            loop = 0;
            break;
        }
        if (word_2D066->f72 == 1) {
            while (ovlCall_ccb(0) != 0)
                ;
            byte_20A1A = 0;
            while (byte_20A1A <= 5)
                ;
            while (ovlCall_ccb(0) != 0)
                ;
        }
    } while (loop != 0);
    sub_14EDA();
    byte_2CA62 = 0;
}
