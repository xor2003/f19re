/* eg3dload.c — 3D3 model file loader (F19) */
#include "inttype.h"
#include "pointers.h"
#include <dos.h>
#include <stdio.h>
#include <string.h>

void strcpyFromDot(char *dst, const char *src); /* sub_11068 */
void printError(char *msg);                      /* sub_1104C */

extern char regnStr[];              /* "STFLT.xxx" @dseg:5C56 */
extern int16 sign3d3;               /* @dseg:6376 */
extern size_t size3d3;              /* word_35228 @dseg:63B8 */
extern uint16 buf3d3[];             /* @dseg:6378 */
extern uint8 flt15_buf2[];          /* @dseg:238A staging */
extern char FAR g_world3dData[];    /* seg004:A430 */
extern FILE *fileHandle;            /* word_354C8 */

/* ==== seg000:0xcb8c ==== */
void load15Flt3d3(void) {
    char FAR *dst;
    struct SREGS sregs;
    int16 size3d3_2;
    int16 chunk;
    strcpyFromDot(regnStr, ".3D3");
    fileHandle = fopen(regnStr, "rb");
    if (fileHandle == NULL) {
        printError("Open Error on *.3D3");
        return;
    }
    fread(&sign3d3, 2, 1, fileHandle);
    fread(&size3d3, 2, 1, fileHandle);
    fread(buf3d3, 2, size3d3, fileHandle);
    fread(&size3d3_2, 2, 1, fileHandle);
    segread(&sregs);
    for (dst = g_world3dData; size3d3_2 > 0; size3d3_2 -= 0x800, dst += 0x800) {
        chunk = (size3d3_2 > 0x800) ? 0x800 : size3d3_2;
        fread(flt15_buf2, 1, chunk, fileHandle);
        movedata(sregs.ds, PTR_OFF(flt15_buf2), FP_SEG(dst), FP_OFF(dst), chunk);
    }
    fclose(fileHandle);
}
