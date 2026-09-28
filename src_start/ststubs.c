/* src/stubs.c overflowed CL's heap under -DEXE_START; the stmenu.c globals
 * added for sub_1A68C live here instead (satellite builds compile every
 * src_start/*.c module, so these defs link in like any other). */
#include "inttype.h"

int16 word_2D2C6;                            /* sub_161CC arg */
int16 word_22324;                            /* sub_161CC side flag */
int16 word_2C972, word_2C974;                /* sub_14746 args */
uint8 byte_298E9, byte_2B384;                /* res-loaded flags */
int16 word_2D064, word_2D270;                /* sub_14BEE args */
int16 *word_25B2C, *word_25B44, *word_25CF6; /* page record handles */
int16 word_25CE6;                            /* widget param */
uint8 far *word_209AA;                       /* far ptr→item count byte */
int16 word_25B46[10 * 16];                   /* dseg:0x5b46, stride 0x20 */
int16 word_25B64, word_25B84;                /* theater 0 b4f args */
int16 word_25BA4, word_25BC4;                /* theater 1 */
int16 word_25BE4, word_25C04;                /* theater 2 */
int16 word_25C24, word_25C44;                /* theater 3 */
int16 word_25C64, word_25C84;                /* theater 4 */
void  sub_14BEE(int16 a, int16 b) { }
void  sub_161CC(int16 a, int16 b, int16 c) { }
void  far ovlCall_c4e(int16 v) { }           /* overlay 1000:0c4e */
void  far ovlCall_bea(int16 v) { }           /* overlay 1000:0bea */
