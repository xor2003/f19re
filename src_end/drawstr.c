/* END.EXE — string drawing + text wrapping (f15se2 enmisc.c/entext.c lineage) */
#include "inttype.h"

extern void far gfx_drawString(int16 *pageNum, const char *string);
extern int16 far gfx_charWidth(int16 ch, int16 font);   /* slot — 9d9:0x143b */
extern void farStrcpy(char *dst, char far *src);        /* sub_138EC — asm */
extern void copyBytes(char *dst, char *src, int16 n);   /* sub_13998 — asm */
extern void memcpyFromFar(char *dst, char far *src, int16 n); /* sub_139B6 — asm */
void drawFarString(int16 *s, char far *str);

/* seg000:0x07a0 — f15 enmisc.c: set record pos then far-string draw */
void drawStringAtPos(int16 *s, char far *str, int16 x, int16 y) {
    s[4] = x;
    s[5] = y;
    drawFarString(s, str);
}

/* seg000:0x07c6 */
void drawStringAt(int16 *pageNum, const char *string, int16 x, int16 y) {
    pageNum[4] = x;
    pageNum[5] = y;
    gfx_drawString(pageNum, string);
}

/* seg000:0x07eb — f15 enmisc.c: stage far string into near buf then draw */
void drawFarString(int16 *s, char far *str) {
    char buf[200];
    farStrcpy(buf, str);
    gfx_drawString(s, buf);
}

/* seg000:0x0814 — far-string variant of drawWrappedText (buf[500]);
 * adds a while(*a==' ')a++ skip of leading spaces on each wrapped line */
void drawWrappedTextFar(int16 *page, char far *str, uint16 maxWidth, int16 x, int16 y, int16 lineHeight) {
    int16 p;            /* font */
    char far *a;        /* lineStart */
    char far *b;        /* scan */
    int16 c;            /* charCount */
    int32 d;            /* unused — 4-byte slot */
    char far *e;        /* strBegin */
    int8 f;             /* running */
    uint16 g;           /* pixWidth */
    char buf[500];

    e = str;
    a = str;
    b = str;
    p = page[6];
    page[5] = y;
    f = 1;
    for (;;) {
        g = c = 0;
        while (g < maxWidth && *b != 0 && *b != '\r' && *b != '\n') {
            g += gfx_charWidth(*b++, p);
            c++;
        }
        if (g >= maxWidth) {
            b--;
            c--;
        }
        while (*b != ' ' && *b != 0 && *b != '\r' && *b != '\n' && *b != '-' && b > e) {
            b--;
            c--;
        }
        if (*b == '-') {
            c++;
        }
        while (*a == ' ') {
            a++;
        }
        if (*b == 0) {
            f = 0;
        }
        if (c != 0) {
            memcpyFromFar(buf, a, c);
            buf[c] = 0;
            page[4] = x;
            gfx_drawString(page, buf);
            page[5] += lineHeight;
            if (*b == '\r') {
                page[5] += 2;
            }
        }
        b++;
        a = b;
        if (f == 0) {
            break;
        }
    }
}

/* seg000:0x0965 — f15 entext.c drawWrappedText (near-string, buf[1000]) */
void drawWrappedText(int16 *page, char *str, uint16 maxWidth, int16 x, int16 y, int16 lineHeight) {
    int16 p;            /* font */
    char *a;            /* lineStart */
    uint8 *b;           /* scan */
    int16 c;            /* charCount */
    int16 d;            /* unused */
    char *e;            /* strBegin */
    int8 f;             /* running */
    uint16 g;           /* pixWidth */
    char buf[1000];

    e = str;
    a = str;
    b = (uint8 *)str;
    p = page[6];
    page[5] = y;
    f = 1;
    for (;;) {
        g = c = 0;
        while (g < maxWidth && *b != 0 && *b != '\r' && *b != '\n') {
            g += gfx_charWidth(*b++, p);
            c++;
        }
        if (g >= maxWidth) {
            b--;
            c--;
        }
        while (*b != ' ' && *b != 0 && *b != '\r' && *b != '\n' && *b != '-' && b > e) {
            b--;
            c--;
        }
        if (*b == '-') {
            c++;
        }
        if (*b == 0) {
            f = 0;
        }
        if (c != 0) {
            copyBytes(buf, a, c);
            buf[c] = 0;
            page[4] = x;
            gfx_drawString(page, buf);
            page[5] += lineHeight;
            if (*b == '\r') {
                page[5] += 2;
            }
        }
        b++;
        a = (char *)b;
        if (f == 0) {
            break;
        }
    }
}

/* seg000:0x0a88 — sum of per-char widths; font from item word at +0x0c */
int16 stringWidth(int16 *item, uint8 *str) {
    int16 font;
    uint8 *cur;
    int16 n;
    cur = str;
    font = item[6];
    n = 0;
    while (*cur != 0)
        n += gfx_charWidth(*cur++, font);
    return n;
}
