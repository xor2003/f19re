/* src/stubs.c overflowed CL's heap under -DEXE_START; the stmenu.c globals
 * added for sub_1A68C live here instead (satellite builds compile every
 * src_start/*.c module, so these defs link in like any other). */
#include "inttype.h"

int16 word_2D2C6;                            /* sub_161CC arg */
int16 word_22324;                            /* sub_161CC side flag */
int16 word_2C972, word_2C974;                /* sub_14746 args */
uint8 byte_298E9, byte_2B384;                /* res-loaded flags */
int16 word_2D064, word_2D270;                /* sub_14BEE args */
int16 *word_25B2C, *word_25B44, *word_25CF6; /* page record handles */
int16 word_25CE6;                            /* widget param */
uint8 far *word_209AA;                       /* far ptr→item count byte */
int16 word_25B46[10 * 16];                   /* dseg:0x5b46, stride 0x20 */
int16 word_25B64, word_25B84;                /* theater 0 b4f args */
int16 word_25BA4, word_25BC4;                /* theater 1 */
int16 word_25BE4, word_25C04;                /* theater 2 */
int16 word_25C24, word_25C44;                /* theater 3 */
int16 word_25C64, word_25C84;                /* theater 4 */
void  sub_14BEE(int16 a, int16 b) { }
void  sub_161CC(int16 a, int16 b, int16 c) { }
void  far ovlCall_c4e(int16 v) { }           /* overlay 1000:0c4e */
void  far ovlCall_bea(int16 v) { }           /* overlay 1000:0bea */
/* stmenu.c sub_18F12 (briefing/objectives screen) globals + slots */
int16 word_2542C[0x14];                      /* per-mission values (long arg) */
char  *word_25454[0x14];                     /* row string-id table */
int16 word_2547C[2], word_25480[2];          /* packed 4/4 nibble pairs */
int16 *word_25014, *word_24FFC, *word_2542A; /* page record handles/selp */
int16 word_25016, word_25034, word_25064;    /* sel-init / b4f arg / widget */
uint8 byte_29B50, byte_2D06A;                /* res-loaded flags */
void  sub_14ACB(char *s, int16 v, long x) { }
void  sub_1513B(char far *d, char *s) { }
void  far ovlCall_bc7(int16 *pg, int16 a, int16 b, int16 c, int16 d,
                      int16 e, int16 f) { }  /* overlay 1000:0bc7 */
/* stmenu.c sub_193EE (mission roster screen) globals */
struct MsnRec {                             /* dseg:0x9d54, stride 0x50 */
    int16 f0;                               /* record id */
    char  name[0x1E];                       /* mission name */
    int16 f20, f22, f24, f26, f28, f2a, f2c, f2e, f30;
    int16 f32, f34;                         /* coord pair (long arg) */
    int16 f36, f38, f3a, f3c, f3e, f40;
    int16 f42, f44, f46, f48, f4a, f4c, f4e;
};
struct MsnRec word_29D54[0x14];             /* roster records */
int16 word_25722[0x14][0x19];               /* per-sel rect/state block */
int16 word_256E2, word_256FA;               /* page record indices */
int16 word_256FC, word_2591A, word_298D2;   /* widget params */
int16 word_2B38A;                           /* roster-active flag */
int16 far *word_2B942;                      /* far ptr → reload-request */
uint16 word_29B52;                          /* roster slot index */
char  *word_2591C[8];                       /* theater field table */
char  *word_2592A[8];                       /* mission-kind field table */
uint8 byte_212BA, byte_2D06B, byte_298F6;
void  sub_1A4FB(void) { }
void  sub_1A376(void) { }
void  sub_15189(char *d, char *s) { }
void  sub_13D15(int16 *pg, char *s, int16 a, int16 b, int16 c, int16 d) { }
void  sub_13E7C(long v, char *buf) { }
void  sub_10882(void) { }
/* stpanel.c sub_12754 (object detail panel) globals */
int16 word_2C968, word_2C144;               /* selected object indices */
int16 word_2CA70[8];                        /* id→string-id table */
struct ObjD {                               /* dseg:0xb38e, stride 0x10 */
    int16 f0;                               /* linked id (0 => fE byte) */
    int16 pad2[2];                          /* worldObjects x/y_coord */
    int16 pad4;                             /* sub-record index */
    int16 targetFlags;                      /* &0x400 / &0x100 tested */
    int16 padA;                             /* *0x20 name-table index */
    int16 padC;                             /* count field (itoa'd) */
    uint8  fE;                              /* fallback id */
    uint8  padF;
};
struct ObjD word_2B38E[8];
struct Attr14 { int16 f0, f2;               /* dseg:0x3e26, stride 0x0e */
                uint8 f4, pad5[9]; };
