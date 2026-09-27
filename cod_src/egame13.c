// seg000 debug code (/Zi)
#include "debug.h"
#include "egame.h"
#include "offsets.h"
#include "pointers.h"
#include "slot.h"
#include "const.h"
#include "comm.h"

#include <dos.h>
#include <stdio.h>
#include <string.h>

// ==== seg000:0x55ab ====
// something to do with view switching?
int ProcessPlayerInputAndAI() {
    int var_2, var_4, var_6, var_8, var_A, var_C, var_E;
    dword_3B1FE = dword_3C01C = g_ViewX;
    dword_3B4D4 = g_ViewY;
    dword_3C024 = 0x100000 - g_ViewY;
    // 55ef
    word_3B4DE = g_viewZ + 0x18;
    word_3C02C = g_viewZ;
    var_2 = word_336FE = sub_1CF64(word_336FE, 2, 8);
    // 5613
    switch(keyValue) {
    case 0:
    case 0x44: // 5616
        word_3C5AA = g_ourHead;
        word_3BE94 = g_ourPitch;
        word_3B4E4 = g_ourRoll;
        // 5628
        break;
    case 0x41: // 562b
        word_3C5AA = g_ourHead + 0x8000;
        word_3BE94 = -g_ourPitch;
        word_3B4E4 = -g_ourRoll;
        // 5644
        break;
    case 0x43:
        word_3C5AA = g_ourHead + 0x4000;
        word_3BE94 = -g_ourRoll;
        word_3B4E4 = g_ourPitch;
        // 565e
        break;
    case 0x42:
        word_3C5AA = g_ourHead - 0x4000;
        word_3BE94 = g_ourRoll;
        word_3B4E4 = -g_ourPitch;
        // 5678
        break;
    case 0x84: // 567b
        var_E = (word_336E8 - ((g_frameRateScaling  + 1) / 2) - 1) & 0xf;
        word_3C5AA = stru_3A95A[var_E].field_A;
        word_3BE94 = stru_3A95A[var_E].field_C;
        // 56a9
        word_3B4E4 = stru_3A95A[var_E].field_E;
        dword_3B1FE = stru_3A95A[var_E].field_0;
        dword_3B4D4 = stru_3A95A[var_E].field_4;
        word_3B4DE = stru_3A95A[var_E].field_8;
        // 56d1
        break;
    case 0x85:
        word_3C5AA = g_ourHead - 0x4000;
        word_3BE94 = 0;
        word_3B4E4 = 0;
        // 5708
        dword_3B1FE = sinX(g_ourHead + 0x4000, 0x18 << var_2) + g_ViewX;
        // 572e
        dword_3B4D4 = cosX(g_ourHead + 0x4000, 0x18 << var_2) + g_ViewY;
        // 5735
        break;
    case 0x86: // 5738
        word_3C5AA = 0x8000;
        word_3BE94 = 0;
        word_3B4E4 = 0;
        dword_3B4D4 = (0x18 << var_2) + g_ViewY;
        // 5762
        break;
    case 0x87:
        word_3C5AA = g_ourHead;
        word_3BE94 = 0;
        word_3B4E4 = 0;
        // 5796
        dword_3B1FE = sinX(g_ourHead + 0x8000, 0x18 << var_2) + g_ViewX;
        // 57ad
        dword_3B4D4 = cosX(g_ourHead + 0x8000, 0x18 << var_2) + g_ViewY;
        word_3B4DE = (4 << var_2) + g_viewZ;
        // 57d2
        break;
    case 0x88:
    case 0x89:
    case 0x8b: // 57d5
        if (keyValue != 0x89) { // 57dd
            if (g_currentWeaponType == 1) { // 57e4
                // XXX: test byte ptr word_336F2, 80h -> check which byte is tested, other byte ptr instructions in this routine
                if (!(word_336F2 & 0x80)) word_3C02E = word_336F2 + 0x20;
            }
            else { // 57f6
                if (!(word_336F4 & 0x80)) word_3C02E = word_336F4 + 0x40;
            }
            // 5806
        }
        else { // 5808
            if (word_3370E == 0) word_3C02E = word_3A940;
        } // 5815
        var_4 = var_2;
        // 581b
        if (!(word_3C02E & 0x40)) { // 5825
            if (!(word_3C02E & 0x20)) { // 582c
                if (stru_335C4[word_3C02E].field_E != 0) { // 583c
                    // 584e
                    dword_3C01C = (uint32)(stru_335C4[word_3C02E].field_0) << 5;
                    // 5867
                    dword_3C024 = (uint32)(stru_335C4[word_3C02E].field_2) << 5;
                    word_3C02C = stru_335C4[word_3C02E].field_4;
                }
                else { // 5877
                    stru_335C4[word_3C02E].field_8 = g_ourHead;
                    stru_335C4[word_3C02E].field_A = g_ourPitch;
                    if (word_3370E != 0) keyValue = 0x87;
                } // 589b
                var_2 = 5;
            }
            else { // 58a2
                // .... word_3C02E & 0x1f
                dword_3C01C = stru_3B208[word_3C02E & 0x1f].field_2;
                dword_3C024 = stru_3B208[word_3C02E & 0x1f].field_6;
                word_3C02C = stru_3B208[word_3C02E & 0x1f].field_0;
                var_2 = 5;
            } // 58d9
        }
        else { // 58db
            dword_3C01C = (uint32)g_planes[word_3C02E & 0x3f].field_0 << 5;
            dword_3C024 = (uint32)g_planes[word_3C02E & 0x3f].field_2 << 5;
            // 5928
            word_3C02C = g_planes[word_3C02E & 0x3f].field_6 & 0x200 ? 0xc8 : 0x32;
            var_2 = 7;
            if (word_336EA != 0 && word_3370C == -1) var_2 = 6;
        } // 5943
        if (word_3370E == 0) var_2 = var_4;
        // 5967
        var_A = (dword_3C01C >> 5) - g_viewX_;
        // 5981
        var_C = (dword_3C024 >> 5) - g_viewY_;
        var_6 = Dist2D(var_A, var_C);
        word_3C5AA = ARCTAN(var_A, -var_C);
        // 59ba
        word_3BE94 = -ARCTAN((word_3C02C - g_viewZ) >> 5, var_6);
        word_3B4E4 = 0;
        // 59d6
        var_8 = cosX(word_3BE94, 0x18 << var_2);
        if (word_3C02E & 0x60 || word_3370E != 0) { // 59ea
            if (keyValue == 0x88) { // 59f2
                // 5a0b
                dword_3B1FE = sinX(word_3C5AA + 0x8000, var_8) + g_ViewX;
                dword_3B4D4 = cosX(word_3C5AA + 0x8000, var_8) + g_ViewY;
                // 5a53
                word_3B4DE = sinX(word_3BE94, 0x18 << var_2) + (4 << var_2) + g_viewZ;
                word_3BE94 = -word_3BE94;
            }
            else { // 5a62
                dword_3B1FE = sinX(word_3C5AA, var_8) + dword_3C01C;
                dword_3B4D4 = cosX(word_3C5AA, var_8) - dword_3C024 + 0x100000;
                // 5ac3
                word_3B4DE = (4 << var_2) - sinX(word_3BE94, 0x18 << var_2) + word_3C02C;
                if (word_3C02E & 0x40 && g_planes[word_3C02E & 0x3f].field_6 & 0x200 && word_3B4DE < 0x84) {
                    word_3B4DE = 0x84;
                } // 5aed
                word_3C5AA += 0x8000;
            } // 5af2
        }
        else { // 5af5
            word_3C5AA = stru_335C4[word_3C02E].field_8;
            word_3BE94 = stru_335C4[word_3C02E].field_A - 0x400;
            // 5b22
            var_8 = cosX(word_3BE94, 0x10 << var_2);
            dword_3B1FE = dword_3C01C - sinX(word_3C5AA, var_8);
            // 5b68
            dword_3B4D4 = 0x100000 - (cosX(word_3C5AA, var_8) + dword_3C024);
            // 5b88
            word_3B4DE = word_3C02C - sinX(word_3BE94, 0x10 << var_2);
        }
        // 5b8c
        break;
    case 0x8c: // 5b8f
        word_3BE94 = 0xf400;
        word_3B4E4 = 0;
        dword_3B1FE = (int32)word_3C028 << 5;
        // 5bcf
        dword_3B4D4 = (0x8000 - (int32)word_3C03A) << 5;
        word_3B4DE = word_3C040;
        // 5bdc
        break;
    } // 5c33
    // 5c3d
    if (abs(word_3BE94) > 0x4000 || word_3BE94 == 0x8000) { // 5c4a
        // 5c51
        word_3BE94 = 0x8000 - word_3BE94;
        word_3C5AA += 0x8000;
        // 5c60
        word_3B4E4 = 0x8000 - word_3B4E4;
    } // 5c63
    if (keyValue == 0) { // 5c6a
        memcpy(unk_3A948, unk_3806E, 0x12);
    }
    else {// 5c7e
        sub_20BAE(unk_3A948, word_3C5AA, word_3BE94, word_3B4E4);
    } // 5c96
    // 5ca1
    word_3B4DE = word_3B4DE < 0x10 ? 0x10 : word_3B4DE;
    var_E = word_330C2;
    word_330C2 = (keyValue & 0xc0) == 0;
    if (var_E != word_330C2) { // 5cc3
        gfx_jump_45_retrace();
        if (word_330C2 != 0) { // 5cd2
            gfx_jump_23();
            // the pointer arguments are probably rastports, RectCopy?
            gfx_jump_2a(*off_38364, 0, 0x61, *off_38334, 0, 0x61, 0x140, 0x67);
            // 5d23
            gfx_jump_2a(*off_38364, 0, 0x61, *off_3834C, 0, 0x61, 0x140, 0x67);
            UpdateThrottleState();
            sub_11A18();
            SetupWeaponDisplay(missileSpecIndex);
            if (word_3C09A == 0) { // 5d42
                RenderScene(g_viewX_, g_viewY_);
            } // 5d50
            word_336F4 = word_336F2 = 0xffff;
            ClearMessageArea(3, 3);
            word_39604 = 0;
        }
        else { // 5d6c
            gfx_jump_2a(*off_38334, 0, 0x61, *off_38364, 0, 0x61, 0x140, 0x67);
        }
    } // 5d96
    if (keyValue != word_38152) { // 5da2
        if (keyValue == 0x42 || keyValue == 0x43 || keyValue == 0x41) { // 5dba
            gfx_jump_45_retrace();
            if (gfx_jump_3f_modecode() == 3) { // 5dc9
                openBlitClosePic(keyValue == 0x42 ? a256left_pic : keyValue == 0x43 ? a256right_pic : a256rear_pic, *off_38334);
            }
            else { // 5df3
                openBlitClosePic(keyValue == 0x42 ? aLeft_pic : keyValue == 0x43 ? aRight_pic : aRear_pic, *off_38334);
            } // 5e1b
            gfx_jump_2a(*off_38334, 0, 0x61, *off_3834C, 0, 0x61, 0x140, 0x67);
            // 5e50
            off_38334[0x10] = off_3834C[0x10] = 0x60;
        }
        else { // 5e58
            off_38334[0x10] = off_3834C[0x10] = word_330C2 != 0 ? 0x60 : 0xc7;
        } // 5e75
        word_38152 = keyValue;
    } // 5e7b
    byte_34197 = byte_228D0[0x2f];
    *(uint8*)(&word_3BE98) = 3;
    if (g_difficultyLevel == 0 && commData->gfxModeNum != 0) { // 5e9d
        byte_34197 = 3;
        *(uint8*)(&word_3BE98) = 0x0b;
    } // 5ea7
    copySomeMem(word_330BC);
    *(uint8*)(&word_36B86) = 0;
    // 5eeb
    sub_121CA(-word_3C5AA, word_3BE94, word_3B4E4, dword_3B1FE, dword_3B4D4, (int32)word_3B4DE, 0, 0, 0x140, off_38334[0x10] + 1);
    byte_3850E = 0;
    byte_3995A = word_36B86;
    // 5efc
    if (keyValue == 0x41) { // 5f06
        sub_160D3(unk_38128);
        gfx_jump_21(0xf);
        g_LineX1 = 0xf1;
        g_LineY1 = 0x15;
        g_LineX2 = 0xfb;
        g_LineY2 = 0x5e;
        // 5f34
        sub_2152A();
        g_LineX1 = 0x53;
        g_LineY1 = 0x15;
        g_LineX2 = 0x49;
        g_LineY2 = 0x5e;
        // 5f51
        sub_2152A();
        gfx_jump_23();
        var_E = byte_3C5A0;
        byte_3C5A0 = gfx_jump_2d();
        // 5f84
        TransformAndProjectObject_2(0x6b, 0x30, 0xd1, 0, 0x6f, 0x2f, 0);
        TransformAndProjectObject_2(0x41, 0x5f, 0x7d, 0x36, 0xc3, 2, 0);
        byte_3C5A0 = var_E;
    } // 5fb1
    gfx_jump_46_retrace2();
    // 5fd3
    // height of picture depending on whether view full or cockpit in the way?
    word_38126 = (word_3C09E == 0x13 || word_3C09A == 1 || word_330C2 == 0) ? 0xc8 : 0x61;
} // 5fda

