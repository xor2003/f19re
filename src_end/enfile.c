/* END.EXE — file-service wrappers + section helpers (f15se2 egfileio/enfile lineage) */
#include "inttype.h"

/* int21 primitives — asm in the skeleton */
extern int16 openFile(const char *name, int16 mode);               /* 0x13ae */
extern int16 createFile(const char *name, int16 attr);             /* 0x1405 */
extern void  fileClose(int16 fd);                                  /* 0x145c */
extern int16 readFile1(int16 fd, int16 count, int16 off);          /* 0x147e */
extern int16 readFile2(int16 fd, int16 count, int16 off, int16 seg);/* 0x14a9 */
extern int16 writeFileAtRaw(int16 fd, int16 c, int16 o, int16 s, int16 a); /* 0x1539 */
extern void  picStreamRead(int16 fd);                              /* 0x1521 — asm */
extern uint16 allocBuffer(int16 size);                             /* stalloc.c */
extern void memsetFar(uint8 far *dst, int16 val, uint16 count);    /* sub_13982 — rep stosb asm */

extern int16 picBufPos;      /* dseg — stream cursor shared with the asm refill */
extern uint8 picStreamBuf[]; /* dseg:0x194a — 0x200-byte block buffer */

int16 openFileWrapper(const char *name, int16 mode);
int16 createFileWrapper(const char *name, int16 attr);
void  closeFileWrapper(int16 fd);
int16 readFile1Wrapper(int16 fd, int16 count, int16 off);
int16 readFile2Wrapper(int16 fd, int16 count, int16 off, int16 seg);
int16 writeFileAtRawWrapper(int16 fd, int16 count, int16 off, int16 seg, int16 addend);

union FarWords { uint8 far *p; struct { uint16 off; uint16 seg; } w; };

/* seg000:0x12c6 — open + readFile2(-1) + close */
int16 loadFileSection(const char *name, int16 b, int16 c) {
    int16 handle, result;
    handle = openFileWrapper(name, 0);
    result = readFile2Wrapper(handle, -1, b, c);
    closeFileWrapper(handle);
    return result;
}

/* seg000:0x12fe — create + writeFileAtRaw + close */
int16 writeFileSection(const char *name, int16 b, int16 c, int16 d, int16 e) {
    int16 handle, result;
    handle = createFileWrapper(name, 0);
    result = writeFileAtRawWrapper(handle, e, b, c, d);
    closeFileWrapper(handle);
    return result;
}

/* seg000:0x1336 — f15 egfileio.c */
int16 openFileWrapper(const char *name, int16 mode) {
    return openFile(name, mode);
}

/* seg000:0x1348 */
int16 createFileWrapper(const char *name, int16 attr) {
    return createFile(name, attr);
}

/* seg000:0x135a */
void closeFileWrapper(int16 fd) {
    fileClose(fd);
}

/* seg000:0x1368 */
int16 readFile1Wrapper(int16 fd, int16 count, int16 off) {
    return readFile1(fd, count, off);
}

/* seg000:0x137c — far-read shim (was map-named strcoll) */
int16 readFile2Wrapper(int16 fd, int16 count, int16 off, int16 seg) {
    return readFile2(fd, count, off, seg);
}

/* seg000:0x1394 */
int16 writeFileAtRawWrapper(int16 fd, int16 count, int16 off, int16 seg, int16 addend) {
    return writeFileAtRaw(fd, count, off, seg, addend);
}

/* seg000:0x1a3e — buffered byte reader over the 0x200 block refill */
int16 readPicStream(uint8 *dst, int16 count, int16 fd) {
    int16 i;
    for (i = 0; i < count; i++) {
        if (picBufPos > 0x1ff) {
            picStreamRead(fd);
            picBufPos = 0;
        }
        *dst++ = picStreamBuf[picBufPos++];
    }
    return i;
}

/* seg000:0x20fc — allocate a section buffer and clear it */
int16 allocClearBuf(int16 size) {
    union FarWords dst;
    int16 result;
    dst.w.seg = result = allocBuffer(size);
    dst.w.off = 0;
    memsetFar(dst.p, 0, size);
    return result;
}
