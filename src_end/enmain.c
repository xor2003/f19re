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

extern void intDispatch(int16 intNum, uint8 *inRegs, uint8 *outRegs); /* sub_139FD */
extern void far misc_jump_5e_clearKeyFlags(void);   /* 9D9:1526 */
extern void restoreTimerIrqHandler(void);           /* sub_13664 */
extern void restoreCbreakHandler(void);             /* sub_11264 */
extern int16 far misc_jump_5a_keybuf(void);         /* 9D9:1512 */
extern int16 far misc_jump_5b_getkey(void);         /* 9D9:1517 */
extern int16 far misc_jump_5d_readJoy(int16 a);     /* 9D9:1521 */
extern void serviceTick(void);                      /* sub_1281B */
extern void exit(int16 code);                       /* sub_18AD2 */
extern uint8 quitFlag;                              /* byte_1B6D2 — enbrief.c */

uint8 timerHandlerInstalled;                        /* byte at dseg:0x41bf */

/* seg000:0x0398 — f15se2 shared/cleanup.c: timer IRQ, text mode, key flags */
void cleanup(void) {
    uint8 regs[0xe];

    if (timerHandlerInstalled == 1) {
        restoreTimerIrqHandler();
    }
    regs[1] = 0;
    regs[0] = 3;
    intDispatch(0x10, regs, regs);
    misc_jump_5e_clearKeyFlags();
}

/* seg000:0x03d5/0x03e6 — empty teardown hooks (f15 enmisc.c) */
void restoreVideoMode(void) { }
void restoreInterrupts(void) { }

/* seg000:0x0655 */
void clearKeybuf(void) {
    while (misc_jump_5a_keybuf() == 0) {
        misc_jump_5b_getkey();
    }
}

/* seg000:0x067b — f15 eninput.c waitForKeyOrJoy (END drops the quitFlag arm) */
void waitForKeyOrJoy(void) {
    int16 key;

    if (commData->setupUseJoy == 1) {
        while (misc_jump_5a_keybuf() != 0 && misc_jump_5d_readJoy(0) == 0) {
        }
        if (misc_jump_5a_keybuf() == 0) {
            key = misc_jump_5b_getkey();
        }
    } else {
        key = misc_jump_5b_getkey();
    }
    if (key == 0x1000) {                            /* KEYCODE_ALTQ */
        cleanup();
        if (quitFlag != 0) {
            restoreCbreakHandler();
        }
        exit(0);
    }
}

/* seg000:0x0702 — F19 variant: serviceTick() pumped while polling */
void waitForKeyOrJoy2(void) {
    int16 key;

    if (commData->setupUseJoy == 1) {
        while (misc_jump_5a_keybuf() != 0 && misc_jump_5d_readJoy(0) == 0) {
            serviceTick();
        }
        if (misc_jump_5a_keybuf() == 0) {
            key = misc_jump_5b_getkey();
        }
    } else {
        while (misc_jump_5a_keybuf() != 0) {
            serviceTick();
        }
        key = misc_jump_5b_getkey();
    }
    if (key == 0x1000) {
        cleanup();
        if (quitFlag != 0) {
            restoreCbreakHandler();
        }
        exit(0);
    }
}

