/* START.EXE — exit cleanup (f15se2 shared/cleanup.c lineage) */
#include "inttype.h"

extern void restoreTimerIrqHandler(void);
extern void intDispatch(int16 intNum, uint8 *inRegs, uint8 *outRegs);
extern void far misc_clearKeyFlags(void);

uint8 timerHandlerInstalled;   /* byte dseg:0x16af */

void cleanup(void) {
    uint8 regs[0xe];
    if (timerHandlerInstalled == 1) {
        restoreTimerIrqHandler();
    }
    regs[1] = 0; /* func 0 */
    regs[0] = 3; /* mode 3 (80x25) */
    intDispatch(0x10, regs, regs);
    misc_clearKeyFlags();
}
