/* START.EXE — tactical-map line wrapper (same role as EGAME's drawMapLine) */
#include "inttype.h"

extern int16 mapToScreenX(int16 v);
extern int16 mapToScreenY(int16 v);
extern void drawClippedLineEx(int16 x1, int16 y1, int16 x2, int16 y2,
                              int16 cx1, int16 cy1, int16 cx2, int16 cy2, int16 flag);

int16 mapClipX1, mapClipY1, mapClipX2, mapClipY2;  /* ds:0x653c/0x6540/0x653e/0x6542 */

void drawMapLine(int16 x1, int16 y1, int16 x2, int16 y2) {
    drawClippedLineEx(mapToScreenX(x1), mapToScreenY(y1), mapToScreenX(x2), mapToScreenY(y2),
                      mapClipX1, mapClipY1, mapClipX2, mapClipY2, 1);
}