// ==== seg000:0x8e50 ====
int UpdateFlightModelAndHUD(int arg_0) {
    int var_2, var_4, var_6, var_8, var_A, var_C, var_E, var_10, var_12, var_14, var_16, var_18, var_1A;
    char var_1C;
    byte_3C5A0 = gfx_jump_2d();
    // probably x,y
    var_16 = waypoints[waypointIndex].x - g_viewX_;
    var_1A = waypoints[waypointIndex].y - g_viewY_;
    // 8e83
    word_3BE92 = ARCTAN(var_16, -var_1A);
    if (word_330C2 != 0) { // 8e96
        if (word_38FEA != 0) { // 8e9d
            word_38FEA = 0;
            if (!(keyValue & 0x80)) { // 8eaa
                sub_19E44(0xd);
                DrawHUDElement(0, 0, 0x13f, 0x60);
                gfx_jump_4f(0x3c);
            }
        } // 8ed2
        byte_37C2F = 1;
        if (keyValue == 0 && g_halfSpeedUpdateFlag == 0) { // 8eeb
            if (!commData->setupUseJoy) { // 8ef9
                sub_19E44(0);
                sub_19C0C(0x115, 0x53, 0x125, 0x53);
                sub_19C0C(0x125, 0x53, 0x125, 0x5f);
                // 8f3e
                sub_19C0C(0x125, 0x5f, 0x115, 0x5f);
                sub_19C0C(0x115, 0x5f, 0x115, 0x53);
                sub_19C0C(0x11d, 0x59, 0x11d, 0x59);
                // 8f74
                sub_19E44(0xf);
                var_14 = ((int16)(noJoy80 - 0x78) >> 4) + 0x11d;
                // 8fa1
                var_18 = ((int16)((noJoy80_2 * 3) - 0x168) >> 6) + 0x59;
                sub_19C0C(var_14 - 1, var_18, var_14 + 1, var_18);
                // 8fc8
                sub_19C0C(var_14, var_18 + 1, var_14, var_18 - 1);
            } // 8fce
            if (g_playerPlaneFlags & 0x200) { // 8fd6
                sub_19E44(0xf);
                sub_19C0C(0x9c, 0x59, 0xa4, 0x59);
                sub_19C0C(0xa0, 0x56, 0xa0, 0x5c);
            } // 900c
            sub_19E44(word_330BC != 0 ? 4 : 0);
            // 9041
            var_10 = sub_1CF64((((word_3C5A6 - g_knots) * 2) / 5) + 0x1d, 0, 0x3d);
            if (var_10) sub_19C0C(0x48, 0x55 - var_10, 0x48, 0x55);
            // 9089
            sub_19C0C(0xf7,  0x38, 0xf7, sub_1CF64(-((word_3C8B6 >> 4) - 0x38), 0x14, 0x55));
            // 908f
            if ((g_playerPlaneFlags & 1) == 0 && (word_336E8 & 1) != 0 && gameData->unk4 != 0 && word_3C8B6 < 0) { // 90af
                var_2 = (((g_planes[g_closestThreatIndex].field_6 & 0x200 ? 0x100 : 0x80) / gameData->unk4) >> 4) + 0x38;
                sub_19E44(0xf);
                // 90f7
                sub_19C0C(0xf2, var_2 - 2, 0xf4, var_2);
                sub_19C0C(0xf2, var_2 + 2, 0xf4, var_2);
            } // 9115
            // stall warning display
            if (g_knots < word_3C5A6 && word_3BEBE != g_viewZ && word_336E8 & 1) { // 912e
                draw2Strings(aStallWarning, 0x84, 0x1e, 0xf);
            } // 9144
            if (g_currentWeaponType == 0 || g_currentWeaponType == 2) { // 9152
                sub_19E44(7);
                word_3C008 = (word_38FC4 >> 6) + 0x38;
                if (word_3C008 > 0xa && word_3C008 < 0x6f) { // 9173
                    TransformAndProjectObject_2(0x9a, word_3C008 - 4, 0x94, 0x15, 0x0b, 7, 0xf);
                }
            } // 9198
            if (g_currentWeaponType == 1) { // 91a2
                var_1C = g_halfSpeedUpdateFlag + 4;
                var_14 = (word_3C6A4 >> var_1C) + 0x9f;
                var_18 = (word_3C6AC >> var_1C) + 0x38;
                // 91c3
                if (var_14 > 0xa && var_14 < 0x135 && var_18 > 8 && var_18 < 0x5b) { // 91da
                    TransformAndProjectObject_2(var_14 - 6, var_18 - 5, 0x91, 0x4, 0xd, 0xb, 0xe);
                } // 9202
                // 7 = air to air? Only Sidewinder and Amraam have it
                if (sams[missiles[missleSpec[missileSpecIndex].field_0].field_16].field_C == 7) { // 9223
                    sub_19E44((uint8)gfxModeUnset != 0 ? 0xf : 7);
                    // 9239
                    for (var_A = 0; var_A <= 0x100; var_A += 0x10) { // 924b
                        var_4 = var_A << 8;
                        var_8 = sinX(var_4, 0x28) + 0x9f;
                        // 9278
                        var_C = -(cosX(var_4, 0x23) - 0x38);
                        if (var_A != 0) sub_19C0C(var_8, var_C, var_E, var_12);
                        // 9294
                        var_E = var_8;
                        var_12 = var_C;
                    }
                }
            } // 929f
            sub_1A183(g_knots, 0x50, 0x36, 0xf);
            if (word_380D0 <= 0x4e20) { // 92bd
                sub_1A183(word_380D0 < 0x64 ? word_380D0 : (word_380D0 / 5) * 5, 0xe4, 0x36, 0xf);
            } // 92ee
            if (word_3370A > 1) { // 92f5
                drawSomeStrings(aAccel, 0x96, 0x4, 0xf);
            } // 930b
            if (g_playerPlaneFlags & 0x1000) { // 9313
                drawSomeStrings(aTraining, 0xea, 0x10, 0xf);
            } // 9329
            if (g_autopilotAltitude != 0) { // 9330
                drawSomeStrings(aAutopilot, 0xec, 0x5a, 0xf);
            } // 9346
            var_6 = sub_1CF64((((word_3BE92 - g_ourHead) >> 6) / 3) + 0x9f, 0x59, 0xe5);
            sub_19E44(0x0b);
            sub_19C0C(var_6 - 2, 0xf, var_6, 0x11);
            // 93a0
            sub_19C0C(var_6, 0x11, var_6 + 2, 0xf);
            sub_19C0C(var_6 - 2, 0xf, var_6 + 2, 0xf);
            goto somewhere;
        } // 93c4
somewhere:
        MakeRotationMatrix(byte_3C5A0);
    } // 93cf
    if (word_383F2 != 0 && ((keyValue == 0 && g_halfSpeedUpdateFlag == 0) || (word_3370E != 0))) { // 93eb
        draw2Strings(tempString, -(((int16)strlen(tempString) >> 1) - 0x28) * 4, 0x18, 0xf);
        word_383F2--;
        // 9417
        if (word_336EA == 1) { // 941e
            draw2Strings(aPressAnyKeyToP, 0x78, 1, word_330BC != 0 ? 0xe : 0);
        }
    } // 943f
    if (word_383F4 != 0 && keyValue == 0 && g_halfSpeedUpdateFlag == 0) { // 9454
        draw2Strings(string_3C04A, -(((int16)strlen(string_3C04A) >> 1) - 0x28) * 4, 0x5a, 0xf);
        word_383F4--;
    } // 9480
} // 9485

// ==== seg000:0x9e44 ====
void sub_19E44(int arg_0) {
    off_38334[2] = arg_0;
    off_3834C[2] = arg_0;
}

// ==== seg000:0x9e5d ====
void DrawHUDElement(int arg_0, int arg_2, int arg_4, int arg_6) {
    sub_21444(off_38334, arg_0, arg_2, arg_4, arg_6);
    sub_21444(off_3834C, arg_0, arg_2, arg_4, arg_6);
}

// ==== seg000:0xa0cb ====
int drawSomeStrings(char *arg_0, int arg_2, int arg_4, int arg_6) {
    drawString(off_38334, arg_0, arg_2, arg_4, arg_6);
    drawString(off_3834C, arg_0, arg_2, arg_4, arg_6);
}

// ==== seg000:0xa13a ====
int drawString(int* arg_0, char *arg_2, int arg_4, int arg_6, int arg_8) {
    arg_0[6] = 0;
    arg_0[4] = arg_4;
    arg_0[5] = arg_6;
    arg_0[2] = arg_8;
    gfx_jump_05_drawString(arg_0, strupr(arg_2), strlen(arg_2));
}
