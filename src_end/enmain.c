/* END.EXE — graphics init (f15se2 enmain.c lineage) */
#include "inttype.h"

struct CommDataEnd {                            /* far ptr word_23C66 */
    int8 pad00[0x1a];
    int16 field1a;                              /* 0x1a — res driver seg */
    int8  pad1c[0x02];
    int16 field1e;                              /* 0x1e — res driver seg */
    int16 gfxInitResult;                        /* 0x20 */
    int8 pad22[0x02];
    int16 setupMono;                            /* 0x24 */
    int16 landingType;                          /* 0x26 */
    int8 pad28[0x08];
    int16 trainingFlag;                         /* 0x30 */
    int8 pad32[0x40];
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


/* ==== seg000:0x0010 — debrief main driver: pull the inter-exe handoff ptr at
 * 0:0x4F0 (commData seg / pilotRec seg), init graphics + buffers, dispatch on
 * landingType, then run the four debrief screens with joy debounce between. */
extern int16  sub_17248(void);                  /* seg000:0x7248 */
extern void   sub_17094(void);                  /* seg000:0x7094 */
extern void   sub_1714C(void);                  /* seg000:0x714c */
extern void   sub_17280(void);                  /* seg000:0x7280 */
extern void   sub_16486(void);                  /* seg000:0x6486 */
extern void   sub_17334(void);                  /* seg000:0x7334 */
extern void   sub_175BC(void);                  /* seg000:0x75bc */
extern void   sub_16076(void);                  /* seg000:0x6076 */
extern void   sub_1883C(void);                  /* seg000:0x883c */
extern void   sub_10D1A(int16 seg);             /* seg000:0x0d1a — drv tbl patch */
extern void   loadWorldStrings(void);           /* seg000:0x03f7 */
extern void   installCBreakHandler(void);       /* seg000:0x1241 */
extern void   timerWait(uint16 ticks);          /* seg000:0x0cd9 */
extern uint16 allocBuffer(int16 size);          /* seg000:0x2e52 */
extern int16  far gfx_getAuxBufSize(void);      /* 9D9:1445 */
extern int16  far gfx_getBufSize(void);         /* 9D9:13C3 */
extern int16  far gfx_getConst1(void);          /* 9D9:149F */
extern void   far joyTableSetup(char far *p);   /* 9C7:0109 */
extern int16  word_18EA2;                       /* dseg:0x8ea2 — gfx init flag */
extern int16  word_1295C;                       /* dseg:0x295c */
extern int8   byte_199F4;                       /* dseg:0x99f4 — display flag */
extern int8   byte_22F08;                       /* dseg:0x9178 — mono copy */
extern int16  word_23C6C;                       /* dseg:0x9edc — res page */
extern int16  word_23C76, word_23C78, word_23C7A;/* dseg:0x9ee6/8/a */
extern int8   byte_1D6CE, byte_1D6CF;           /* dseg:0x193e/f — joy center */
extern int8   byte_1D6D2;                       /* dseg:0x1942 — quit flag */
extern int16  missionResult;                    /* word_2351C */

struct PilotRecMain {                           /* far ptr word_2243E */
    int8  pad0[0x36];
    uint16 missionCount;                        /* 0x36 */
    int8  pad38[0x16];
    int16 field4e;                              /* 0x4e */
};
extern struct PilotRecMain far *pilotRec;

/* seg000:0x02fd — quit hook: if the user asked to quit, clean up + exit */
void sub_102FD(void)
{
    if (byte_1D6D2 != 0) {
        cleanup();
        restoreCbreakHandler();
        exit(0);
    }
}

void sub_10010(void)
{
    union { int16 w[2]; int16 far *p; } uh;      /* handoff far cell */
    int16 bufsz, k, asx, ui, uj;

    uh.w[1] = 0;
    uh.w[0] = 0x4f0;
    ((int16 *)&commData)[1] = *uh.p;
    ((int16 *)&commData)[0] = 0;
    ((int16 *)&pilotRec)[1] = *uh.p;
    ((int16 *)&pilotRec)[0] = 0x120e;
    sub_10D1A(commData->field1a);
    sub_10D1A(commData->field1e);
    misc_jump_5e_clearKeyFlags();
    clearKeybuf();
    word_18EA2 = gfx_getConst1();
    word_1295C = 0;
    byte_199F4 = 0;
    byte_22F08 = (int8)commData->setupMono;
    if (commData->trainingFlag == 0)
        pilotRec->missionCount++;
    installCBreakHandler();
    initGraphics();
    if (commData->setupUseJoy == 1) {
        joyTableSetup((char far *)commData + 0x48);
    } else {
        byte_1D6CE = byte_1D6CF = 0x80;
    }
    loadWorldStrings();
    asx = gfx_getAuxBufSize();
    bufsz = gfx_getBufSize();
    word_23C6C = allocBuffer(asx);
    if (word_18EA2 == 1) {
        word_23C76 = allocBuffer(0x3c8c);
        word_23C7A = word_23C76;
        word_23C78 = 0;
    }
    missionResult = 3;
    switch (commData->landingType) {
    case 1:
        sub_1883C();
        break;
    case 2:
        switch (sub_17248()) {
        case 0:
            sub_17094();
            break;
        default:
            sub_1714C();
            break;
        }
        break;
    case 3:
        sub_17280();
        break;
    }
    if (commData->setupUseJoy == 1) {
        while (misc_jump_5d_readJoy(0) != 0) { }
        timerWait(5);
        while (misc_jump_5d_readJoy(0) != 0) { }
    }
    sub_102FD();
    clearKeybuf();
    sub_16486();
    if (commData->setupUseJoy == 1) {
        while (misc_jump_5d_readJoy(0) != 0) { }
        timerWait(5);
        while (misc_jump_5d_readJoy(0) != 0) { }
    }
    sub_102FD();
    clearKeybuf();
    sub_17334();
    sub_102FD();
    clearKeybuf();
    sub_175BC();
    if (commData->setupUseJoy == 1) {
        while (misc_jump_5d_readJoy(0) != 0) { }
        timerWait(5);
        while (misc_jump_5d_readJoy(0) != 0) { }
    }
    sub_102FD();
    clearKeybuf();
    sub_16076();
    if (commData->setupUseJoy == 1) {
        while (misc_jump_5d_readJoy(0) != 0) { }
        timerWait(5);
        while (misc_jump_5d_readJoy(0) != 0) { }
    }
    sub_102FD();
    clearKeybuf();
    if (pilotRec->missionCount == 0x63)
        pilotRec->field4e = 1;
    if (commData->landingType == 1 && commData->trainingFlag == 0)
        pilotRec->field4e = 2;
    exit(0x23);
}
