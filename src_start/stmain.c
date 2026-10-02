/* START.EXE — the menu program's main driver plus entry-adjacent helpers.
 * seg000:0x0010-0x080f  sub_10010: program entry — comm-block hookup,
 * overlay patch, gfx/audio init, title/credit/ad splash sequence,
 * memory-tier resource fit, then the menu state machine and exit to EGAME.
 * seg000:0x0810-0x08c7: the quit-path tail (sub_10810), the overlay
 * settle-poll (sub_108B7) and two empty stubs (sub_108B5/sub_108B6). */
#include "inttype.h"
#include "pointers.h"

#define FP_OFF(p) (*(uint16 *)&(p))
#define FP_SEG(p) (*((uint16 *)&(p) + 1))

/* ---- comm block (far ptr at dseg:0xd066) — fields touched by sub_10010 */
struct CommData {
    int8  pad0[0x1a];
    int16 f1a, f1c, f1e;                    /* overlay slot segments */
    int16 f20;                              /* arg to ovlCall_c71 */
    int16 f22;
    uint8 f24; int8 pad25;
    int16 f26, f28, f2a, f2c;
    int16 missionRange;                     /* +0x2e */
    int16 f30;
    int16 pad32[6];
    int16 missionKind[4];                   /* +0x38 */
    int16 missionStat[4];                   /* +0x40 */
    int8  f48[0x28];                        /* +0x48 — far arg to ovlFee_fd */
    int16 f70;                              /* +0x70 quick-start flag */
    int16 f72;                              /* +0x72 input-flush flag */
};

/* ---- game data (far ptr at dseg:0x991c) */
struct GameData {
    int16 f0;
    int8  name[0x36];                       /* +0x02 pilot name */
    int16 f38, f3a, f3c, f3e;               /* +0x38..0x3e */
};

extern struct CommData far *commData;
extern struct GameData far *gameData;

extern uint8  byte_212BA;                      /* abort/quit flag */
extern uint8  byte_2C160;                      /* menu mode / next-screen */
extern uint8  byte_20A1A;                      /* timer tick counter */
extern uint8  byte_216AA, byte_216AB;          /* joystick axis centers */
extern uint8  byte_2BE4E;                      /* keyboard-status copy */
extern uint8  byte_29B50, byte_2CA6A;          /* mode flags */
extern uint8  byte_298F6, byte_298F7;          /* res-loaded flags */
extern uint8  byte_2B384, byte_2B388;          /* res-loaded flags */
extern uint8  byte_2C7D6, byte_2C7D7;          /* buffer-clear flags */
extern uint8  byte_2CE26, byte_2D06A;
extern int16  word_200E6;                      /* screen buffer offset */
extern int16  word_22324;
extern int16  word_298E0;
extern int16  far *word_298EC;                 /* far ptr → gfx vector slot */
extern int16  far *word_2B942;                 /* far ptr → reload-request */
extern int16  word_2B38A;
extern int16  word_2B394[];                    /* init record table */
extern int16  word_2BB74;                      /* aux-table tier flag */
extern int16  word_2C7D4;                      /* memory tier 0/1/2 */
extern int16  word_2C972, word_2C974;          /* pic far buf (off/seg) */
extern int16  word_2D064, word_2D06C;
extern int16  word_2D26E, word_2D270, word_2D272;
extern int16  word_2D2C4, word_2D2C6, word_2D2C8;
extern int16  word_2D2CA, word_2D2CC, word_2D2CE;
extern int16  word_2D2F6, word_2D2F8;
extern int16  objectCount;
extern uint8  objectActive[];
struct ObjD {                                  /* dseg:0xb38e, stride 0x10 */
    int16 f0;
    int16 pad2[2];
    int16 pad4;                                /* sub-record index (main clears) */
    int16 targetFlags;
    int16 padA;
    int16 padC;
    uint8 fE, padF;
};
extern struct ObjD word_2B38E[];

