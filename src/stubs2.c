/* stubs.c overflow — second unit (MSC heap limit) */
#include "inttype.h"

int16  word_1E25C[16], word_1E280[16];          /* dseg:0x44cc/0x44f0 — icon params */
struct { int16 w0; int16 pad[8]; } word_22A12[8] = {0};	/* dseg:0x8c82 */
int16  word_1AAD8[64];                          /* dseg:0x0d48 */
int16  word_23C66, word_23C68;                  /* dseg:0x9ed6/8 — commData halves */
int16  word_2243E, word_22440;                  /* dseg:0x86ae/b0 — pilotRec halves */
int8   byte_1D6CE, byte_1D6CF, byte_1D6D2;      /* dseg:0x193e/0x193f/0x1942 */
int8   byte_22F08;                              /* dseg:0x9178 */
/* sub_16486/sub_175BC now real in src_end/endevt.c */
int16 *word_1F426;                              /* dseg:0x5696 — eval panel ptr */
void  clearRect(int16 *item, int16 x1, int16 y1, int16 x2, int16 y2) { } /* 0xdb2 — skeleton */
int16 *awardTextItem;                           /* dseg:0x61e6 — award text item */
int16  awardStatGrid[8][45];                    /* dseg:0x61e8 — stat panel rows */
int16  ribbonItems[17][15];                     /* dseg:0x647c — ribbon sprites */
int16 *purpleHeartSpr;                          /* dseg:0x66ae */
int16 *medalSpriteTab[8];                       /* dseg:0x66a2 */
int16 *rankSpriteA[8], *rankSpriteB[8], *rankSpriteC[8];  /* 0x667a/88/96 */
char  *medalNames[8];                           /* dseg:0x66e8 */
char  *queuedAwardName[8];                      /* dseg:0x66f4 */
char  *ribbonNames[8];                          /* dseg:0x66fe */
char  *newRankNames[8];                         /* dseg:0x6706 */
char  *nextRankNames[8];                        /* dseg:0x6714 */
int16  ribbonIcons[11][9];                      /* dseg:0x670e */
int16  ribbonPos[11][9];                        /* dseg:0x67b0 */
char  *malloc(int16 size) { return (char *)0; }   /* 0x8c20 */
void   free(char *p) { }                          /* 0x8c0e */
void  sub_1883C(void) { }                         /* landingtype-1 handler — skeleton */
void  sub_10D1A(int16 seg) { }                    /* drv tbl patch — skeleton */
int16 far gfx_getConst1(void) { return 0; }       /* 9D9:149F */
int16 far gfx_getAuxBufSize(void) { return 0; }   /* 9D9:1445 */
void  far joyTableSetup(char far *p) { }          /* 9C7:0109 */
