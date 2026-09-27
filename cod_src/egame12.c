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

// ==== seg000:0x3f72 ====
int otherKeyDispatch(void) {
    // Local variables corresponding to the stack frame (sub sp, 3Eh)
    int16 var_3E, var_3C, var_38, var_34, var_32, var_2C, var_2A, var_28,
     var_24, var_22, var_20, var_1A, var_18, var_16, var_14, var_10,
     var_E, var_C;

    // Allocate stack, unused
    int16 temp_si;
    int16 temp_di;
    int16 temp_ax, temp_bx, temp_cx;
    int32 temp_long_dx_ax;
    int16 temp_2, temp_3, temp_4, temp_5;

    // seg000:3F7A
    if (word_3BECC == 0) {
        // seg000:3F81
        g_thrust =
        g_setThrust =
        g_velocity =
        word_380D0 =
        g_viewZ =
        g_ourRoll =
        g_ourPitch = 0;

        // seg000:3F9C
        if (gameData->difficulty == 0) {

            // seg000:3FA2
            g_ourHead = ((g_viewY_ - (waypoints[1].y)) < 0x8000) ? 0 : 0x8000;
        } else {
            // seg000:3FBA
            g_ourHead = (gameData->theater == 6)
               ? 0  // If true, the result is 0
               : ((gameData->theater & 1) ? 0 : 0x8000); // Else, evaluate the second condition
        }

        // seg000:3FDE
        if (g_planes[g_playerTargetIndex].field_6 & 0x200) {
            // seg000:3FE6
            *((unsigned char*)&g_ourHead + 1) += 4;
        }

        // seg000:3FEB
        UpdateRotationMatrix();
        // seg000:3FEE
        UpdateThrottleState();
        // seg000:3FF1
        word_3BECC = 1;
    }

    // seg000:3FF7
    keyScancode = 0;
    // seg000:3FFD
    if (_kbhit()) {
        // seg000:4007
        keyScancode = __bios_keybrd(0);
        // seg000:4010
        if (word_336EA == 1) {
            // seg000:4019
            word_3370E =
            word_336EA =
            keyValue = 0;
        }
    }

    // seg000:4022
    while (_kbhit()) {
        // seg000:402C
        __bios_keybrd(0); // Flush keyboard buffer
    }

    // Main key dispatch logic
    // seg000:4037
    switch (keyScancode) {
        // seg000:41F5
        case 0x0C2D: // Minus
            // seg000:403A
            g_setThrust = sub_1CF64(g_setThrust - 10, 0, 100);
            // seg000:4051
            UpdateThrottleState();
            // seg000:4054
            goto switch_break;
        // seg000:420D
        case 0x0D3D: // Equal
            // seg000:4057
            g_setThrust = sub_1CF64(g_setThrust + ((g_setThrust < 10) ? 5 : 10), 0, 100);
            // seg000:407B
            UpdateThrottleState();
            // seg000:407E
            *((unsigned char*)&g_playerPlaneFlags) &= 0xF7; // ~8
            // seg000:4083
            goto switch_break;
        // seg000:421C
        case 0x1E61: // A
            // seg000:4086
            g_setThrust = 0x90;
            // seg000:408C
            UpdateThrottleState();
            // seg000:408F
            *((unsigned char*)&g_playerPlaneFlags) &= 0xF7; // ~8
            // seg000:4094
            goto switch_break;
        // seg000:4205
        case 0x0D2B: // Shift-Equal
            // seg000:4097
            g_setThrust = 100;
            // seg000:409D
            UpdateThrottleState();
            // seg000:40A0
            *((unsigned char*)&g_playerPlaneFlags) &= 0xF7; // ~8
            // seg000:40A5
            goto post_key_B_check;
        // seg000:41FD
        case 0x0C5F: // Shift-Minus
            // seg000:40A7
            g_setThrust = 0;
            // seg000:40B4
            makeSound(0x10, 0);
            // seg000:40BA
            UpdateThrottleState();
            // seg000:40BD
            goto switch_break;
        // seg000:4234
        case 0x3062: // B
            // seg000:40C0
            *((unsigned char*)&g_playerPlaneFlags) ^= 8;
        // seg000:40C5
        post_key_B_check:
            // seg000:40C5
            if (!(*((unsigned char*)&g_playerPlaneFlags) & 8) && word_3BEBE != 0 && g_setThrust == 100) {
                // seg000:40DA
                g_velocity = 0x546;
                // seg000:40E8
                makeSound(0x1C, 2);
            }
            // seg000:40EE
            goto switch_break;
        // seg000:4224
        case 0x2400: // Alt-J
            // seg000:40F1
            if (word_380E2 == 0) {
                // seg000:40F8
                sub_2265B();
                // seg000:40FD
                word_380E2 = 40;
            }
            // seg000:4103
            goto switch_break;
        // seg000:41EB
        case 0x1000: // Alt-Q
            // seg000:410A
            PrepareToExit(1);
            // seg000:4110
            exitCode = 0;
            // seg000:4115
            goto switch_break;
        // seg000:422C
        case 0x3000: // Alt-B
            // seg000:4118
            if (word_330C2 != 0) {
                // seg000:4141
                gfx_jump_2a(off_38364[0], 0, 0x61, off_38334[0], 0, 0x61, 0x140, 0x67);
            }
            // seg000:414C
            sub_19E44(0);
            // seg000:415E
            DrawHUDElement(0, 0, 0x13F, 0xC7);
            // seg000:417B
            TransformAndProjectObject_2(0, 0, 0x71, 0x37, 0x0C, 7, 0);
            // seg000:4181
            sub_1613B();
            // seg000:4184
            if (word_330C2 != 0) {
                // seg000:41AD
                gfx_jump_2a(off_38364[0], 0, 0x61, off_38334[0], 0, 0x61, 0x140, 0x67);
                // seg000:41D7
                gfx_jump_2a(off_38364[0], 0, 0x61, off_3834C[0], 0, 0x61, 0x140, 0x67);
                // seg000:41DF
                UpdateThrottleState();
            }
            // seg000:41E2
            goto switch_break;
        // seg000:4217
        case 0x1900: // Alt-P
            // seg000:41E4
            sub_1613B();
            // seg000:41E7
            goto switch_break;
    }

// seg000:423E
switch_break:
    // seg000:423E
    if (word_380E2 > 0) {
        // seg000:4245
        word_380E2--;
    }

    // seg000:4249
    if (g_setThrust != 0 && g_thrust == 0) {
        // seg000:425F
        makeSound(0x0E, 2);
    }

    // seg000:4265
    if (word_330BE != 0) {
        // seg000:426C
        noJoy80 = 0;
        // seg000:4271
        noJoy80_2 = 0;
    } else {
        // seg000:427C
        if (commData->setupUseJoy != 0) {
            // seg000:4283
            sub_2267E();
        } else {

            // seg000:4294
            //temp_si = word_38602 + 1;
            noJoy80 = (unsigned char)(((int)((unsigned char)byte_37F98 - 0x80) * (word_38602 + 1)) / 3) - 0x80;

            // seg000:42A9
            noJoy80_2 = (unsigned char)(((int)((unsigned char)byte_37F99 - 0x80) * (word_38602 + 1)) / 3) - 0x80;
        }
    }

    // seg000:42B6
    word_3C00E = ((uint16)noJoy80 >> 4) - 8;
    // seg000:42C5
    if (word_3C00E < 0) word_3C00E++;

    // seg000:42CD
    word_3C5A4 = ((uint16)noJoy80_2 >> 4) - 8;
    // seg000:42DC
    if (word_3C5A4 < 0) word_3C5A4++;

    // seg000:42E4
    word_3C00E = -((abs(word_3C00E) + 2) * word_3C00E) * 2;
    // seg000:42FC
    word_3C5A4 *= 6;
    // seg000:4306
    if (word_3C5A4 < 0) {
        // seg000:430E
        word_3C5A4 /= 2;
    }

    // seg000:4316
    if (word_3BEBE == g_viewZ && word_3C5A4 < 0 && g_ourPitch <= 0) {
        // seg000:432A
        word_3C5A4 = 0;
    }

    // seg000:4330
    if (g_knots > 0x15E && !(*((unsigned char*)&g_playerPlaneFlags) & 1) && word_336EC != 0) {
        // seg000:4346
        word_336EC = 0;
        // seg000:434C
        *((unsigned char*)&g_playerPlaneFlags) |= 1;
        // seg000:4355
        tempStrcpy("Landing gear raised");
        // seg000:4363
        makeSound(0x20, 2);
    }

    // seg000:4369
    if (word_3BEBE == g_viewZ && g_setThrust == 0 && !(*((unsigned char*)&g_playerPlaneFlags) & 8)) {
        // seg000:4380
        *((unsigned char*)&g_playerPlaneFlags) |= 8;
        // seg000:4389
        tempStrcpy("Brakes on");
    }

    // seg000:438F
    if (word_3C00E != 0 || word_3C5A4 != 0) {
        // seg000:439D
        g_autopilotAltitude = 0;
    }

    // seg000:43A3
    if (g_autopilotAltitude != 0)
    {
        // seg000:43AD
        var_2C = (word_336EA != 0) ? (int16)((word_38FE0 & 0xF) << 8) - 0x800
                                   : 0;

        // seg000:43DE
        var_2C = forceRange(var_2C - g_ourHead + word_3BE92, 0xEC00, 0x1400) * 2;

        // seg000:43FD
        word_3C00E = -sub_1CF64((var_2C - g_ourRoll) >> 6, -24, 24);

        // seg000:4420
        var_14 = forceRange(((g_autopilotAltitude - g_viewZ) << 4) - word_38FC4, 0xEC00, 0xC00);

        // seg000:443D
        word_3C5A4 = sub_1CF64((var_14 - g_ourPitch) >> 7, -8, 8);

        // seg000:4446
        if (waypointIndex == 3)
        {
            // seg000:4450
            var_3E = word_3AFA8;
            // seg000:4456
            var_10 = word_3B15A;

            // seg000:4462
            var_2A = g_planes[var_10].field_0 - g_viewX_;
            // seg000:446D
            var_34 = g_planes[var_10].field_2 - g_viewY_;

            // seg000:4478
            if (!(g_planes[var_10].field_6 & 0x200))
            {
                // seg000:4481
                var_3E = -abs_word(var_34);
            }

            // seg000:4493
            var_34 += ((g_planes[var_10].field_6 & 0x200) ? 0x1E : 0x40) * var_3E;

            // seg000:44A9
            var_2C = abs(g_ourHead);
            // seg000:44B6
            if (var_3E == -1)
            {
                // seg000:44BC
                var_2A = -var_2A;
                // seg000:44C4
                var_34 = -var_34;
                // seg000:44D3
                var_2C = abs(g_ourHead - 0x8000);
            }

            // seg000:4514
            var_14 = sub_1CF64((abs(var_2A) + abs(var_34)) * 2 + var_2C / 32, 50, 0x1000);
            // seg000:451D
            if (var_14 < 0x1000)
            {
                // seg000:4522
                sub_1DB9C();
            }

            // seg000:452C
            if (g_planes[var_10].field_6 & 0x200)
            {
                // seg000:4534
                var_14 += 100;
            }

            // seg000:4538
            if (word_33702 != 0 && abs(var_2C) < 0x200)
            {
                // seg000:454D
                var_14 = -20;
            }

            // seg000:4552
            var_34 = g_planes[var_10].field_2 + (((g_planes[var_10].field_6 & 0x200) ? 0x1C : 0x38) * var_3E);

            // seg000:459F
            var_34 += sub_1CF64((abs(var_2A) * 4) + (var_2C / 16), 0, 0xC00) * var_3E;

            // seg000:45AB
            *((unsigned char *)&g_playerPlaneFlags) &= 0xF7;

            // seg000:45B0
            if (var_2C > 0x4000)
            {
                // seg000:45BE
                var_2A = g_planes[var_10].field_0;
                // seg000:45C5
                var_14 = 0x1000;
            }
            else
            {
                // seg000:45CC
                var_2A = g_planes[var_10].field_0 + (var_2A * var_3E * 2);
                // seg000:45E5
                if (g_setThrust * 80 < g_knots)
                {
                    // seg000:45EF
                    *((unsigned char *)&g_playerPlaneFlags) |= 8;
                }
            }

            // seg000:4603
            var_E = ARCTAN(var_2A - g_viewX_, g_viewY_ - var_34);
            // seg000:460C
            var_3C = (int16)g_knots / 16;

            // seg000:4638
            var_2C = forceRange(var_E - g_ourHead, (-var_3C) << 8, var_3C << 8) * 2;

            // seg000:4643
            if (word_33702 != 0)
            {
                // seg000:464A
                var_2C = 0;
            }

            // seg000:4663
            word_3C00E = -sub_1CF64((var_2C - g_ourRoll) >> 6, -32, 32);

            // seg000:46A5
            g_setThrust = sub_1CF64((var_14 / 64) + (abs(var_2C) / 128), 35, 80);
            // seg000:46AE
            UpdateThrottleState();

            // seg000:46CF
            var_14 = forceRange(((var_14 - g_viewZ) >> 3) + (word_38FC4 >> 7), -24, 24);

            // seg000:46EE
            word_3C5A4 = sub_1CF64(var_14 - (g_ourPitch >> 7), -16, 16);

            // seg000:46F7
            if (g_knots < 0x15E)
            {
                // seg000:46FF
                *((unsigned char *)&g_playerPlaneFlags) &= 0xFE;
            }

            // seg000:4704
            if (word_3BEBE == g_viewZ)
            {
                // seg000:470D
                g_setThrust = 0;
                // seg000:4713
                word_3C00E = 0;
                // seg000:4719
                g_playerPlaneFlags |= 8;
                // seg000:471E
                word_3C5A4 = 0;
            }
        }
    }
    // seg000:4724
    if (gameData->unk4 != 0) {
        // seg000:473F
        var_24 = ((int32)g_knots * (1000 - g_viewZ)) >> 15;
    } else {
        // seg000:4753
        var_24 = 0;
    }

    // seg000:4758
    if (!((g_playerPlaneFlags) & 1)) {
        // seg000:4771
        var_24 += sub_1CF64((g_knots - 200) >> 5, 0, 32);
    }

    // seg000:477A
    if (var_24 > 0 && ((uint16)word_3BEBE) < ((uint16)g_viewZ)) {
        // seg000:478C
        word_3C00E += randlmul(var_24) - (var_24 >> 1);
        // seg000:47A0
        word_3C5A4 += (randlmul(var_24) - (var_24 >> 1)) >> 1;
    }

    // seg000:47B3
    if ((g_playerPlaneFlags & 1) && word_3C5A4 <= 0 && ((uint16)g_velocity) < ((uint16)word_3A8FE) && gameData->unk4 < 2 && abs(g_ourRoll) < 0x3000 && word_38FE8 == 0) {
        // seg000:47EB
        var_14 = ((((word_38FC4) - (g_ourPitch)) >> 2) - g_viewZ + 300) >> 2;
        // seg000:4804
        if (var_14 > 0) {
            // seg000:4812
            word_3C5A4 = sub_1CF64(var_14, 0, 32);
        }
    }

    // seg000:481B
    if (word_3BE3C != 0) {
        //int16 pitch_input_modifier;
        //int16 stall_decay_effect;
        int16 bailout_index;

        // seg000:4825
        word_3C00E = 0x40;

        // seg000:482B
        word_3C5A4 = (abs(g_ourRoll) > 0x4000) ? 0x10 : -8; // pitch_input_modifier

        // seg000:4864
        //word_3BE3C++;
        word_3C040 += sub_1CF64(
            - (++word_3BE3C - 0x20),
            (int16)0xFF00 / g_frameRateScaling,
            (int16)0x80 / g_frameRateScaling
        );
        //word_3C040 += stall_decay_effect;

        // seg000:486E
        if (word_3C040 < 0) {
            // seg000:4870
            word_3C040 = 0;
            // seg000:4876
            if ((word_38FE0 & 7) == 0) {
                // seg000:4880
                PrepareToExit(0);
            }
        }

        // seg000:4886
        if (g_viewZ == 0 && word_336F6 == -1) {
            // seg000:4894
            word_336F6 = 0;
            // seg000:489A
            g_planes[0].field_0 = g_viewX_;
            // seg000:48A0
            g_planes[0].field_2 = g_viewY_;
            // seg000:48A6
            word_3BEBC = g_viewX_;
            // seg000:48AC
            word_3BEC8 = g_viewY_;
            // seg000:48B2
            word_3BECE = 0;
            // seg000:48B8
            word_39606 = -8;
            // seg000:48C3
            makeSound(2, 2);
            // seg000:48CB
            g_setThrust =
            g_velocity = 0;
        }

        // seg000:48D1
        if ((word_3BE3C & 0xFFFC) == 0x10 && (word_336E8 & 3) == 1) {
            // seg000:48E4
            word_336F6 = -1;

            // seg000:48ED
            bailout_index = ((uint16)word_336E8 / 2) & 7;

            // seg000:48FB
            stru_33402[bailout_index].field_0 = g_viewX_;
            // seg000:4902
            stru_33402[bailout_index].field_2 = g_viewY_;
            // seg000:4909
            stru_33402[bailout_index].field_4 = g_viewZ;

            // seg000:4914
            stru_33402[bailout_index].field_6 = randlmul(0x20) << 11;

            // seg000:4922
            word_33442 = bailout_index;
            // seg000:4928
            word_3BEBC = g_viewX_;
            // seg000:492E
            word_3BEC8 = g_viewY_;
            // seg000:4934
            word_3BECE = g_viewZ;
            // seg000:493A
            word_39606 = -8;
            // seg000:4947
            makeSound(0, 2);

            // seg000:494D
            g_ourPitch = -0x4000;
            // seg000:4953
            byte_380DD = 1;
        }
    }

    // seg000:4958
    if (g_gunHits != 0) {
        // seg000:496D
        if (g_setThrust > -((g_gunHits * 4) - 0x90)) {
            // seg000:4973
            g_setThrust = -((g_gunHits * 4) - 0x90);
            // seg000:4977
            if (g_setThrust < 0)
                // seg000:497B
                g_setThrust = 0;
            // seg000:4981
            UpdateThrottleState();
        }
    }

    // seg000:4984
    g_thrust += ((g_setThrust - g_thrust) / 4) / ((int16)g_frameRateScaling);
    // seg000:49A7
    if (g_setThrust > g_thrust) g_thrust++;
    // seg000:49B4
    if (g_setThrust < g_thrust) g_thrust = g_setThrust;

    // seg000:49C0
    if ((((uint16)word_336E8) % ((uint16)(g_frameRateScaling << 1))) == 0 && g_setThrust != 0 && word_336EA == 0) {
        // seg000:49ED
        word_33098 -= ((g_setThrust * g_setThrust) / 750) + 2;
        // seg000:49F1
        sub_1606C();
    }

    // seg000:49F4
    if (word_33098 <= 0) {
        // seg000:49FB
        g_thrust = 0;
        // seg000:4A01
        word_33098 = 0;
    }

    // seg000:4A07
    g_gees = byte_37FEC[(abs(g_ourRoll) >> 8) & 0x7f];
    // seg000:4A23
    if (((uint16)word_3BEBE) < ((uint16)g_viewZ)) {
        // seg000:4A34
        g_gees += word_3C5A4 / 2;
    }

    // seg000:4A38
    if (g_gees > 0x80) {
        // seg000:4A40
        g_gees = 0x80;
        // seg000:4A6C
        word_3C5A4 = sub_1CF64(0x80 - byte_37FEC[(abs(g_ourRoll) >> 8) & 0x7f], 0, word_3C5A4);
    }


    // seg000:4A9A
    strcpy(unk_38FD0, itoa(g_gees / 16, strBuf, 10));
    // seg000:4AA8
    strcat(unk_38FD0, ".");

    // seg000:4AD3
    strcat(unk_38FD0, itoa((abs(g_gees) & 0xF) >> 1, strBuf, 10));
    // seg000:4AE1
    strcat(unk_38FD0, "G");

    // seg000:4AFB
    var_32 = (((int32)(g_thrust - sinX(g_ourPitch, 80))) * ((int32)800)) / ((int32)100);

    // seg000:4B16
    word_3C5A6 = 100;
    // seg000:4B30
    var_32 = ((int32)(((uint16)g_viewZ >> 7) + 0x0400) * (int32)var_32) >> 10;

    // seg000:4B56
    word_3C5A6 = ((int32)100 * (int32)(((uint16)word_380D0 >> 6) + 0x0400)) >> 10;

    // seg000:4B83
    var_32 = ((int32)var_32) * ((int32)(-((word_33098 >> 9) - 100))) / (int32)90;

    // seg000:4B9E
    var_32 = (((int32)var_32) * ((int32)(128 - g_gees))) >> 7;

    // seg000:4BC7
    word_3C5A6 = ((int32)sub_15557(g_gees * 4) * (int32)word_3C5A6) >> 3;
    // seg000:4BDA
    word_3C5A6 = abs(word_3C5A6);

    // seg000:4BE3
    if (!(*((unsigned char*)&g_playerPlaneFlags) & 1)) {
        // seg000:4BEA
        var_32 -= var_32 >> 3;
    }

    // seg000:4BF4
    word_3A8FE = word_3C5A6 * 27;
    // seg000:4C08
    var_1A = sub_1CF64(var_32, 0, 900) * 27;

    // seg000:4C39
    g_velocity += ((((int32)var_1A - g_velocity) / 16) / (int32)g_frameRateScaling);

    // seg000:4C5F
    word_3B4DA = ((int32)word_3A8FE * 3072) / (abs(g_velocity) + 1);
    // seg000:4C65
    if ((uint16)word_3B4DA > 0x2000) word_3B4DA = 0x2000;

    // seg000:4C7B
    word_38FC4 = cosX(g_ourRoll, word_3B4DA - 0x300);

    // seg000:4C84
    if (*((unsigned char*)&g_playerPlaneFlags) & 8) {
        // seg000:4C8B
        if (word_3BEBE == g_viewZ) {
            // seg000:4CB0
            g_velocity -= (-((gameData->unk4 * 8) - 32) * 27) / g_frameRateScaling;
            // seg000:4CB8
            if (word_3BEBE != 0 && (uint16)g_velocity < 0x1B0) {
                 // seg000:4CC7
                 g_velocity = 0;
            }
        } else {
             // seg000:4CD8
             g_velocity -= ((uint16)g_velocity >> 4) / g_frameRateScaling;
        }
    }

    // seg000:4CE0
    if ((uint16)g_velocity > 0xAFC8) g_velocity = 0;

    // seg000:4CF6
    var_22 = cosX(g_ourPitch, g_velocity);
    // seg000:4D07
    g_knots = (uint16)g_velocity / 27;

    // seg000:4D11
    audio_jump_6a(g_knots, g_thrust);

    // seg000:4D47
    var_18 = (((int32)sinX(g_ourRoll, g_gees << 4)) << 7) / ((int32)((int16)((uint16)g_velocity >> 9) + 0x20));

    // seg000:4D52
    var_18 = cosX(g_ourPitch, var_18);

    // seg000:4D5B
    if (word_3BEBE == g_viewZ) {
        // seg000:4D64
        var_18 = (word_3C00E * -1) << 6;
        // seg000:4D70
        word_3C00E = 0;
        // seg000:4D76
        if (g_knots < word_3C5A6) {
            // seg000:4D7F
            word_3C5A4 = 0;
        }
    }

    // seg000:4D85
    if (word_38FDE != 0) {
        // seg000:4D8C
        word_3C5A4 = -2000 - g_ourPitch;
        // seg000:4D96
        g_velocity =
        g_setThrust = 0;
    }

    // seg000:4DB6
    var_28 = (((int32)word_3C00E) << 7) / ((int32)g_frameRateScaling);
    // seg000:4DBC
    if (var_28 != 0) {
        // seg000:4DC1
        word_380AC = word_380A4 = sub_13B86(var_28);
        // seg000:4DD0
        word_380A6 = sub_13B96(var_28);
        // seg000:4DD9
        word_380AA = -word_380A6;
        // seg000:4DE6
        sub_151F9(unk_3806E, &word_380A4);
    }

    // seg000:4DF8
    var_20 = (int16)((int32)word_3C5A4 << 7) / g_frameRateScaling;
    // seg000:4DFD
    if (var_20 != 0) {
        // seg000:4E02
        word_380A2 = word_3809A = sub_13B86(var_20);
        // seg000:4E11
        word_380A0 = sub_13B96(var_20);
        // seg000:4E1A
        word_3809C = -word_380A0;
        // seg000:4E27
        sub_151F9(unk_3806E, unk_38092);
    }

    // seg000:4E35
    var_16 = (int16)var_18 / g_frameRateScaling;
    // seg000:4E3A
    if (var_16 != 0) {
        // seg000:4E3F
        word_38090 = word_38080 = sub_13B86(var_16);
        // seg000:4E4E
        word_38084 = sub_13B96(var_16);
        // seg000:4E57
        word_3808C = -word_38084;
        // seg000:4E64
        sub_151F9(&word_38080, unk_3806E);
    }

    // seg000:4E6A
    sub_15237();

    // seg000:4E6D
    if ((uint16)g_velocity > (uint16)word_3A8FE && (uint16)word_3BEBE < (uint16)g_viewZ) {
        // seg000:4E9A
        g_ourPitch -= ((uint16)word_3A8FE - (uint16)g_velocity) >> ((gameData->unk4 == 2 || g_gunHits > 8) ? 1 : 2);
        // seg000:4EA4
        byte_380DD = 1;
        // seg000:4EA9
        if (g_ourPitch < 0 || (uint16)g_viewZ < 200) {
            // seg000:4EC0
            makeSound(0x14, 1);
        }
    }

    // seg000:4EC6
    if (word_3BEBE == g_viewZ) {
        // seg000:4ECF
        if (g_ourRoll != 0) {
            // seg000:4ED6
            g_ourRoll = 0;
            // seg000:4EDC
            byte_380DD = 1;
        }
        // seg000:4EE1
        if (g_ourPitch < 0 || (g_ourPitch > 0 && g_knots < word_3C5A6)) {
            // seg000:4EF3
            if (word_38FDE == 0) {
                // seg000:4EFA
                g_ourPitch = 0;
            }
            // seg000:4F00
            byte_380DD = 1;
        }
    }

    // seg000:4F05
    word_38FDE = 0;

    // seg000:4F0B
    byte_3C6A0 = ( (abs(g_ourPitch)) - (abs(g_ourRoll) / 2) > 0x1000) ? 1 : 0;

    // seg000:4F36
    if (byte_380DD) {
        // seg000:4F3D
        UpdateRotationMatrix();
    }

    // seg000:4F40
    var_C = word_380D0;
    // seg000:4F60
    word_3C8B6 = FixedPointMul((((uint16)g_velocity) / 10), sub_13B96(g_ourPitch - word_38FC4));

    // seg000:4F69
    if (word_33712 == 0) {
        // seg000:4F75
        word_380D0 += (word_3C8B6 / g_frameRateScaling);

        // seg000:4F9A
        g_ViewX += FixedPointMul(var_22, sub_13B96(g_ourHead)) / 10 / g_frameRateScaling;

        // seg000:4FC4
        g_ViewY += FixedPointMul(var_22, sub_13B86(g_ourHead)) / 10 / g_frameRateScaling;
    }

    // seg000:4FCF
    if ((uint16)word_380D0 > 0xf230 || (uint16)word_380D0 < (uint16)word_3BEBE) {
        // seg000:4FE0
        word_380D0 = word_3BEBE;
    }
    // seg000:4FE6
    if (word_380D0 > 0xEA60) word_380D0 = 0xEA60;

    // seg000:4FF4
    if (word_380D0 < 0x2000) {
        // seg000:4FFC
        g_viewZ = word_380D0;
    } else if (word_380D0 < 0x4000) {
        // seg000:500C
        g_viewZ = ((word_380D0 - 0x2000) >> 1) + 0x2000;
    } else {
        // seg000:501C
        g_viewZ = ((word_380D0 - 0x4000) >> 2) + 0x3000;
    }

    // seg000:502C
    if (word_3BEBE == g_viewZ) {
        // seg000:5038
        if (var_C > word_3BEBE && word_33702 != 0) {
            // seg000:5055
            makeSound(0xC, 2);
            //temp_bx = g_closestThreatIndex << 4;

            // seg000:5063
            if ((( ((g_planes[g_closestThreatIndex].field_6 & 0x200) ? 0x100 : 0x80)
                < ((int16)(-word_3C8B6 * g_missionStatus) / 2))) ||
                ((gameData->unk4 != 0 &&
                    (((g_playerPlaneFlags & 1)!=0) ||
                        (((int16)abs(g_ourRoll)) > (int16)((0x30 / (g_missionStatus + 1)) << 8))) ))) {
                            // seg000:50B8
                            makeSound(0, 2);
                            // seg000:50C9
                            sub_12278(0x3C);
                            // seg000:50D3
                            PrepareToExit(5);
            }
        }
        // seg000:50D9
        word_3C8B6 = 0;
    }

    // seg000:50DF
    var_38 = word_336E8 & 0xF;
    // seg000:50EE
    stru_3A95A[var_38].field_A = g_ourHead;
    // seg000:50F5
    stru_3A95A[var_38].field_C = g_ourPitch;
    // seg000:50FC
    stru_3A95A[var_38].field_E = g_ourRoll;
    // seg000:510A
    *(int32*)&stru_3A95A[var_38].field_0 = g_ViewX;
    // seg000:5119
    *(int32*)&stru_3A95A[var_38].field_4 = g_ViewY;
    // seg000:5121
    stru_3A95A[var_38].field_8 = g_viewZ;

    // seg000:5128
    if (g_currentWeaponType == 1) {
        // seg000:5132
        if (word_336F2 >= 0) {
            // TODO struct
            // seg000:5168
            var_38 = sub_1CF64((Dist2D(g_viewX_ - word_3B204[word_336F2 * 0x12], g_viewY_ - word_3B206[word_336F2 * 0x12]) * g_frameRateScaling) >> 8, 0, 12);

        } else {
            // seg000:5173
            var_38 = g_frameRateScaling - 1;
        }

        // seg000:517A
        var_38 = (word_336E8 - var_38) & 0xF;

        // seg000:518C
        var_2C = g_ourHead - stru_3A95A[var_38].field_A;
        // seg000:5196
        var_14 = g_ourPitch - stru_3A95A[var_38].field_C;

        // seg000:51BF
        word_3C6A4 = cosX(g_ourRoll, ((-var_2C) >> 2)) + sinX(g_ourRoll, (var_14 >> 2));

        // seg000:51E8
        word_3C6AC = sinX(g_ourRoll, (var_2C >> 2)) + cosX(g_ourRoll, (var_14 >> 1));
    }

    // seg000:51F8
} // 51f9