extern void   sub_10810(void);                 /* abort check (below) */
extern void   sub_108B7(void);                 /* overlay settle (below) */
extern int16  far ovlCall_cbc(void);           /* overlay key-ready */
extern int16  far ovlCall_cc1(void);           /* overlay getch */
extern void   sub_10882(void);                 /* cleanup */
extern void   sub_14107(int16);                /* overlay slot patcher */
extern void   sub_14622(void *o, int16 a, int16 b, int16 c, int16 d);
extern void   sub_146E3(void);                 /* restore int vector */
extern void   sub_14BEE(int16 a, int16 b);     /* decode pic into buf */
extern void   sub_14E9C(void);
extern void   sub_14EDA(void);
extern void   sub_1CBD8(void);                 /* mode-2 screen (map gap) */
extern void   sub_1CE56(void);                 /* mode-6 screen (map gap) */
extern void   sub_18F12(void);
extern void   sub_193EE(void);
extern void   sub_1A68C(void);
extern void   sub_1ACA0(void);
extern void   sub_1AE22(void);
extern void   sub_1AFA8(void);
extern void   sub_1B184(void);
extern void   sub_1B304(void);
extern void   sub_1B452(void);
extern void   printMission(void);
extern void   drawStoreIcons(void);
extern void   initGraphics(void);
extern void   installCBreakHandler(void);
extern void   cleanup(void);
extern void   missionGenerate(void);
extern void   exportWorldToComm(const char *filename);
extern int16  allocBuffer(uint16 paras);
extern void   freeBuffer(uint16 segment);
extern int16  resFileReadBlock(const char *path, int16 b, int16 c);
extern void   loadSpriteRes(const char *name, int16 sel);
extern int16  randMul(uint16 n);
extern void   delayTicks(int16 n);
extern void   sub_1DCAC(int16);                /* exit(code) */
extern int16  getch(void);
extern int16  putch(int16 c);
extern int16  time(int16 *t);                  /* seg000:0xe236 CRT time */
extern int16  sub_1E1EC(void);                 /* key-waiting check (al) */

extern int16  far ovlF43_a(int16 v);           /* overlay 0f43:0x0a */
extern void   far ovlF43_10d(int16 v);         /* overlay 0f43:0x10d */
extern void   far ovlFee_fd(int8 far *p);      /* overlay 0fee:0xfd */
extern void   far ovlCall_c71(int16 a, int16 b);/* overlay 1000:0c71 */
extern void   far ovlCall_cf3(void);           /* overlay 1000:0cf3 */
extern void   far ovlCall_cee(void);           /* overlay 1000:0cee */
extern int16  far ovlCall_bef(void);           /* overlay 1000:0bef */
extern void   far ovlCall_c4e(int16 v);        /* overlay 1000:0c4e */
extern void   far ovlCall_c2b(int16 v);        /* overlay 1000:0c2b */
extern int16  far ovlCall_c53(void);           /* overlay 1000:0c53 */
extern void   far ovlCall_bea(int16 v);        /* overlay 1000:0bea */
extern void   far ovlCall_c8a(void);           /* overlay 1000:0c8a */
extern void   far ovlCall_c58(void);           /* overlay 1000:0c58 */
extern void   far ovlCall_cfd(void);           /* overlay 1000:0cfd */
extern int16  far ovlCall_ccb(int16 v);        /* overlay 1000:0ccb poll */
extern int16  far ovlCall_b6d(void);           /* overlay 1000:0b6d */
extern int16  far ovlCall_c49(void);           /* overlay 1000:0c49 */
extern int16  far ovlCall_bf4(void);           /* overlay 1000:0bf4 */
extern void   far ovlCall_cd0(void);           /* overlay 1000:0cd0 */

