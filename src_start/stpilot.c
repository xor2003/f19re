/* START.EXE — roster/hall-of-fame load (f15se2 stpilot.c lineage; START opens
 * "Roster.Fil" where F15 opens "HallFame") */
#include "inttype.h"

extern int16 openFile(const char *name, const char *mode);
extern int16 readFile1(void *buf, int16 itemsz, int16 count, int16 handle);
extern int16 closeFile(int16 handle);

typedef struct {
    char data[0x50];        /* hallfame record, 0x50 bytes */
} HallfameEntry;

int16 hallfameCount;        /* ds:0x9b52 */
HallfameEntry hallfameBuf[10]; /* ds:0x9d54 */

void loadHallfame(void) {
    int16 handle, slotIdx;
    handle = openFile("Roster.Fil", "rb");
    readFile1(&hallfameCount, 2, 1, handle);
    for (slotIdx = 0; slotIdx < 0xa; slotIdx++) {
        readFile1(hallfameBuf + slotIdx, 0x50, 1, handle);
    }
    closeFile(handle);
}
