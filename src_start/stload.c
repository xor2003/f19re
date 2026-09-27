/* START.EXE — resource section loader (seg000:0x4acb).
 * Its own module: prologue is `mov ax,2; call __chkstk` = built WITHOUT /Gs,
 * while the sibling res-file wrappers (stfile.c) probe nothing.
 * The pushed-but-unread di/si are phantom register params (same trick as
 * replaceExtension): their homes live in caller arg space at [bp+0xc/0xe]. */
#include "inttype.h"

extern int16 resFileOpen(const char *path, int16 mode);     /* seg000:0x47b6 */
extern int16 resFileClose(int16 handle);                    /* seg000:0x47da */
extern int16 sub_1E172(int16 h, int16 b, int16 c, int16 mode);   /* lseek */
extern void  sub_14B14(int16 fd, int16 a, int16 b);         /* sprite loader A */
extern void  sub_14B86(int16 fd, int16 sel);                /* pic loader */
extern void  sub_14C62(int16 fd, int16 sel);                /* sprite loader */

/* seg000:0x4a26 — arming.spr 3-arg variant (dup name: loadSpriteScaled) */
void loadSpriteScaled(const char *name, int16 a, int16 b) {
    int16 fd;
    fd = resFileOpen(name, 0);
    sub_14B14(fd, a, b);
    resFileClose(fd);
}

/* seg000:0x4a5f — Clip.pic */
void loadPicRes(const char *name, int16 sel) {
    int16 fd;
    fd = resFileOpen(name, 0);
    sub_14B86(fd, sel);
    resFileClose(fd);
}

/* seg000:0x4a95 — arming.spr / Maps.spr */
void loadSpriteRes(const char *name, int16 sel) {
    int16 fd;
    fd = resFileOpen(name, 0);
    sub_14C62(fd, sel);
    resFileClose(fd);
}

void loadResSection(const char *name, int16 sel, int16 offLo, int16 offHi) {
    int16 fd;
    fd = resFileOpen(name, 0);
    sub_1E172(fd, offLo, offHi, 0);
    sub_14C62(fd, sel);
    resFileClose(fd);
}
