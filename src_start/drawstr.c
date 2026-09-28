/* START.EXE — drawStringAt (same shape as src_end/drawstr.c; f15se2
 * shared/drawstr.c lineage) */
#include "inttype.h"

extern void far gfx_drawString(int16 *pageNum, const char *string);

void drawStringAt(int16 *pageNum, const char *string, int16 x, int16 y) {
    pageNum[4] = x;
    pageNum[5] = y;
    gfx_drawString(pageNum, string);
}

/* seg000:0x3b9b — copies a far string to a local buf then draws it */
extern void sub_15152(char *dst, const char far *src);   /* far-src strcpy */

void drawStringFar(int16 *pageNum, const char far *string) {
    char workbuf[0xC8];
    sub_15152(workbuf, string);
    gfx_drawString(pageNum, workbuf);
}

/* seg000:0x3b50 — drawStringAt variant for far strings */
void drawStringAtFar(int16 *pageNum, const char far *string, int16 x, int16 y) {
    pageNum[4] = x;
    pageNum[5] = y;
    drawStringFar(pageNum, string);
}
