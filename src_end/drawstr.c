/* END.EXE — drawStringAt (f15se2 shared/drawstr.c lineage) */
#include "inttype.h"

extern void far gfx_drawString(int16 *pageNum, const char *string);

void drawStringAt(int16 *pageNum, const char *string, int16 x, int16 y) {
    pageNum[4] = x;
    pageNum[5] = y;
    gfx_drawString(pageNum, string);
}
