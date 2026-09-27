/* START.EXE — roster save (seg000:0xa376). EN variant: when doFcbSearch finds
 * the disk write-protected, draws the warning and waits for a key/joy release
 * instead of writing the file. */
#include "inttype.h"
#include <stdio.h>

struct GD0 { int16 pilotIdx; };
extern struct GD0 far *gameData;            /* far ptr dseg:0x991c */
extern int16 hallfameCount;                 /* dseg:0x9b52 */
typedef struct { char data[0x50]; } HallfameEntry;
extern HallfameEntry hallfameBuf[10];       /* dseg:0x9d54 */
struct CommJoy { int8 pad[0x72]; int16 joyPresent; };
extern struct CommJoy far *commData;        /* far ptr dseg:0xd066 */
extern int16 *uiPage;                       /* dseg:0x56e2 */
extern char scrStrBuf[];                    /* dseg:0xb96a */
extern int16 savedPage;                     /* dseg:0xd06c */

extern int16 doFcbSearch(void);             /* seg000:0x40de asm */
extern void sub_14089(int16 n);             /* seg000:0x4089 waitTicks */
extern void mystrcpy(char *d, const char *s);
extern int16 stringWidth(int16 *page, const uint8 *s);
extern void drawStringAt(int16 *page, const char *s, int16 x, int16 y);
extern int16 far misc_jump_5a_keybuf(void);
extern int16 far misc_jump_5b_getkey(void);
extern int16 far misc_jump_5d_readJoy(int16 n);
extern int16 far gfx_blitToCurrent(int16 pagePtr);

void saveHallfame(void) {
    FILE *file;
    int16 idx;
    int16 w;
    hallfameCount = gameData->pilotIdx;
    if (doFcbSearch() != 0) {
        file = fopen("Roster.Fil", "wb");
        fwrite(&hallfameCount, 2, 1, file);
        for (idx = 0; idx < 0xa; idx++) {
            fwrite(&hallfameBuf[idx], 0x50, 1, file);
        }
        fclose(file);
    }
    else {
        uiPage[6] = 4;
        uiPage[2] = 0;
        uiPage[3] = 0xf;
        gfx_blitToCurrent(savedPage);
        mystrcpy(scrStrBuf, "Original disk in drive.  Roster file will not be saved.");
        w = stringWidth(uiPage, (uint8*)scrStrBuf);
        drawStringAt(uiPage, scrStrBuf, (0x140 - w) / 2, 0x64);
        uiPage[2] = 9;
        mystrcpy(scrStrBuf, "Press Selector to continue.");
        w = stringWidth(uiPage, (uint8*)scrStrBuf);
        drawStringAt(uiPage, scrStrBuf, (0x140 - w) / 2, 0x96);
        if (commData->joyPresent == 1) {
            while (misc_jump_5a_keybuf() != 0) {
                if (misc_jump_5d_readJoy(0) != 0) break;
            }
            if (misc_jump_5a_keybuf() == 0) misc_jump_5b_getkey();
        }
        else {
            misc_jump_5b_getkey();
        }
        if (commData->joyPresent == 1) {
            while (misc_jump_5d_readJoy(0) != 0) ;
            sub_14089(5);
            while (misc_jump_5d_readJoy(0) != 0) ;
        }
        gfx_blitToCurrent(savedPage);
    }
}

/* seg000:0xa4fb — load Roster.Fil hall-of-fame records (mirrors saveHallfame) */
void loadHallfame() {
    FILE *file;
    int16 idx;
    file = fopen("Roster.Fil", "rb");
    fread(&hallfameCount, 2, 1, file);
    for (idx = 0; idx < 0xa; idx++) {
        fread(&hallfameBuf[idx], 0x50, 1, file);
    }
    fclose(file);
}