struct Attr14 word_23E26[8];
int16 word_23E28;                             /* aliases word_23E26[0].f2 */
struct Rec18 { int16 f0, pad[8]; };
struct Rec18 word_241C8[8];                 /* dseg:0x41c8, stride 0x12 */
char  strTab14[8][0x0E];                    /* dseg:0x3e1e name strings */
char  strTab32[8][0x20];                    /* dseg:0x3f60 name strings */
/* stobj.c sub_15460 (theater/object data loader) globals */
int16 word_21716;                           /* RLE run state */
uint8 byte_22318, byte_22319, byte_2231A, byte_2231B,
      byte_2231C, byte_2231D, byte_2231E;
int16 word_2C7D8[200];                      /* row-offset table */
int16 word_2D05C;                           /* object file handle */
int16 word_2D2F4;                           /* second alloc seg */
int16 word_2B83E;                           /* gfx mode 0..3 */
int16 word_216D6, word_216CA;
int16 *word_21706, *word_21708;             /* screen ptr pair */
int16 word_21712;
uint8 *word_22320;                          /* buffer ptr (0x1b18) */
uint8 byte_27D37[16], byte_27D47[16],
      byte_27D57[16], byte_27D67[16];       /* nibble palette tables */
int16 word_2367E, word_23680;
int16 word_21718[0x100];                    /* delta-decoded pair table */
int16 word_2CA4E, word_2CA50;               /* draw offsets */
int16 word_2D05A;                           /* stream state flag */
int16 seg_2D274;                            /* source data segment */
int16 sub_15AD2(int32 a) { return (int16)a; }
int16 sub_153F2(int16 a, int16 fd) { return 0; }
int16 sub_15B02(int16 n) { return n; }
void  sub_169BE(int16 a, int16 b, int16 c, int16 d) { }
void  sub_16AE7(int16 a, int16 b, int16 c, int16 d) { }
void  sub_168C8(int16 a, int16 b, int16 c, int16 d,
                int16 e, int16 f, int16 g, int16 h) { }
void  sub_16D93(int16 a, int16 b, int16 c, int16 d, int16 e, int16 f) { }
void  sub_16D90(int16 a, int16 b, int16 c, int16 d, int16 e, int16 f) { }
void  sub_16DC2(int16 a, int16 b, int16 c, int16 d, int16 e, int16 f) { }
void  sub_16E0C(int16 a, int16 b, int16 c, int16 d, int16 e, int16 f) { }
void  far ovlCall_ba9(void) { }             /* overlay 1000:0ba9 */
int8  far ovlCall_cbc(void) { return 0; }   /* overlay 1000:0cbc key-ready (al) */
int16 far ovlCall_cc1(void) { return 0; }   /* overlay 1000:0cc1 */
void  far ovlFee_23(void) { }               /* overlay 0fee:0x23 joy settle */
/* stmap.c sub_11366 deps */
uint16 *word_27E50;                         /* palette-cycle table ptr */
uint16 word_27E54;                          /* cycle index */
uint8  byte_27E59;                          /* blink phase */
uint8  byte_216AA, byte_216AB;              /* joystick axis centers */
uint8  byte_20A1D;                          /* anim tick counter (0xa1d) */
