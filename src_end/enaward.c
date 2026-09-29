/* END.EXE — file-open/seek helper (f15se2 enaward.c lineage) */
#include "inttype.h"

extern int16 openFileWrapper(const char *name, int16 mode);  /* sub_11336 */
extern void seekFileAt(int16 fd, int16 off, int16 whence);   /* sub_11694 */
extern void closeFileWrapper(int16 fd);                      /* sub_1135A */

/* seg000:0x15a6 — open, seek, close (map name: loadPicFromFileAt) */
void loadPicFromFileAt(const char *name, int16 off, int16 whence) {
    int16 handle;
    handle = openFileWrapper(name, 0);
    seekFileAt(handle, off, whence);
    closeFileWrapper(handle);
}
