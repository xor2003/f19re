/* ==========================================================================
 * SU.EXE (English) — field/text-layout utility cluster.
 * Routines recovered from the fused seg000:0971-0e9d extent (originally one
 * ada proc label, now split into the nine per-function map extents).
 *
 * The cluster operates on a pen-position record: three parallel word arrays
 * indexed by a record offset — word_12FC8 = pen.x, word_12FCA = pen.y,
 * word_12FCC = font/attr — plus far calls into the resident output engine
 * (seg 0x2FC): suFieldPrint renders a text field, suCharWidth returns a
 * glyph's pixel width.
 * ==========================================================================
 */
#include "inttype.h"
#include "pointers.h"

/* ---- resident far output engine (seg 0x2FC slots, skeleton-reproduced) ---- */
void  far suFieldPrint(int16 field, char *text);    /* 2FC:0A3F */
int16 far suCharWidth(int16 ch, int16 font);        /* 2FC:0B11 */

/* ---- near helpers (unported — asm skeleton / stubs) ---- */
void  sub_11668(char *buf, int16 a, int16 b);       /* format a pair into buf */
void  sub_113B2(void);                              /* counter/timer start    */
void  sub_113F0(void);                              /* counter/timer stop     */
int16 sub_115D0(void);
void  sub_11C3C(int16 v);
int16 sub_11C4E(void);
void  pascal sub_11D56(int32 d, int32 *v);         /* *v /= d  (in-place)    */
void  sub_11732(char *dst, char far *src, int16 n);       /* far->near copy */
void  sub_11714(char *dst, char *src, int16 n);     /* near copy              */

/* ---- pen-position record arrays (dseg) ---- */
extern int16 word_12FC8[];                          /* pen.x  per record      */
extern int16 word_12FCA[];                          /* pen.y  per record      */
extern int16 word_12FCC[];                          /* font   per record      */
extern uint8 byte_138C0;                            /* tick counter (ISR-bumped) */

/* --------------------------------------------------------------------------
 * sub_10971 — format b,c into a scratch buffer, print as field a.
 *   0x971-0x999
 * ------------------------------------------------------------------------ */
void sub_10971(int16 a, int16 b, int16 c) {
    char buf[200];
    sub_11668(buf, b, c);
    suFieldPrint(a, buf);
}

/* --------------------------------------------------------------------------
 * sub_1099A — word-wrap a FAR string into lim-wide lines, printing each via
 *   suFieldPrint.  Far-pointer twin of sub_10AEB; after locating the break it
 *   also skips leading spaces on the line start before emitting.
 *   0x99A-0xAEA
 * ------------------------------------------------------------------------ */
void sub_1099A(int16 idx, char far *str, uint16 lim, int16 penx, int16 peny, int16 dy) {
    int16 pa;               /* fnt (font attr)        */
    char far *wgt;          /* cur (line-start ptr)   */
    char far *cp;           /* scan cursor            */
    int16 cnt;              /* n (word length)        */
    int16 dum;              /* spare / dead slot      */
    char far *st;           /* strStart               */
    int8  ag;               /* more flag              */
    int16 tw;               /* width accumulator      */
    char  buf[0x1F0];       /* wrapped line buffer    */
    char  cch;              /* current char           */
    st = str;
    wgt = str;
    cp = str;
    pa = *(int16 *)((char *)word_12FCC + idx);
    *(int16 *)((char *)word_12FCA + idx) = peny;
    ag = 1;
wrapline:
    tw = cnt = 0;
    while (tw < lim) {
        cch = *cp;
        if (cch == 0 || cch == 0x0D || cch == 0x0A) goto scanned;
        tw += suCharWidth(*cp++, pa);
        cnt++;
    }
scanned:
    if (tw >= lim) goto backdec;
    goto chkspace;
    do {
chkbrk:
        if (cch == 0 || cch == 0x0D || cch == 0x0A || cch == 0x2D) goto worddone;
        if (cp <= st) goto worddone;
backdec:
        cp--;
        cnt--;
chkspace:
        cch = *cp;
    } while (cch != 0x20);
worddone:
    if (*cp == 0x2D) cnt++;
    while (*wgt == 0x20) wgt++;
    if (*cp == 0) ag = 0;
    if (cnt != 0) {
        sub_11732(buf, wgt, cnt);
        buf[cnt] = 0;
        *(int16 *)((char *)word_12FC8 + idx) = penx;
        suFieldPrint(idx, buf);
        *(int16 *)((char *)word_12FCA + idx) += dy;
        if (*cp == 0x0D) *(int16 *)((char *)word_12FCA + idx) += 2;
    }
    cp++;
    wgt = cp;
    if (ag) goto wrapline;
}

/* --------------------------------------------------------------------------
 * sub_10C0E — pixel width of a near string in field idx's font.
 *   0xC0E-0xC51
 * ------------------------------------------------------------------------ */
int16 sub_10C0E(int16 idx, uint8 *str) {
    int16 w, s;
    uint8 *p;
    p = str;
    w = *(int16 *)((char *)word_12FCC + idx);
    s = 0;
    while (*p) {
        s += suCharWidth(*p++, w);
    }
    return s;
}

/* --------------------------------------------------------------------------
 * sub_10AEB — word-wrap a NEAR string into lim-wide lines, printing each via
 *   suFieldPrint and advancing pen.y.  Scans a word's pixel width, backtracks
 *   to a space/hyphen/EOL break when it overflows, emits, repeats.
 *   0xAEB-0xC0D
 * ------------------------------------------------------------------------ */
