/* START.EXE — buffer alloc helper (f15se2 stalloc.c lineage; END has the same
 * routine plus freeBuffer, which START lacks) */
#include "inttype.h"

extern void cleanup(void);
extern void dos_printstring(const char *s);
extern void exit(int16 code);
extern uint16 dos_alloc(uint16 size);    /* asm helper (bytes->paras, int 21/48) */

uint16 allocBuffer(int16 size) {
    uint16 segment;
    if ((segment = dos_alloc(size)) < 0x10) {
        cleanup();
        dos_printstring("Insufficient system memory - AllocBuffer$");
        exit(0);
    }
    return segment;
}

extern int16 dos_free(uint16 segment);   /* asm helper (int 21/49) */

void freeBuffer(uint16 segment) {
    if (dos_free(segment) != 0) {
        cleanup();
        dos_printstring("...dealloc error...$");
        exit(0);
    }
}