/* seg000:0x0010 — START's program entry. */
void sub_10010(void) {
    int16 szc;          /* computed need (bp-2) */
    uint16 m4;          /* ovlCall_c49 ret (bp-4) */
    uint16 m6;          /* ovlCall_b6d ret (bp-6) */
    int16 gm;           /* ovlF43_a ret (bp-8) */
    uint16 bs;          /* ovlCall_bef ret (bp-a) */
    int16 szi;          /* frame pad (bp-c) */
    uint16 szj;         /* deadline (bp-e) */
    int16 szk;          /* frame pad (bp-10) */
    int16 far *lpm;     /* low-mem comm-seg ptr (bp-14/-12) */
    int16 szm;          /* time buf (bp-16) */
    uint8 szn;          /* object loop index (bp-18) */
    uint16 need;        /* free paras (bp-1a) */
    int16 cj;           /* frame pad (bp-1c) */

    word_2B38A = 0;
    FP_SEG(word_2B942) = 0;
    FP_OFF(word_2B942) = 0x4F2;
    FP_SEG(word_298EC) = 0;
    FP_OFF(word_298EC) = 0x4F4;
    FP_SEG(lpm) = 0;
    FP_OFF(lpm) = 0x4F0;
    FP_SEG(commData) = *lpm;
    FP_OFF(commData) = 0;
    FP_SEG(gameData) = *lpm;
    FP_OFF(gameData) = 0x120E;

    if (commData->f70 == 0)
        byte_2C160 = 7;
    else
        byte_2C160 = 0xC;

    installCBreakHandler();
    sub_14107(commData->f1e);
    sub_14107(commData->f1a);
    sub_14107(commData->f1c);

    gm = ovlF43_a(0x42);
    ovlF43_10d(gm);
    ovlCall_c71(commData->f20, 2);
    byte_2BE4E = commData->f24;
    initGraphics();
    ovlCall_cf3();
    ovlCall_cee();
    bs = ovlCall_bef();

    word_2C974 = word_2D064 = allocBuffer(0x2EE0);
    word_2C972 = 0;

    if (*word_2B942 == 1) {
        word_2D270 = allocBuffer(bs);
        ovlCall_c4e(0);
        ovlCall_c2b(0xE);
        ovlCall_c53();
        resFileReadBlock("title16.pic", word_2C972, word_2C974);
        sub_14BEE(word_2D064, word_2D270);
        sub_14E9C();
        ovlCall_bea(word_2D270);
        ovlCall_c8a();
        ovlCall_c58();
        byte_20A1A = 0;
        while (byte_20A1A <= 4)
            ;
        ovlCall_cfd();
        sub_14EDA();
        resFileReadBlock("credit16.pic", word_2C972, word_2C974);
        sub_14BEE(word_2D064, word_2D270);
        ovlCall_c53();
        ovlCall_bea(word_2D270);
        ovlCall_c8a();
        ovlCall_c58();
        sub_108B7();
        szj = time(&szm) + 6;
        while (time(&szm) < szj) {
            if (sub_1E1EC() != 0) {
                getch();
                break;
            }
            sub_10810();
        }
        resFileReadBlock("adv.pic", word_2C972, word_2C974);
        sub_14BEE(word_2D064, word_2D270);
        ovlCall_c53();
        ovlCall_bea(word_2D270);
        ovlCall_c8a();
        ovlCall_c58();
        sub_108B7();
        szj = time(&szm) + 5;
        while (time(&szm) < szj) {
            if (sub_1E1EC() != 0) {
                getch();
                break;
            }
            sub_10810();
        }
        freeBuffer(word_2D270);
    }

    if (commData->f72 == 1)
        ovlFee_fd(commData->f48);
    else
        byte_216AA = byte_216AB = 0x80;

    word_22324 = 0;
    byte_2CA6A = 1;
    byte_298F7 = 0;
    byte_2C7D6 = 1;
    byte_2B388 = 0xFF;
    m6 = ovlCall_b6d();
    m4 = ovlCall_c49();

    if (commData->f70 == 0) {
        need = ovlCall_bf4();
        szc = (int16)(((int32)bs << 2) / 16)
            + (int16)((int32)m6 * 3 / 16)
            + 0x11E9 * m4;
        if (szc < need) {
            word_2C7D4 = 1;
            word_2BB74 = m4;
            goto tier_alloc;
        }
        szc = (int16)((int32)m6 * 3 / 16)
            + (int16)((int32)bs / 16)
            + 0x11E9 * m4;
        if (szc < need) {
            word_2C7D4 = 2;
            word_2BB74 = m4;
            goto tier_alloc;
        }
        szc = (int16)((int32)bs / 16)
            + (int16)((int32)m6 / 16)
            + 0x1101 * m4;
        if (szc < need) {
            word_2C7D4 = 0;
            word_2BB74 = m4;
            goto tier_alloc;
        }
        szc = (int16)((int32)bs / 16)
            + (int16)((int32)m6 / 16);
        if (szc < need) {
            word_2C7D4 = 0;
            word_2BB74 = 0;
            goto tier_alloc;
        }
        cleanup();
        putch(0x70);
        sub_1DCAC(0);
tier_alloc:
        switch (word_2C7D4) {
    case 0:
        if (word_2BB74 == 1) {
            word_2D2CC = word_2D2C6 = allocBuffer(0x1010);
            word_2D2CA = 0;
        }
        word_2D06C = allocBuffer(bs);
        word_2D26E = allocBuffer(m6);
        break;
    case 1:
        word_2D06C = allocBuffer(bs);
        byte_2D06A = 0;
        word_2D270 = allocBuffer(bs);
        word_2D2C4 = allocBuffer(bs);
        byte_2C7D7 = 0;
        word_298E0 = allocBuffer(bs);
        byte_2CE26 = 0;
        word_2D26E = allocBuffer(m6);
        byte_2B384 = 0;
        word_2D272 = allocBuffer(m6);
        byte_298F6 = 0;
        word_2D2C8 = allocBuffer(m6);
        if (word_2BB74 == 1) {
            word_2D2CC = word_2D2C6 = allocBuffer(0xE80);
            word_2D2CA = 0;
            resFileReadBlock("Clipc.pak", word_2D2CA += 0, word_2D2CC);
            word_2D2F8 = word_2D2CE = allocBuffer(0x1010);
            word_2D2F6 = 0;
            resFileReadBlock("Notec.pak", word_2D2F6 += 0, word_2D2F8);
        }
        break;
    case 2:
        word_2D06C = allocBuffer(bs);
        word_2D26E = allocBuffer(m6);
        ovlCall_c2b(0xB);
        loadSpriteRes("Maps.spr", word_2D26E);
        word_2D272 = allocBuffer(m6);
        ovlCall_c2b(1);
        loadSpriteRes("arming.spr", word_2D272);
        word_2D2C8 = allocBuffer(m6);
        if (word_2BB74 == 1) {
            word_2D2CC = word_2D2C6 = allocBuffer(0xE80);
            word_2D2CA = 0;
            resFileReadBlock("Clipc.pak", word_2D2CA += 0, word_2D2CC);
            word_2D2F8 = word_2D2CE = allocBuffer(0x1010);
            word_2D2F6 = 0;
            resFileReadBlock("Notec.pak", word_2D2F6 += 0, word_2D2F8);
        }
        break;
        }
    }

    byte_29B50 = 0;
    sub_108B7();

    while (byte_2C160 != 0x15 && byte_2C160 != 0xC) {
        switch (byte_2C160) {
        case 7:  sub_18F12(); break;
        case 8:  sub_193EE(); break;
        case 15: sub_1A68C(); break;
        case 1:  sub_1ACA0(); break;
        case 9:  sub_1AE22(); break;
        case 3:  sub_1AFA8(); break;
        case 10: sub_1B184(); break;
        case 11: sub_1B304(); break;
        case 2:  sub_1CBD8(); break;
        case 5:  printMission(); break;
        case 4:  sub_1B452(); break;
        case 6:  sub_1CE56(); break;
        default: byte_2C160 = 0xC; break;
        }
        if (commData->f72 == 1) {
            while (ovlCall_ccb(0) != 0)
                ;
            delayTicks(5);
            while (ovlCall_ccb(0) != 0)
                ;
        }
        sub_10810();
    }

    byte_2C160 = 0xC;
    sub_146E3();
    *word_2B942 = 0;
    drawStoreIcons();
    sub_14622((void *)word_200E6, 0, 0, 0x13F, 0xC7);

    if (commData->f70 == 1) {
        gameData->f38 = 1;
        gameData->f3a = 1;
        gameData->f3c = 1;
        gameData->f3e = 1;
        gameData->name[0] = 'S';
        gameData->name[1] = 'i';
        gameData->name[2] = 'd';
        gameData->name[3] = 0;
        gameData->f0 = 2;
        commData->f22 = 1;
        missionGenerate();
    }

    szn = 0;
    goto Lcond;
Lrand2:
    if (randMul(9) > 6)
        goto Linc;
Lset:
    word_2B38E[szn].pad4 = 0;
Linc:
    szn++;
Lcond:
    if ((uint16)objectCount < (uint8)szn)
        goto Ldone;
    if (objectActive[(uint8)szn] <= 1)
        goto Linc;
    if (objectActive[(uint8)szn] == 2)
        goto Lrand2;
    if (objectActive[(uint8)szn] != 3)
        goto Linc;
    if (randMul(9) <= 6)
        goto Linc;
    goto Lset;
Ldone:

    exportWorldToComm("temp.wld");

    if (commData->f2a == 0) {
        for (szn = 0; szn < 4; szn++)
            commData->missionStat[szn] = 0;
    }
    commData->f26 = 3;
    commData->f28 = 0;
    commData->f2c = 0;
    if (gameData->f3c > 1)
        commData->f30 = 1;
    else
        commData->f30 = 0;
    ovlCall_cd0();
    sub_1DCAC(byte_2C160);
}

/* seg000:0x0810 — if the abort flag is up, restore state and quit. */
void sub_10810(void) {
    if (byte_212BA != 0) {
        sub_10882();
        sub_146E3();
        sub_1DCAC(0);
    }
}

/* seg000:0x08b5 — empty slot (bare retn). */
void sub_108B5(void) { }

/* seg000:0x08b6 — empty slot (bare retn). */
void sub_108B6(void) { }

/* seg000:0x4cca — empty slot (bare retn). */
void sub_14CCA(void) { }

/* seg000:0x08b7 — pump the overlay until its poll reports settled. */
void sub_108B7(void) {
    while (ovlCall_cbc() == 0)
        ovlCall_cc1();
}
