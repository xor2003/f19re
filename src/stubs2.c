/* stubs.c overflow — second unit (MSC heap limit) */
#include "inttype.h"

int16  word_1E25C[16], word_1E280[16];          /* dseg:0x44cc/0x44f0 — icon params */
struct { int16 w0; int16 pad[8]; } word_22A12[8] = {0};	/* dseg:0x8c82 */
int16  word_1AAD8[64];                          /* dseg:0x0d48 */
int16  word_23C66, word_23C68;                  /* dseg:0x9ed6/8 — commData halves */
int16  word_2243E, word_22440;                  /* dseg:0x86ae/b0 — pilotRec halves */
int8   byte_1D6CE, byte_1D6CF, byte_1D6D2;      /* dseg:0x193e/0x193f/0x1942 */
int8   byte_22F08;                              /* dseg:0x9178 */
void  sub_16486(void) { }                         /* event renderer — skeleton */
void  sub_175BC(void) { }                         /* event renderer — skeleton */
void  sub_1883C(void) { }                         /* landingtype-1 handler — skeleton */
void  sub_10D1A(int16 seg) { }                    /* drv tbl patch — skeleton */
int16 far gfx_getConst1(void) { return 0; }       /* 9D9:149F */
int16 far gfx_getAuxBufSize(void) { return 0; }   /* 9D9:1445 */
void  far joyTableSetup(char far *p) { }          /* 9C7:0109 */
