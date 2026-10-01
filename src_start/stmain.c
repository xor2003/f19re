/* START.EXE — entry-adjacent helpers (seg000:0x0810-0x08c7). These live
 * between the main driver sub_10010 and the graphics-init cluster: the
 * quit-path tail (sub_10810) and the overlay settle-poll (sub_108B7). */
#include "inttype.h"

extern uint8 byte_212BA;                     /* abort/quit flag */
extern void sub_10882(void);                 /* cleanup */
extern void sub_146E3(void);                 /* restore int vector */
extern void sub_1DCAC(int16);                /* exit(code) */
extern int16 far ovlCall_cbc(void);          /* overlay 1000:0cbc poll */
extern void  far ovlCall_cc1(void);          /* overlay 1000:0cc1 step */

/* seg000:0x0810 — if the abort flag is up, restore state and quit. */
void sub_10810(void) {
    if (byte_212BA != 0) {
        sub_10882();
        sub_146E3();
        sub_1DCAC(0);
    }
}

/* seg000:0x08b7 — pump the overlay until its poll reports settled. */
void sub_108B7(void) {
    while (ovlCall_cbc() == 0)
        ovlCall_cc1();
}
