/* START.EXE — grid-file parsing (linker-tree stgrid.c lineage;
 * verified vs EN binary). */
#include "inttype.h"
#include <stdio.h>

extern void mystrcpy(char *d, const char *s);      /* seg000:0x5120 */
extern int16 showMsgWaitKey(const char *m);        /* seg000:0x7518 */
extern void sub_151D4(void *d, int8 v, int16 n);   /* seg000:0x51d4 memset */

void replaceExtension();  /* fwd: 3rd param is a codegen device (see def) */

extern FILE *fileHandle;                           /* dseg:0x98f4 */
extern char *regnPlhPtr;                           /* dseg:0x4dac */
extern uint16 gridSignature;                       /* dseg:0x3d0a */
extern int16 gridValidFlag;                        /* dseg:0x3d10 */
extern uint8 gridBuf1[0x10];                       /* dseg:0xb374 */
extern uint8 gridBuf2[0x100];                      /* dseg:0xa4c8 */
extern uint8 gridBuf3[0x200];                      /* dseg:0xa2c6 */
extern uint8 gridBuf4[0x200];                      /* dseg:0x9b54 */
extern uint8 gridBuf5[0x200];                      /* dseg:0x994e */

void parseGrid(void) {                        /* seg000:0x73df */
    int16 i, n;    /* i gets the post-loop j store; n unused in the frame */
    register int16 j;
    replaceExtension(regnPlhPtr, ".3DG");
    if ((fileHandle = fopen(regnPlhPtr, "rb")) == 0) {
        showMsgWaitKey("Open error on grid file");
        j = 0;
        do {
            gridBuf1[j] = j;
            j++;
        } while (j < 0x10);
        i = j;   /* emits the observed mov [bp-2],si register flush */
        sub_151D4(gridBuf2, 0, 0x100);
        sub_151D4(gridBuf3, 0, 0x200);
        sub_151D4(gridBuf4, 0, 0x200);
        sub_151D4(gridBuf5, 0, 0x200);
        gridValidFlag = 0;
        return;
    }
    fread(&gridSignature, 2, 1, fileHandle);
    if (gridSignature != 0x3232) {
        showMsgWaitKey("Bad grid file for region");
    }
    else {
        fread(gridBuf1, 1, 0x10, fileHandle);
        fread(gridBuf2, 1, 0x100, fileHandle);
        fread(gridBuf3, 1, 0x200, fileHandle);
        fread(gridBuf4, 1, 0x200, fileHandle);
        fread(gridBuf5, 1, 0x200, fileHandle);
    }
    fclose(fileHandle);
}

/* seg000:0x7534 - `s` is a phantom register param (callers pass 2 args;
 * its home [bp+8] is never touched).  It binds si without a local slot,
 * and `path = s` emits the observed `mov [bp+4],si` writeback. */
void replaceExtension(char *path, char *ext, register char *s) {
    for (s = path; *s != '.';) {
        if (*s == 0) break;
        s++;
    }
    path = s;
    mystrcpy(path, ext);
}

