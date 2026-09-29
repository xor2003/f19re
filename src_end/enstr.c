/* END.EXE — string helpers (seg000:0x38ba-0x394a, f15 util lineage).
 * Same cluster as START's stutil.c twins; see notes below for the
 * hand-asm members that stay in the skeleton. */
#include "inttype.h"

void mystrcpy(char *dst, const char *src) {   /* seg000:0x38ba */
    do {
    } while ((*dst++ = *src++) != '\0');
}

void mystrcat(char *d, char *s) {         /* seg000:0x3923 */
    for (;;) {
        if (*d == 0) break;
        d++;
    }
    for (; (*d = *s++) != 0; d++) ;
}

/* seg000:0x38d5 strcpyToFar, 0x38ec farStrcpy — hand-asm (LES/LDS +
 * LODSB/STOSB, register-save pattern). Skeleton. */
/* seg000:0x3907 mystrlen — hand-asm: preloads s into ax pre-loop,
 * ~(s_orig - s_end) tail, zero locals. Skeleton. */
/* seg000:0x394b mystrchr — hand-asm (push si never used, ch hoisted to ax
 * before loop); skeleton stays. */
/* seg000:0x396e memsetNear, 0x3982 memsetFar, 0x39b6 memcpyFromFar —
 * rep-stosb/movsb asm; 0x3998 copyBytes (`loop`) and 0x39d2 memeq
 * (byte-stepped word cmp + `loope`) likewise. Skeleton. */
