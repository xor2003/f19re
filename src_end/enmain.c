/* END.EXE — graphics init (f15se2 enmain.c lineage) */
#include "inttype.h"

struct CommDataEnd {                            /* far ptr word_23C66 */
    int8 pad20[0x20];
    int16 gfxInitResult;                        /* 0x20 */
    int8 pad22[0x02];
    int16 setupMono;                            /* 0x24 */
    int8 pad26[0x4c];
    int16 setupUseJoy;                          /* 0x72 */
};
extern struct CommDataEnd far *commData;

extern void seedRandom(void);                   /* sub_10CF3 */
extern int16 readBiosTickLo(void);              /* sub_13844 */
extern void seedRandom16(int16 v);              /* sub_18C84 — srand */
extern void far gfx_setPageN(uint16 n);
extern int16 far gfx_allocPage(int16 page);
extern void far gfx_getCurPage(int16 a);
extern void far gfx_setMonoFlag(int16 mono);
extern void far gfx_setDac(int16 a);
extern void far gfx_storeBufPtr(int16 ptr, int16 n);
extern int16 far gfx_initDone(void);

int16 initResultFlag;                           /* word_22906 */

/* seg000:0x0328 */
void initGraphics(void) {
    int16 a, b, c, d, e, f, g, h;               /* 0x10 chkstk frame */
    (void)a; (void)b; (void)c; (void)d;
    (void)e; (void)f; (void)g; (void)h;
    seedRandom();
    gfx_setPageN(0);
    gfx_allocPage(0);
    gfx_getCurPage(0);
    gfx_setMonoFlag(commData->setupMono);
    gfx_setDac(1);
    gfx_storeBufPtr(commData->gfxInitResult, 1);
    initResultFlag = gfx_initDone();
}

/* seg000:0x0cf3 */
void seedRandom(void) {
    seedRandom16(readBiosTickLo());
}
