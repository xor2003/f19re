/* ASOUND.EXE ports -- the F-117-generation "STEALTH.EXE 11-16-90" AdLib
 * driver (9-slot ABI, first_slot=0x64).  routine_NN names track
 * map/asound.map; globals are the driver's seg000 data items. */
#include "inttype.h"

/* seg001:0198 asm register-convention helper (ax=a, bl=m out) --
 * stubbed here so test links resolve; the real routine stays asm. */
int routine_37(int a, char m) { (void)m; return a; }

/* seg000 globals */
int word_1128C;
int word_1128E;
uint8 byte_10008[16];                   /* seg000:0008 table */


int routine_36(char a)                  /* seg001:063e */
{
    char b;
    int c;

    b = a;
    if (a > 6) {
        b = 7;
        if (a != 7 && a != 10)
            b = 8;
    }
    c = b;
    return c;
}


int routine_48(void)                    /* seg001:0d32 */
{
    int t;

    t = word_1128C + 0xe0;
    return routine_37(t, byte_10008[word_1128E] & 3);
}
