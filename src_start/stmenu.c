/* START.EXE — theater/mission menu screens (seg000:0xa68c+). f15 stmissn.c
 * lineage: draws a vertical item list via sub_13B50 rows, highlights the
 * current selection with sub_10924, then sub_10AE8 runs the select widget
 * and the result lands in gameData->theater; byte_2C160 picks the next page. */
#include "inttype.h"

struct GD { int16 f0;                       /* 0x00 next-page */
            int8 pad0[0x1E];
            int16 f20,f22,f24,f26,f28,f2a,f2c,f2e,f30,f32,f34,f36;
            int16 theater;                  /* 0x38 */
            int16 isCampaignMission;        /* 0x3a */
            int16 flags3c;                  /* 0x3c */
            int16 flags3e;                  /* 0x3e */
            int16 flags40;                  /* 0x40 */
            int16 f42,f44,f46,f48,f4a,f4c,f4e; };
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
struct PgParms { int8 pad[0x22]; int16 f22; int8 pad1[0x0C]; int16 f30;
                 int8 pad2[0x40]; int16 f72; };
extern struct PgParms far *word_2D066;      /* far page parm record */
/* sub_1A68C (main select screen) externs */
extern int16 word_2D2C6;                    /* sub_161CC arg */
extern int16 word_22324;                    /* sub_161CC side flag */
extern int16 word_2C972, word_2C974;        /* sub_14746 args */
extern uint8 byte_298E9, byte_2B384;        /* res-loaded flags */
extern int16 word_2D064, word_2D270;        /* sub_14BEE args */
extern int16 *word_25B2C, *word_25B44, *word_25CF6; /* page records */
extern int16 word_25CE6;                    /* widget param */
extern uint8 far *word_209AA;               /* far ptr→item count byte */
extern int16 word_25B46[];                  /* dseg:0x5b46, stride 0x20 */
extern int16 word_25B64, word_25B84;        /* theater 0 b4f args */
extern int16 word_25BA4, word_25BC4;
extern int16 word_25BE4, word_25C04;
extern int16 word_25C24, word_25C44;
extern int16 word_25C64, word_25C84;
extern void sub_14BEE(int16 a, int16 b);
extern void sub_161CC(int16 a, int16 b, int16 c);
extern void sub_15120(char *d, char *s);
extern int16 sub_13E38(int16 *p, char *s);
extern void far ovlCall_c4e(int16 v);       /* overlay 1000:0c4e */
extern void far ovlCall_bea(int16 v);       /* overlay 1000:0bea */
/* sub_18F12 (briefing screen) externs */
extern int16 word_2542C[];                  /* per-mission value table */
extern char *word_25454[];                  /* row string-id table */
extern int16 word_2547C[], word_25480[];    /* packed nibble pairs */
extern int16 *word_25014, *word_24FFC, *word_2542A;
extern int16 word_25016, word_25034, word_25064;
extern uint8 byte_29B50, byte_2D06A;        /* res-loaded flags */
extern void sub_14ACB(char *s, int16 v, long x);
extern void sub_1513B(char far *d, char *s);
extern void far ovlCall_bc7(int16 *pg, int16 a, int16 b, int16 c,
                            int16 d, int16 e, int16 f);
extern uint8 blinkTimer;                    /* dseg:0x0a1c timer-irq counter */
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

/* seg000:0x8f12 — briefing/mission-summary screen: builds the objective
 * list (two word_25454 row runs), picks a random mission index, runs the
 * select widget, then branches to the accept or next-page arm. The shipped
 * EN binary is patched: the second sub_10AE8 call site was nop'd (3 bytes)
 * and `if (res == choice)` became `res = choice` + unconditional jumps,
 * forcing the accept arm — the map marks both patch sites U so mzdiff
 * skips them; the C below carries the unpatched semantics. */
