/* egmain.c — program entry / session init (F19) */
#include "inttype.h"
#include "pointers.h"
#include <dos.h>
#include <stdlib.h>
#include <string.h>

/* private helpers — still asm */
int16 getOverlayLoadSeg(int16 slot);            /* sub_102B4 — exec/mem probe, returns gfx ovl load seg */
void  gfxInit(void);                            /* sub_1026A */
void  setupOverlaySlots(uint16 addr);           /* sub_103B2 */
void  drawCockpit(void);                        /* sub_10170 */
void  runGameSession(void);                     /* sub_101FC */
void  installCBreakHandler(void);               /* sub_11C83 */
void  restoreCbreakHandler(void);               /* sub_11CA6 */
void  far gfx_initOverlay(void);                /* sub_2F066 */
void  far gfx_setMonoFlag(int16 mono);          /* sub_2F1C4 */
void  far gfx_waitRetrace(void);                /* sub_2F183 */
void  far gfx_setFadeSteps(int16 steps);        /* sub_2F15B */
void  far setupInstrumentLayoutFar(void);       /* sub_2208A (seg002 trampoline) */
void  far copyJoystickData(uint8 far *data);    /* sub_22D57 */
int16 far restoreJoystickData(uint8 far *data); /* sub_22D45 */

struct CommData {
    int8   pad0[0x1A];
    uint16 gfxOvlAddr;          /* +0x1A — rewritten via getOverlayLoadSeg when gfxModeNum==0 */
    uint16 sndOvlAddr;          /* +0x1C */
    uint16 miscOvlAddr;         /* +0x1E */
    int16  gfxInitResult;       /* +0x20 */
    int8   pad22[2];
    int16  setupMono;           /* +0x24 */
    int8   pad26[0x22];
    uint8  joyData[0x14];       /* +0x48 */
    int8   pad5C[0x16];
    int16  setupUseJoy;         /* +0x72 */
    int8   pad74[4];
    int16  gfxModeNum;          /* +0x78 — 0 = driver not preselected */
};
extern struct CommData FAR *commData;           /* dword_38B10 */
extern uint16 FAR *g_viewParamsFar;             /* dword_354D0 — game data block (commSeg:0x120E; [0x1C]=theater) */

extern int16 g_gfxModeUnset;                    /* word_2EEE6 — 1 when gfxModeNum==0 */
extern int16 g_savedGfxOvl;                     /* word_38D28 — original gfxOvlAddr restored on exit */
extern uint8 hercFlag;                          /* byte_379C0 — setupMono copy */
extern uint8 joyAxes[];                         /* @0x3345A — stick calibration center */
extern int16 gfxBufPtr;                         /* word_38D1C */
extern int8  g_exitStatus;                      /* byte_2EEE5 */
extern union REGS regs;                         /* @0x95DE */
extern uint8 FAR *g_floppyMotorPtr;             /* dword_354C2 — BDA floppy motor count @0000:0440 */
extern char *regnName;                          /* word_2EEE8 -> "regn.xxx" (strcpy dst) */
extern char *scenarioPlh[];                     /* @0x7A — per-theater region names */

int16  far gfx_getModecode(void);               /* sub_2F165 */
void   openBlitClosePic(int16 picId, int16 p);  /* sub_1E40A (pic cluster) */
void   far gfx_copyRect(int16 a, int16 b, int16 c, int16 d,
                        int16 e, int16 f, int16 g, int16 h); /* sub_2F0FC */
void   initStoreData(void);                     /* sub_14D77 — ported in egframe.c */
void   load15Flt3d3(void);                      /* sub_1CB8C — ported in eg3dload.c */
void   initMissionStrings(void);                /* sub_10446 trampoline -> sub_10ABE */
void   far audio_shutdown(void);                /* sub_2F223 */
void   far audio_setup(void);                   /* sub_2F21E */
void   far gfx_setDacAnimCount(int16 n);        /* sub_2F1B5 */
void   setTimerIrqHandler(void);                /* sub_11D12 (asm) */
void   restoreTimerIrqHandler(void);            /* sub_11D59 (asm) */
void   runGameLoop(void);                       /* sub_11CD2 shim -> sub_11CDE */
void   moveDataFar(void);                       /* sub_14EF7 — ported in egframe.c */
void   waitFrameSync(int16 ticks);              /* sub_104E2 — ported in egrender.c */
void   far setInt9Handler(void);                /* seg003:0x000e */
void   far restoreInt9Handler(void);            /* seg003:0x005e */
uint8  far gfx_getModeFlag(void);               /* sub_2F1A6 */
int16  far gfx_allocPage(int16 page);           /* sub_2F02A */
void   far gfx_storeBufPtr(int16 ptr, int16 n); /* sub_2F1A1 */
void   setupDac(void);                          /* sub_11BB4 — int10h AX=1012 (asm) */

