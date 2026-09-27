/* START.EXE — quadtree terrain-grid lookup (linker-tree stterr.c lineage;
 * verified vs EN binary).  /Ot module (inlined case-epilogues). */
#include "inttype.h"

extern uint8 gridBuf1[0x10];                       /* dseg:0xb374 */
extern uint8 gridBuf2[0x100];                      /* dseg:0xa4c8 */
extern uint8 gridBuf3[0x200];                      /* dseg:0xa2c6 */
extern uint8 gridBuf4[0x200];                      /* dseg:0x9b54 */
extern uint8 gridBuf5[0x200];                      /* dseg:0x994e */
extern int16 gridLevelSize[];                        /* dseg:0x3bb4 */

/* seg000:0x70e0 — recursive quadtree descent through the 5 grid levels */
int16 lookupGridCell(int16 level, int16 col, int16 row) {
    if (col < 0 || row < 0 || col >= gridLevelSize[level] ||
        row >= gridLevelSize[level])
        return -1;
    switch (level) {
    case 4:
        return gridBuf1[col + (row << 2)];
    case 3:
        return gridBuf2[col + (row << 4)];
    case 2:
        return gridBuf3[(col & 3) + (((row & 3) << 2) +
            (lookupGridCell(3, col >> 2, row >> 2) << 4))];
    case 1:
        return gridBuf4[(col & 3) + (((row & 3) << 2) +
            (lookupGridCell(2, col >> 2, row >> 2) << 4))];
    case 0:
        return gridBuf5[(col & 3) + (((row & 3) << 2) +
            (lookupGridCell(1, col >> 2, row >> 2) << 4))];
    }
}