void sub_18F12(void) {
    int16 low;                  /* [bp-2]  */
    int16 fl;                   /* [bp-4]  */
    int16 hn;                   /* [bp-6]  */
    int16 v;                    /* [bp-8]  */
    int16 t2;                   /* [bp-0a] */
    int16 pad1;                 /* [bp-0c] */
    int16 i;                    /* [bp-0e] */
    int16 y;                    /* [bp-10] */
    int16 w2;                   /* [bp-12] */
    int16 res;                  /* [bp-14] */
    int16 w3;                   /* [bp-16] */
    int16 choice;               /* [bp-18] */
    int16 len;                  /* [bp-1a] */

    if (word_2D066->f22 == 1) {
        byte_2C160 = 8;
        return;
    }
    choice = randMul(0x14);
    word_2B386 = (struct MenuRow *)0x992;
    word_2CA46 = (uint8 far **)0x98E;
    word_2CA48 = 0x996;
    byte_29B50 = 1;
    byte_298F0 = 1;
    gfx_unknown2b(0);
    switch (word_2C7D4) {
    case 0:
        sub_14746((char *)0x4F0A, word_2C972, word_2C974);
        sub_14BEE(word_2D064, word_2D06C);
        sub_14ACB((char *)0x4F15, word_2D26E, (long)word_2542C[choice]);
        v = word_2D26E;
        break;
    case 1:
        sub_14ACB((char *)0x4F1F, word_2D2C8, (long)word_2542C[choice]);
        v = word_2D2C8;
        if (byte_2D06A == 0) {
            gfx_unknown2b(0);
            sub_14746((char *)0x4F29, word_2C972, word_2C974);
            sub_14BEE(word_2D064, word_2D06C);
            byte_2D06A = 1;
        }
        break;
    case 2:
        sub_14746((char *)0x4F34, word_2C972, word_2C974);
        sub_14BEE(word_2D064, word_2D06C);
        sub_14ACB((char *)0x4F3F, word_2D2C8, (long)word_2542C[choice]);
        v = word_2D2C8;
        break;
    }
    ovlCall_c53();
    ovlCall_bea(word_2D06C);
    word_25016 = v;
    ovlCall_b4f(word_25034);
    y = 0x6F;
    for (i = 0; i < 0xA; i++) {
        sub_13B76(word_24FFC, word_25454[i], 0x2E, y);
        y += 8;
    }
    y = 0x6F;
    for (i = 0xA; i < 0x14; i++) {
        sub_13B76(word_24FFC, word_25454[i], 0x9F, y);
        y += 8;
    }
    sub_15120((char *)0xB96A, (char *)0x4F49);
    len = sub_13E38(word_24FFC, (char *)0xB96A);
    sub_13B76(word_24FFC, (char *)0xB96A, (uint16)(0x140 - len) >> 1, 0x15);
    sub_13B76(word_25014, (char *)0x4F66, 0x46, 0xC1);
    ovlCall_c8a();
    ovlCall_c4e(1);
    sub_108B7();
    sub_14E9C();
    word_25064 = 2;
    res = 0;
    sub_10924((char *)0x5036, word_2CA48, **word_2CA46, 0x64, 0x70,
              word_24FFC);
    res = sub_10AE8((char *)0x5036, word_2CA48, **word_2CA46,
                    word_2542A, word_24FFC);
    sub_14622(word_25014, 0x1E, 0xC1, 0x12C, 0xC6);
    fl = 0;
    if (res == choice)
        goto ACCEPT;
    goto NEXT;

ACCEPT:
    word_25014[2] = 2;
    sub_13B76(word_25014, (char *)0x4F94, 0x6E, 0xC1);
    ovlCall_c8a();
    byte_2C160 = 8;
    word_2D066->f22 = 1;
    t2 = 0;
    while (t2 < 8) {
        if (blinkTimer <= 8)
            continue;
        blinkTimer = 0;
        {
            register int16 sv = word_2547C[fl];
            hn = sv >> 4;
            low = sv & 0xF;
        }
        ovlCall_bc7(word_25014, 0x64, 0xC1, 0x12C, 0xC6, hn, low);
        fl = (fl + 1) & 1;
        t2++;
    }
    sub_14EDA();
    return;

NEXT:
    word_25014[2] = 4;
    sub_13B76(word_25014, (char *)0x4FAB, 0x46, 0xC1);
    ovlCall_c8a();
    byte_2C160 = 0xF;
    gameData->f0 = 0xA;
    sub_1513B((char far *)gameData + 2, (char *)0x4FDC);
    gameData->f20 = 0;
    gameData->f22 = 0;
    gameData->f24 = 0;
    gameData->f26 = 0;
    gameData->f28 = 0;
    gameData->f2a = 0;
    gameData->f2c = 0;
    gameData->f2e = 0;
    gameData->f30 = 0;
    gameData->f32 = gameData->f34 = 0;
    gameData->f36 = 0;
    gameData->theater = 0;
    gameData->isCampaignMission = 0;
    gameData->flags3e = 0;
    gameData->flags40 = 0;
    gameData->flags3c = 3;
    gameData->f42 = 4;
    gameData->f44 = 0;
    gameData->f46 = 0;
    gameData->f48 = 0;
    gameData->f4a = 0;
    gameData->f4c = 0;
    gameData->f4e = 0;
    word_2D066->f30 = 1;
    word_2D066->f22 = 0;
    t2 = 0;
    do {
        if (blinkTimer > 0xC) {
            register int16 sv = word_25480[fl];
            hn = sv >> 4;
            low = sv & 0xF;
            ovlCall_bc7(word_25014, 0x46, 0xC1, 0x12C, 0xC6, hn, low);
            fl = (fl + 1) & 1;
            t2++;
        }
    } while (t2 < 8);
    sub_14EDA();
}

