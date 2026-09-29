/* END.EXE — file-section helpers (f15se2 enfile.c lineage) */
#include "inttype.h"

extern int16 dosReadFar(int16 fd, uint16 n, uint16 off, uint16 seg); /* sub_114A9 — int21/3Fh asm */
extern uint16 allocBuffer(int16 size);                               /* stalloc.c */
extern void memsetFar(uint8 far *dst, int16 val, uint16 count);      /* sub_13982 — rep stosb asm */

union FarWords { uint8 far *p; struct { uint16 off; uint16 seg; } w; };

/* seg000:0x137c — far-read shim (map name: strcoll) */
int16 strcoll(int16 fd, int16 count, int16 bufOff, int16 bufSeg) {
    return dosReadFar(fd, count, bufOff, bufSeg);
}

/* seg000:0x20fc — allocate a section buffer and clear it (map name: loadFileSection) */
int16 loadFileSection(int16 size) {
    union FarWords dst;
    int16 result;
    dst.w.seg = result = allocBuffer(size);
    dst.w.off = 0;
    memsetFar(dst.p, 0, size);
    return result;
}