void sub_10AEB(int16 idx, uint8 *str, uint16 lim, int16 penx, int16 peny, int16 dy) {
    int16 pa;           /* fnt (font attr)        */
    uint8 *wgt;         /* cur (line-start ptr)   */
    uint8 *cp;          /* scan cursor            */
    int16 cnt;          /* n (word length)        */
    int16 dum;          /* spare / dead slot      */
    uint8 *st;          /* strStart               */
    int8  ag;           /* more flag              */
    int16 tw;           /* width accumulator      */
    char  buf[0x3E8];   /* wrapped line buffer    */
    uint8 cch;          /* current char           */
    st = str;
    wgt = str;
    cp = str;
    pa = *(int16 *)((char *)word_12FCC + idx);
    *(int16 *)((char *)word_12FCA + idx) = peny;
    ag = 1;
wrapline:
    tw = cnt = 0;
    while (tw < lim) {
        cch = *cp;
        if (cch == 0 || cch == 0x0D || cch == 0x0A) goto scanned;
        tw += suCharWidth(*cp++, pa);
        cnt++;
    }
scanned:
    if (tw >= lim) goto backdec;
    goto chkspace;
    do {
chkbrk:
        if (cch == 0 || cch == 0x0D || cch == 0x0A || cch == 0x2D) goto worddone;
        if (cp <= st) goto worddone;
backdec:
        cp--;
        cnt--;
chkspace:
        cch = *cp;
    } while (cch != 0x20);
worddone:
    if (*cp == 0x2D) cnt++;
    if (*cp == 0) ag = 0;
    if (cnt != 0) {
        sub_11714(buf, (char *)wgt, cnt);
        buf[cnt] = 0;
        *(int16 *)((char *)word_12FC8 + idx) = penx;
        suFieldPrint(idx, buf);
        *(int16 *)((char *)word_12FCA + idx) += dy;
        if (*cp == 0x0D) *(int16 *)((char *)word_12FCA + idx) += 2;
    }
    cp++;
    wgt = cp;
    if (ag) goto wrapline;
}

/* --------------------------------------------------------------------------
 * sub_10C52 — int32 → decimal into out, thousands-comma at digit group.
 *   dig[i] = val % 10 via aNlrem; val /= 10 via sub_11D56 in-place helper.
 *   0xC52-0xD88
 * ------------------------------------------------------------------------ */
void sub_10C52(int32 val, char *out) {
    char dig[6];
    char *cur;
    int8 comma;
    int8 n;
    cur = out;
    if (val < 0) { val = -val; *cur = '-'; cur++; }
    dig[0] = val % 10; sub_11D56(10L, &val);
    dig[1] = val % 10; sub_11D56(10L, &val);
    dig[2] = val % 10; sub_11D56(10L, &val);
    dig[3] = val % 10; sub_11D56(10L, &val);
    dig[4] = val % 10; sub_11D56(10L, &val);
    dig[5] = val % 10;
    comma = 0;
    n = 5;
    for (; n > 0 && dig[n] == 0; n--);
    do {
        if (n == 2 && comma == 1) { *cur = ','; cur++; }
        *cur = dig[n] + '0';
        comma = 1;
        cur++;
        n--;
    } while (n >= 0);
    *cur = 0;
}

/* --------------------------------------------------------------------------
 * sub_10D89 — int16 → decimal into out (same shape, 16-bit idiv form).
 *   0xD89-0xE5E
 * ------------------------------------------------------------------------ */
void sub_10D89(int16 val, char *out) {
    char dig[6];
    char *cur;
    int8 comma;
    int8 n;
    cur = out;
    if (val < 0) { val = -val; *cur = '-'; cur++; }
    dig[0] = val % 10; val /= 10;
    dig[1] = val % 10; val /= 10;
    dig[2] = val % 10; val /= 10;
    dig[3] = val % 10; val /= 10;
    dig[4] = val % 10; val /= 10;
    dig[5] = val % 10;
    comma = 0;
    n = 5;
    for (; n > 0 && dig[n] == 0; n--);
    do {
        if (n == 2 && comma == 1) { *cur = ','; cur++; }
        *cur = dig[n] + '0';
        comma = 1;
        cur++;
        n--;
    } while (n >= 0);
    *cur = 0;
}

/* --------------------------------------------------------------------------
 * sub_10E5F — busy-wait delay: reset tick counter, wait until it passes n.
 *   0xE5F-0xE78
 * ------------------------------------------------------------------------ */
void sub_10E5F(int16 n) {
    byte_138C0 = 0;
    sub_113B2();
    while (n >= byte_138C0) {
    }
    sub_113F0();
}

/* --------------------------------------------------------------------------
 * sub_10E79 — frameless: forward sub_115D0() result into sub_11C3C.
 *   0xE79-0xE83
 * ------------------------------------------------------------------------ */
void sub_10E79(void) {
    sub_11C3C(sub_115D0());
}

/* --------------------------------------------------------------------------
 * sub_10E84 — fixed-point scale: (a * sub_11C4E()) >> 15.
 *   a is zero-extended (hi pushed as 0); sub_11C4E() sign-extended via cwd.
 *   0xE84-0xE9D
 * ------------------------------------------------------------------------ */
int32 sub_10E84(int16 a) {
    return ((int32)(uint32)(uint16)a * sub_11C4E()) >> 15;
}
