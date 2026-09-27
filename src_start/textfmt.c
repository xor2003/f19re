/* START.EXE — number formatting (f15se2 shared/textfmt.c lineage) */
#include "inttype.h"

void my_ltoa(int32 value, char *buf) {
    int8 i, k;
    int8 *p;
    int8 n[6];
    p = buf;
    if (value < 0) {
        value = -value;
        *p = '-';
        p++;
    }
    n[0] = value % 0xa;
    value /= 0xa;
    n[1] = value % 0xa;
    value /= 0xa;
    n[2] = value % 0xa;
    value /= 0xa;
    n[3] = value % 0xa;
    value /= 0xa;
    n[4] = value % 0xa;
    value /= 0xa;
    n[5] = value % 0xa;
    i = 0;
    for (k = 5; k > 0; k--) {
        if (n[k] != 0) break;
    }
    do {
        if (k == 2 && i == 1) {
            *p = ',';
            p++;
        }
        *p = n[k] + '0';
        i = 1;
        p++;
    } while (--k >= 0);
    *p = '\0';
}

void my_itoa(int16 value, char *buf) {
    int8 i, k;
    char *p;
    int8 num[6];
    p = buf;
    if (value < 0) {
        value = -value;
        *p = 0x2d;
        p++;
    }
    num[0] = value % 0xa;
    value /= 0xa;
    num[1] = value % 0xa;
    value /= 0xa;
    num[2] = value % 0xa;
    value /= 0xa;
    num[3] = value % 0xa;
    value /= 0xa;
    num[4] = value % 0xa;
    value /= 0xa;
    num[5] = value % 0xa;
    i = 0;
    for (k = 5; k > 0 && num[k] == 0; k--);
    do {
        if (k == 2 && i == 1) {
            *p = 0x2c;
            p++;
        }
        *p = num[k] + 0x30;
        i = 1;
        p++;
    } while (--k >= 0);
    *p = 0;
}
