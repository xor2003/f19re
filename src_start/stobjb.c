/* START.EXE — view-origin setter at seg000:0x61a4, contiguous tail of the
 * sub_15B68 extent (5b68-61cb). Stores the new origin then reloads the
 * object file through sub_15460. This module compiles /Os — the original
 * emits the shared-epilogue tail (jmp over sub ax,ax) that /Ot inlines. */
#include "inttype.h"

extern int16 sub_15460(const char *name, int16 pad);   /* seg000:0x5460 */
extern int16 word_2CA4E, word_2CA50;                   /* draw offsets */

/* seg000:0x61a4 — the second arg word is pushed but unread by the callee. */
int16 sub_161A4(int16 y, int16 x, char *name, int16 pad)
{
    word_2CA4E = x;
    word_2CA50 = y;
    if (sub_15460(name, pad) != 0)
        return 1;
    return 0;
}
