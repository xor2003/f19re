#include "egame.h"
#include <stdlib.h> /* For abs() */

int UpdateRotationMatrix()
{
    int result;

    result = sub_20BAE(0x57BE, g_ourHead, g_ourPitch, g_ourRoll);
    byte_380DD = 0;
    word_380D8 = 0;
    return result;
}

int sub_1543B(int arg_0, int arg_2)
{
    int sign_0;
    int sign_2;
    unsigned long abs_0;
    unsigned long abs_2;
    int result;

    sign_0 = 1;
    sign_2 = 1;
    if (arg_0 < 0) {
        sign_0 = -1;
    }
    if (arg_2 < 0) {
        sign_2 = -1;
    }

    abs_0 = (unsigned long)(unsigned int)abs(arg_0);
    abs_2 = (unsigned long)(unsigned int)abs(arg_2);
    result = (int)(abs_2 / abs_0);
    result >>= 1;
    result *= sign_2;
    result *= sign_0;
    return result;
}

int sub_15540(int angle)
{
    return 0x4000 - sub_154B7(angle);
}

/*
Replicates the integer square root function at seg000:5557.
It uses an iterative method to find the root.
- An initial guess is made (n / 4).
- The guess is refined in a loop by averaging the guess and (n / guess).
- The loop terminates when the guess is stable.
*/
int sub_15557(int n) // Sqrt(int n)
{
    int quotient;
    int guess;

    n = abs(n);

    if (n < 4)
    {
        return 1;
    }
    /* Initial guess = n / 4. The assembly uses two signed arithmetic shifts
       to the right, which is equivalent to a signed division by 4. */
    guess = n >> 2;

    /* This do-while loop structure perfectly matches the assembly's
       logic of executing the block once and then checking the condition
       with a 'jg' at the end to jump back to the top. */
    do
    {
        quotient = n / guess;
        guess = (guess + quotient) >> 1; /* (guess + quotient) / 2 */
    } while (abs(guess - quotient) > 1);

    return guess;
}

int Dist2D(int x, int y)
{
    //int min_val, max_val;
    long result;

    x = abs(x);
    y = abs(y);
// 0xcfc4
    result = (long)((y > x)?x:y) + ((long)((x >= y)?x:y) >> 1);
// 0xcff2
    if (result > 32767L) {
        result = 32767L;
    }

    return (int)result;
}

int ARCTAN(int y, int x)  /* assuming arg_0 = y, arg_2 = x based on special cases */  // D008
{
    int abs_y, abs_x, max_val, min_val, t, diff, flag;  // D008: var_E, var_C, var_A, var_4, var_2
    long a, coeff;  // D008: var_A (dword)
    int angle;  // D008: var_4

    if (y == 0) {  // D00F
        if (x > 0) {  // D015
            return 0x4000;  // D036
        } else {
            return 0xC000;  // D03E
        }
    }
    if (x == 0) {  // D02A
        if (y > 0) {  // D019
            return 0;  // D01B
        } else {
            return 0x8000;  // D022
        }
    }

    abs_y = abs(y);  // D046
    abs_x = abs(x);  // D04F

    if (abs_y > abs_x) {  // D05A
        max_val = abs_y;  // D0A6
        min_val = abs_x;  // D08F
        flag = 0;  // D0B2
    } else {
        max_val = abs_x;  // D05E
        min_val = abs_y;  // D0A9
        flag = 1;  // D084
    }

    t = (int)(((long)min_val << 14) / max_val);  // D06A-D0C3

    diff = abs(0x1333 - t);  // D0D5-D0D9

    a = ((long)0xB00 * diff) >> 14;  // D0CC-D0E2

    coeff = 0x2800 - a;  // D0EF

    angle = (int)((coeff * (long)t) >> 14);  // D0FE-D103

    if (y > 0) {  // D10E
        if (x > 0) {  // D114
            if (flag) {  // D11A
                return 0x4000 - angle;  // D120-D123
            } else {
                return angle;  // D128
            }
        } else {
            if (flag) {  // D12E
                return angle + 0xC000;  // D134
            } else {
                return -angle;  // D156
            }
        }
    } else {
        if (x > 0) {  // D142
            if (flag) {  // D148
                return angle + 0x4000;  // D16A
            } else {
                return 0x8000 - angle;  // D13C-D123
            }
        } else {
            if (flag) {  // D15E
                return 0xC000 - angle;  // D164-D123
            } else {
                return angle + 0x8000;  // D16A
            }
        }
    }
}
