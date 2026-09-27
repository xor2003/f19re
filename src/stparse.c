/* seg000 — string/path helpers (ported, verified) */
#include <string.h>
#include "inttype.h"

#ifndef EXE_START /* src_start/stgrid.c provides START's variant */
void replaceExtension(char *path, const char *source) {
    char ch;
    for (; (ch = *path) != '.';) {
        if (ch == 0) break;
        path++;
    }
    strcpy(path, source);
}
#endif
