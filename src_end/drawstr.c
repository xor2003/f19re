/* END.EXE — drawStringAt (f15se2 shared/drawstr.c lineage) */
#include "inttype.h"

extern void far gfx_drawString(int16 *pageNum, const char *string);

void drawStringAt(int16 *pageNum, const char *string, int16 x, int16 y) {
    pageNum[4] = x;
    pageNum[5] = y;
    gfx_drawString(pageNum, string);
}

extern int16 far gfx_charWidth(int16 ch, int16 font);   /* slot — 9d9:0x143b */

/* seg000:0x0a88 — sum of per-char widths; font from item word at +0x0c */
int16 stringWidth(int16 *item, uint8 *str) {
    int16 font;
    uint8 *cur;
    int16 n;
    cur = str;
    font = item[6];
    n = 0;
    while (*cur != 0)
        n += gfx_charWidth(*cur++, font);
    return n;
}
