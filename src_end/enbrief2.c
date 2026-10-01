/* END.EXE — map-view resource loader (compiled /Ot; separate original module
 * from the /Os enbrief.c — ref emits a branch-target alignment nop at the
 * per-channel loop head that /Os would suppress). */
#include "inttype.h"

/* record fields as the animation DSL addresses them (recOff = byte offset) */
#pragma pack(1)
struct AnimRec {
    int16 posX;             /* +0x00 */
    int16 posY;             /* +0x02 */
    int16 maxA;             /* +0x04 counter-A wrap max */
    int16 maxB;             /* +0x06 counter-B wrap max */
    uint8 field8;           /* +0x08 */
    uint8 field9;           /* +0x09 */
    uint8 cntA;             /* +0x0A frame counter A */
    uint8 cntB;             /* +0x0B frame counter B */
    uint8 chanIdx;          /* +0x0C channel index */
    uint8 repeat[10];       /* +0x0D repeat counter[channel] */
    uint8 strPos[11];       /* +0x17 strPos[channel] */
    int16 frameW[18];       /* +0x22 frame word table */
    uint8 frameB[18];       /* +0x46 frame byte table */
    int16 strOff;           /* +0x58 DSL script offset */
    uint8 active;           /* +0x5A */
    uint8 pad5b;
};
/* companion 0x49-stride channel array parallel to AnimRec (0x3426) */
struct ChanRec {
    uint8  f0;              /* +0x00 channel/sub-frame count */
    uint8  f1[18];          /* +0x01 */
    uint8  f2[18];          /* +0x13 */
    uint8  f3[18];          /* +0x25 */
    uint8  f4[18];          /* +0x37 */
};                          /* total 0x49 */
#pragma pack()

extern int16  word_1BADE, word_1BADC, word_1BADA;
extern uint8  byte_1C6E0, byte_1C6E1, byte_1C6E2, byte_1C6E3,
              byte_1C6E4, byte_1C6E5, byte_1C6E6;
extern uint8 *word_1C6E8, *word_1C6EA;     /* decode staging bufs (0x2150/0x2550) */
extern int16  word_23C62;                  /* pic stream handle */
extern int16  word_2244C, word_23C7C;      /* alloc'd block / +0x800 shadow page */
extern int16  word_1BA9E, word_1BA92;      /* gfx page copies */
extern int16 *word_1BAD0, *word_1BACE;     /* display-seg slot ptrs */
extern int16 *word_1BAD2, *word_1BAD4;     /* sprite slot A/B (src/dst) */
extern int16  word_1DA44, word_1DA46, word_1DA48;  /* record count / w,h */
extern int16  word_2351E[];                /* scanline->plane-offset LUT (200) */
extern uint8  byte_2089D[], byte_208AD[], byte_208BD[], byte_208CD[];
extern int16  word_1BAE0[], word_1BAE2[];  /* packed channel word arrays */
extern int16  initResultFlag;              /* word_22906 — display mode selector */
extern int16  word_23786, word_23788;      /* map scroll X / Y */
extern int16  openFileWrapper(const char *name, int16 mode);
extern void   picStreamRead(int16 fd);                       /* sub_11521 */
extern int16  allocClearBuf(int32 a);                       /* enfile.c; caller pushes long, callee reads lo word */
extern int16  readPicStream(uint8 *dst, int16 count, int16 fd);
extern int16  sub_11A1C(int16 count, int16 fd);              /* append script chunk */
extern void   closeFileWrapper(int16 fd);
extern void   sub_12FE8(int16 a, int16 b, int16 c, int16 d);
extern void   sub_12EF2(int16 *p1, int16 a2, int16 a3, int16 *p4,
                        int16 a5, int16 a6, int16 a7, int16 a8);
