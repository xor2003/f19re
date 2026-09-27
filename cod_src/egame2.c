// seg000 optimized code (/Ot)
#include "debug.h"
#include "egame.h"
#include "offsets.h"
#include "pointers.h"
#include "slot.h"
#include "const.h"

#include <dos.h>
#include <memory.h>

// ==== seg002:0xa ====
int far sub_21A7A() {
    GameLoopUpdate();
}

// ==== seg002:0xe ====
void far sub_21A7E() {
    InitializeGameSettings();
}

// ==== seg000:0x21c6 ====
int sub_121C6() {
    return load3DAll();
}

// ==== seg000:0x2278 ====
int sub_12278(int arg_0) {
    uint8 target_tick;

    if (arg_0 > 0) {
        target_tick = (uint8)arg_0 + byte_3790C[0];
        while (target_tick != byte_3790C[0]) {
        }
    }
}

// ==== seg000:0xc8de ====
int load15Flt3d3() {
    int var_A, var_C;
    struct SREGS var_8;
    uint8 FAR *var_10;
    strcpyFromDot(a15flt_xxx, a_3d3_0);
    fileHandle = fopen(a15flt_xxx, aRb_4);
    if (fileHandle == NULL) {
        // c90b
        printError(aOpenErrorOn_3d3_0);
        return;
    }
    // c922
    fread(&flt15_word1, 2, 1, fileHandle);
    fread(&flt15_size, 2, 1, fileHandle);
    fread(flt15_buf1, 2, flt15_size, fileHandle);
    fread(&var_A, 2, 1, fileHandle);
    segread(&var_8);
    var_10 = byte_2D6A4;
    while(var_A > 0) {
        // c98b
        var_C = var_A <= 0x800 ? var_A : 0x800;
        // c99b
        fread(flt15_buf2, 1, var_C, fileHandle);
        movedata(var_8.ds, (uint16)flt15_buf2, FP_SEG(var_10), FP_OFF(var_10), var_C);
        var_A -= 0x800;
        FP_SEG(var_10) += 0x800;
    }
    // c9ca
    fclose(fileHandle);
}

// ==== seg000:0xdd4c ====
int openFileWrapper(char *path, int mode) {
    return openFile(path, mode);
}

// ==== seg000:0xdd5e ====
int sub_1DD5E(char *path, int attr) {
    return createFile(path, attr);
}

// ==== seg000:0xdd70 ====
int closeFileWrapper(int arg_0) {
    return closeFile(arg_0);
}

// ==== seg000:0xdd7e ====
int sub_1DD7E(int arg_0, void *arg_2, int arg_4) {
    return readFile1(arg_0, arg_2, arg_4);
}

// ==== seg000:0xdd92 ====
int sub_1DD92(int arg_0, void *arg_2, int arg_4, int arg_6) {
    return readFile2(arg_0, arg_2, arg_4, arg_6);
}

// ==== seg000:0xddaa ====
int sub_1DDAA(int arg_0, void *arg_2, int arg_4, int arg_6, int arg_8) {
    return sub_1DF4F(arg_0, arg_2, arg_4, arg_6, arg_8);
}