/* seg000:0xa68c — main select screen: word_2C7D4 resource block, five
 * per-theater overlay calls, item rows (or a single row) by word_2D066->f22,
 * centered strings for gameData->f3a..f40, then the select widget. */
void sub_1A68C(void) {
    int16 v;                    /* [bp-2]  */
    int16 i;                    /* [bp-4]  */
    int16 dy;                   /* [bp-6]  */
    int16 len;                  /* [bp-0a] */
    int16 result;               /* [bp-8]  */

    sub_108B7();
    switch (word_2C7D4) {
    case 0:
        if (rtcEnabled == 1) {
            sub_14746((char *)0x59B0, word_2D2CA, word_2D2CC);
            sub_161CC(0x6A, 1, word_2D2C6);
            word_22324 = 0;
        }
        gfx_unknown2b(8);
        sub_14746((char *)0x59BA, word_2C972, word_2C974);
        sub_14BEE(word_2D064, word_2D06C);
        gfx_unknown2b(0xB);
        loadSpriteRes((char *)0x59C3, word_2D26E);
        v = word_2D26E;
        ovlCall_c53();
        ovlCall_bea(word_2D06C);
        break;
    case 1:
        if (rtcEnabled == 1) {
            sub_161CC(0x6A, 1, word_2D2C6);
            word_22324 = 0;
        }
        v = word_2D26E;
        if (byte_298E9 == 0) {
            gfx_unknown2b(8);
            sub_14746((char *)0x59CC, word_2C972, word_2C974);
            sub_14BEE(word_2D064, word_2D270);
            byte_298E9 = 1;
        }
        if (byte_2B384 == 0) {
            gfx_unknown2b(0xB);
            loadSpriteRes((char *)0x59D5, word_2D26E);
            byte_2B384 = 1;
        }
        ovlCall_c53();
        ovlCall_bea(word_2D270);
        break;
    case 2:
        gfx_unknown2b(6);
        sub_14746((char *)0x59DE, word_2C972, word_2C974);
        sub_14BEE(word_2D064, word_2D06C);
        if (rtcEnabled == 1) {
            sub_161CC(0x6A, 1, word_2D2C6);
            word_22324 = 0;
        }
        v = word_2D26E;
        ovlCall_c53();
        ovlCall_bea(word_2D06C);
        break;
    }
    word_25B46[0] = word_25B46[16] = word_25B46[32] = word_25B46[48] =
        word_25B46[64] = word_25B46[80] = word_25B46[96] = word_25B46[112] =
        word_25B46[128] = word_25B46[144] = v;
    switch (gameData->theater) {
    case 0:
        ovlCall_b4f(word_25B64);
        ovlCall_b4f(word_25B84);
        break;
    case 1:
        ovlCall_b4f(word_25BA4);
        ovlCall_b4f(word_25BC4);
        break;
    case 2:
        ovlCall_b4f(word_25BE4);
        ovlCall_b4f(word_25C04);
        break;
    case 3:
        ovlCall_b4f(word_25C24);
        ovlCall_b4f(word_25C44);
        break;
    case 4:
        ovlCall_b4f(word_25C64);
        ovlCall_b4f(word_25C84);
        break;
    }
    ovlCall_c4e(1);
    word_2B386 = (struct MenuRow *)0x9AE;
    word_2CA46 = (uint8 far **)0x9AA;
    word_25CF6[5] = ((int16)*word_209AA - 1) * word_25CF6[1] + word_25CF6[4];
    flag_29948 = 1;
    word_25B2C[2] = 0xF;
    sub_14584(word_25B2C, 0x96, 0x82, 0x104, 0xBE);
    word_25B2C[2] = 6;
    if (word_2D066->f22 == 1) {
        dy = 0x82;
        for (i = 0; i < **word_2CA46; i++) {
            sub_13B50(word_25B2C, word_2B386[i], 0xA0, dy);
            dy += 0xB;
        }
        word_25B44[2] = 9;
        sub_13B76(word_25B44, (char *)0x59E7, 0x98, 0x48);
        word_25CF6[4] = 0x84;
    } else {
        sub_13B50(word_25B2C, word_2B386[1], 0xA0, 0x8D);
        word_25B44[2] = 9;
        sub_13B76(word_25B44, (char *)0x5A08, 0x98, 0x48);
        word_25CF6[4] = 0x8F;
    }
    sub_15120((char *)0xB96A, (char *)0x5A23);
    len = sub_13E38(word_25B44, (char *)0xB96A);
    sub_13B76(word_25B44, (char *)0xB96A, 0x98 + (0x82 - len) / 2, 8);
    word_25B44[2] = 0;
    switch (gameData->isCampaignMission) {
    case 0:  sub_15120((char *)0xB96A, (char *)0x5A37); break;
    case 1:  sub_15120((char *)0xB96A, (char *)0x5A40); break;
    case 2:  sub_15120((char *)0xB96A, (char *)0x5A4C); break;
    }
    len = sub_13E38(word_25B44, (char *)0xB96A);
    sub_13B76(word_25B44, (char *)0xB96A, 0x98 + (0x82 - len) / 2, 0x12);
    switch (gameData->flags3c) {
    case 0:  sub_15120((char *)0xB96A, (char *)0x5A5D); break;
    case 1:  sub_15120((char *)0xB96A, (char *)0x5A71); break;
    case 2:  sub_15120((char *)0xB96A, (char *)0x5A81); break;
    case 3:  sub_15120((char *)0xB96A, (char *)0x5A95); break;
    }
    len = sub_13E38(word_25B44, (char *)0xB96A);
    sub_13B76(word_25B44, (char *)0xB96A, 0x98 + (0x82 - len) / 2, 0x1C);
    switch (gameData->flags3e) {
    case 0:  sub_15120((char *)0xB96A, (char *)0x5AA5); break;
    case 1:  sub_15120((char *)0xB96A, (char *)0x5AB5); break;
    case 2:  sub_15120((char *)0xB96A, (char *)0x5AC7); break;
    case 3:  sub_15120((char *)0xB96A, (char *)0x5AD9); break;
    }
    len = sub_13E38(word_25B44, (char *)0xB96A);
    sub_13B76(word_25B44, (char *)0xB96A, 0x98 + (0x82 - len) / 2, 0x26);
    switch (gameData->flags40) {
    case 0:  sub_15120((char *)0xB96A, (char *)0x5AE9); break;
    case 1:  sub_15120((char *)0xB96A, (char *)0x5AF4); break;
    case 2:  sub_15120((char *)0xB96A, (char *)0x5B02); break;
    }
    len = sub_13E38(word_25B44, (char *)0xB96A);
    sub_13B76(word_25B44, (char *)0xB96A, 0x98 + (0x82 - len) / 2, 0x30);
    word_25B2C[2] = 0xF;
    sub_14E9C();
    word_25CE6 = 2;
    sub_10924((char *)0x5C86, word_2CA48, **word_2CA46, 0xC8,
              word_25CF6[1] + 0x84, word_25B2C, 0);
    result = sub_10AE8((char *)0x5C86, word_2CA48, **word_2CA46,
                       word_25CF6, word_25B2C, 0);
    if (result == 1)
        byte_2C160 = 4;
    else
        byte_2C160 = 1;
    sub_125EA();
    sub_14EDA();
    byte_2CA62 = 0;
    sub_14622(word_25B2C, 0x98, 0x48, 0x117, 0x4E);
    ovlCall_c8a();
    if (rtcEnabled == 1)
        sub_1685C(word_2A0C4);
}

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
