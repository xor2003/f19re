/* START.EXE — resource-file shims + section loader (egame enfile.c lineage).
 * Module was built WITHOUT /Gs (chkstk prologue visible on sub_14ACB). */
#include "inttype.h"

/* skeleton callees (asm file layer + driver loader) */
extern int16 openFile(const char *path, int16 mode);      /* seg000:0x482e */
extern int16 createFile(const char *path, int16 attr);    /* seg000:0x4885 */
extern int16 closeFile(int16 handle);                     /* seg000:0x48dc */
extern int16 sub_14929(int16 a, int16 b, int16 c, int16 d);          /* dos read raw */
extern int16 sub_149B9(int16 a, int16 b, int16 c, int16 d, int16 e); /* dos write raw */

/* seg000:0x47b6 */
int16 resFileOpen(const char *path, int16 mode) {
    return openFile(path, mode);
}

/* seg000:0x47c8 */
int16 resFileCreate(const char *path, int16 attr) {
    return createFile(path, attr);
}

/* seg000:0x47da */
int16 resFileClose(int16 handle) {
    return closeFile(handle);
}

/* seg000:0x47fc (map name "strcoll" — FLIRT mislabel; 4-arg read forwarder) */
int16 resFileReadFar(int16 h, int16 b, int16 c, int16 d) {
    return sub_14929(h, b, c, d);
}

/* seg000:0x4814 */
int16 resFileWrite(int16 a, int16 b, int16 c, int16 d, int16 e) {
    return sub_149B9(a, b, c, d, e);
}

/* seg000:0x4746 — open(path,0) → resFileReadFar(h,-1,b,c) → close; returns r */
int16 resFileReadBlock(const char *path, int16 b, int16 c) {
    int16 h, r;
    h = resFileOpen(path, 0);
    r = resFileReadFar(h, -1, b, c);
    resFileClose(h);
    return r;
}

/* seg000:0x477e — create(path,0) → resFileWrite(h,e,b,c,d) → close */
int16 resFileWriteBlock(const char *path, int16 b, int16 c, int16 d, int16 e) {
    int16 h, r;
    h = resFileCreate(path, 0);
    r = resFileWrite(h, e, b, c, d);
    resFileClose(h);
    return r;
}
