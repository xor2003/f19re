/* egkeys.c — sound / timescale routines (F19) */
#include "inttype.h"

/* ==== seg000:0xdef2 ==== */
extern int16 g_soundPriorityFloor;    /* word_34B08 */
extern int16 g_ejectState;            /* word_382D8 */
void FAR audio_playSound(int16 id);   /* sub_2F228 thunk */
void updateEngineSound(void);         /* sub_1DF1A */

void makeSound(int16 soundId, int16 priority) {
    if (priority >= g_soundPriorityFloor) {
        if (g_ejectState == 0 || priority > 1) {
            audio_playSound(soundId);
        }
    }
    updateEngineSound();
}

/* ==== seg000:0xdf1a ==== */
extern int16 g_engineThrust;          /* word_33588 */
void FAR audio_engineDroneOn(void);   /* sub_2F232 thunk */
void FAR audio_engineDroneOff(void);  /* sub_2F237 thunk */
void updateEngineSound(void) {
    if (g_soundPriorityFloor == 0 && g_engineThrust != 0 && g_ejectState == 0)
        goto droneOn;
    audio_engineDroneOff();
    return;
droneOn:
    audio_engineDroneOn();
}

/* ==== seg000:0xdf3c ==== */
extern int16 g_frameRateScaling;      /* word_33D92 */
extern int16 g_frameSyncWait;         /* word_34AFE */
extern int16 g_timeAccelMode;         /* word_343D0 */
extern int16 g_bulletTrackCount;      /* word_373EC */
extern int16 g_threatTimerInit;       /* word_3758A */
extern int16 g_threatDisplayTtl;      /* word_35D38 */
int16 clampRange(int16 v, int16 lo, int16 hi);

void recalcTimeScale(void) {
    if (g_frameRateScaling > 15) {
        g_frameSyncWait = clampRange((-(120 / g_frameRateScaling - 9)) >> 1, 1, 4);
    } else {
        g_frameSyncWait = 0;
    }
    g_frameRateScaling = clampRange(g_frameRateScaling, 4 - g_timeAccelMode, 15);
    g_bulletTrackCount = clampRange(g_frameRateScaling << 1, 3, 16);
    g_threatTimerInit = 250 * g_frameRateScaling;
    g_threatDisplayTtl = 200 * g_frameRateScaling;
}

/* ==== seg000:0xdfb4 ==== */
extern char colorLut[];               /* byte-ramp LUT @0x9E8 */
extern int16 g_lodDistBase;           /* word_2F870 */
extern int16 g_lodDistScale;          /* word_2F872 */
extern int16 g_lodDistNear;           /* word_2F874 */
extern int16 g_lodDistFar;            /* word_2F876 */
extern int16 g_detailLevel;           /* word_354BC */

void setupLodDistances(void) {
    int16 lod;
    for (lod = 0; lod < 6; lod++) {
        ((int16 *)(colorLut + 0x10))[lod] = 0x20 << ((char)lod + (char)g_detailLevel);
    }
    g_lodDistNear = g_lodDistScale + g_lodDistBase;
    g_lodDistFar = clampRange(g_lodDistScale << 1, 0x1000, 9999);
    *(int16 *)(colorLut + 0x20) = g_detailLevel * 0xD05 + 0xD05;
}

/* ==== seg000:0xe010 ==== */
void exitTimeAccel(void) {
    if (g_timeAccelMode == 2) {
        g_timeAccelMode = 1;
        g_frameRateScaling <<= 1;
        recalcTimeScale();
    }
}