extern uint8 g_dacSupported;                    /* byte_2EEE4 */

/* ==== seg000:0x0010 ==== */
int main(void) {
    uint16 FAR *commPtr;
    register int16 commSeg;

    FP_SEG(commPtr) = 0;
    FP_OFF(commPtr) = 0x4F0;            /* DOS ICA — F19 loader stores comm seg here */
    commSeg = *commPtr;
    FP_SEG(commData) = commSeg;
    FP_OFF(commData) = 0;
    FP_SEG(g_viewParamsFar) = commSeg;
    FP_OFF(g_viewParamsFar) = 0x120E;

    if (commData->gfxModeNum == 0) {
        g_gfxModeUnset = 1;
        g_savedGfxOvl = commData->gfxOvlAddr;
    }
    setupOverlaySlots(commData->gfxOvlAddr);
    setupOverlaySlots(commData->miscOvlAddr);
    setupOverlaySlots(commData->sndOvlAddr);
    hercFlag = commData->setupMono;
    installCBreakHandler();
    if (commData->setupUseJoy == 1) {
        copyJoystickData(commData->joyData);
    } else {
        joyAxes[0] = joyAxes[1] = 0x80;
    }
    gfxInit();
    gfx_initOverlay();
    gfx_setMonoFlag(commData->setupMono);
    gfx_waitRetrace();
    gfx_setFadeSteps(g_viewParamsFar[0x1C] < 2 ? 0xC : 0x10);
    gfxBufPtr = commData->gfxInitResult;
    setupInstrumentLayoutFar();
    drawCockpit();
    runGameSession();
    if (commData->setupUseJoy == 1) {
        restoreJoystickData(commData->joyData);
    }
    if (commData->gfxModeNum == 0) {
        commData->gfxOvlAddr = g_savedGfxOvl;
    }
    restoreCbreakHandler();
    if (g_exitStatus == 0) {
        regs.h.ah = 0;
        regs.h.al = 3;
        int86(0x10, &regs, &regs);
    }
    exit((uint8)g_exitStatus);
}

/* ==== seg000:0x0170 ==== */
void drawCockpit(void) {
    if (gfx_getModecode() == 3)
        openBlitClosePic(0x82, 1);
    else
        openBlitClosePic(0x8D, 1);
    gfx_copyRect(1, 0, 0x6D, 0, 0, 0x6D, 0x140, 0x5B);
    gfx_copyRect(1, 0, 0x6D, 2, 0, 0x6D, 0x140, 0x5B);
    initStoreData();
    load15Flt3d3();
    strcpy(regnName, scenarioPlh[g_viewParamsFar[0x1C]]);
    initMissionStrings();
}

/* ==== seg000:0x01fc ==== */
void runGameSession(void) {
    FP_OFF(g_floppyMotorPtr) = 0x440;
    FP_SEG(g_floppyMotorPtr) = 0;
    if (*g_floppyMotorPtr > 1)
        *g_floppyMotorPtr = 1;
    audio_shutdown();
    audio_setup();
    setTimerIrqHandler();
    if (commData->setupUseJoy == 0)
        setInt9Handler();
    runGameLoop();
    moveDataFar();
    if (commData->setupUseJoy == 0)
        restoreInt9Handler();
    gfx_setDacAnimCount(1);
    waitFrameSync(2);
    restoreTimerIrqHandler();
    audio_shutdown();
}

/* ==== seg000:0x026a ==== */
void gfxInit(void) {
    int16 page;

    if ((g_dacSupported = gfx_getModeFlag()) != 0)
        setupDac();
    page = gfx_allocPage(1);
    gfx_storeBufPtr(page, 1);
    gfx_storeBufPtr(commData->gfxInitResult, 2);
}