extern void   sub_133BA(int16,int16,int16,int16,int16,int16);
extern void   sub_133BD(int16,int16,int16,int16,int16,int16);
extern void   sub_133EC(int16,int16,int16,int16,int16,int16);
extern void   sub_13436(int16,int16,int16,int16,int16,int16);
extern void   far gfx_drvMode(void);        /* far call 9D9:13FF driver slot */

/* ==== loadMapView — map-view resource loader (AnimRec + palette + mode setup) ==== */
int16 loadMapView(char *fname, int16 a2) {
    struct AnimRec *pr;      /* [bp-2] */
    int16 d0;                /* [bp-4] dead slot */
    int16 cnt;               /* [bp-6] */
    int16 i;                 /* [bp-8] */
    int16 j;                 /* [bp-0A] shared index/scratch */
    struct ChanRec *ch;      /* [bp-0C] */

    word_1BADE = 0;
    byte_1C6E0 = 0;
    byte_1C6E1 = 0;
    byte_1C6E2 = 0;
    byte_1C6E3 = 0;
    byte_1C6E4 = 0;
    byte_1C6E5 = 0;
    byte_1C6E6 = 0;

    for (i = 0; i < 0xC8; i++)                          /* line-offset LUT */
        word_2351E[i] = (i / 4) * 0xA0 + ((i & 3) << 13);

    word_23C62 = openFileWrapper(fname, 0);
    picStreamRead(word_23C62);
    word_1BADC = 0;
    word_2244C = allocClearBuf(0xFFFFL);
    word_23C7C = word_2244C + 0x800;
    if (initResultFlag == 2)
        gfx_drvMode();
    word_1BA9E = word_2244C;
    word_1BA92 = word_23C7C;
    word_1BADA = *word_1BAD0;
    word_1C6E8 = (uint8 *)0x2150;
    word_1C6EA = (uint8 *)0x2550;
    readPicStream((uint8 *)0x2150, 0x20, word_23C62);

    for (i = 0; i < 0x10; i++) {                        /* palette nibble expand */
        j = word_1C6E8[i*2]   & 0xF0;  byte_2089D[i] = (j >> 4) | j;
        j = word_1C6E8[i*2]   & 0xF;   byte_208AD[i] = (j << 4) | j;
        j = word_1C6E8[i*2+1] & 0xF0;  byte_208BD[i] = (j >> 4) | j;
        j = word_1C6E8[i*2+1] & 0xF;   byte_208CD[i] = (j << 4) | j;
    }

    readPicStream(word_1C6E8, 6, word_23C62);
    word_1DA44 = word_1C6E8[0];
    byte_1C6E2 = word_1C6E8[1];
    word_1DA46 = (word_1C6E8[2] << 8) + word_1C6E8[3];
    word_1DA48 = (word_1C6E8[4] << 8) + word_1C6E8[5];
    readPicStream(word_1C6E8, 7, word_23C62);

    for (i = 0; i < word_1DA44; i++) {                  /* load records */
        pr = (struct AnimRec *)(i * 0x5C + 0x295E);
        ch = (struct ChanRec *)(i * 0x49 + 0x3426);
        readPicStream(word_1C6E8, 0xB, word_23C62);
        pr->posX   = (word_1C6E8[0] << 8) + word_1C6E8[1];
        pr->posY   = (word_1C6E8[2] << 8) + word_1C6E8[3];
        pr->maxA   = (word_1C6E8[4] << 8) + word_1C6E8[5];
        pr->maxB   = (word_1C6E8[6] << 8) + word_1C6E8[7];
        pr->field8 = word_1C6E8[8];
        pr->field9 = word_1C6E8[9];
        pr->cntA   = word_1C6E8[10];
        ch->f0 = pr->field9;
        pr->strOff = sub_11A1C(pr->cntA, word_23C62);
        pr->field9 = 0;
        pr->cntA = 0;
        pr->cntB = 0;
        pr->chanIdx = 0;
        pr->repeat[0] = 1;
        pr->strPos[0] = 0;
        pr->active = 1;
        for (j = 0; j < ch->f0; j++) {                  /* per-channel recs */
            readPicStream(word_1C6E8, 7, word_23C62);
            pr->frameW[j] = *(int16 *)word_1C6E8;
            pr->frameB[j] = word_1C6E8[2];
            ch->f1[j] = word_1C6E8[3];
            ch->f2[j] = word_1C6E8[4];
            ch->f3[j] = word_1C6E8[5];
            ch->f4[j] = word_1C6E8[6];
        }
    }

    readPicStream(word_1C6E8, 3, word_23C62);
    cnt = (word_1C6E8[0] << 8) + word_1C6E8[1];
    byte_1C6E4 = word_1C6E8[2];
    readPicStream(word_1C6E8, cnt * 2, word_23C62);
    for (i = 0; i < cnt * 2; i++)
        word_1BAE0[i] = word_1C6E8[i];
    readPicStream(word_1C6E8, cnt, word_23C62);
    for (i = 0; i < cnt; i++) {                          /* bit-pack decode */
        j = i * 2;
        word_1BAE0[j] = (((word_1C6E8[i] & 0x70) << 4) | word_1BAE0[j])
                        * ((word_1C6E8[i] & 0x80) ? 0xFFFF : 1);
        word_1BAE2[j] = (((word_1C6E8[i] & 7) << 8) | word_1BAE2[j])
                        * ((word_1C6E8[i] & 8) ? 0xFFFF : 1);
    }

    i = byte_1C6E2;
    byte_1C6E2 = 0;
    sub_12FE8(word_23786, word_23788, word_1DA46, word_1DA48);
    byte_1C6E2 = i;
    sub_12EF2(word_1BAD0, word_23786, word_23788, word_1BACE,
              word_23786, word_23788, word_1DA46, word_1DA48);

    for (i = 0; i < word_1DA44; i++) {                   /* draw first frames */
        pr = (struct AnimRec *)(i * 0x5C + 0x295E);
        sub_12EF2(word_1BACE, pr->posX + word_23786, pr->posY + word_23788,
                  word_1BAD0, pr->frameW[0], pr->frameB[0], pr->maxA, pr->maxB);
    }

    for (i = 0; i < word_1DA44; i++) {                   /* channel joins */
        pr = (struct AnimRec *)(i * 0x5C + 0x295E);
        ch = (struct ChanRec *)(i * 0x49 + 0x3426);
        for (j = 1; j < ch->f0; j++) {
            sub_12EF2(word_1BAD0, pr->frameW[j-1], pr->frameB[j-1], word_1BAD0,
                      pr->frameW[j], pr->frameB[j], pr->maxA, pr->maxB);
            sub_12FE8(pr->frameW[j] + ch->f1[j], pr->frameB[j] + ch->f2[j],
                      ch->f3[j], ch->f4[j]);
        }
    }

    closeFileWrapper(word_23C62);

    switch (initResultFlag) {                            /* video-mode setup */
    case 0:
        *word_1BAD4 = *word_1BACE;
        *word_1BAD2 = 0xB800;
        sub_133BD(0x28, 0x19, *word_1BAD0, 0, *word_1BAD4, 0);
        break;
    case 1:
        *word_1BAD2 = 0xB800;
        *word_1BAD4 = *word_1BAD0;
        break;
    case 2:
        *word_1BAD2 = 0xA800;
        *word_1BAD4 = 0xA400;
        sub_133BA(0x28, 0x19, *word_1BAD0, 0, *word_1BAD4, 0);
        *word_1BACE = 0xA800;
        *word_1BAD2 = 0xA000;
        break;
    case 3:
        sub_133EC(0x28, 0x19, *word_1BAD0, 0, *word_1BACE, 0);
        *word_1BAD2 = 0xA000;
        *word_1BAD4 = *word_1BAD0;
        sub_13436(0x28, 0x19, *word_1BACE, 0, *word_1BAD4, 0);
        break;
    }
    return 1;
}
