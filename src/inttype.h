#ifndef INTTYPE_H
#define INTTYPE_H

/* 16-bit DOS (MSC 5.1): long = 4, int = 2, near ptr = 2, far ptr = 4 */
#if defined(MSDOS)
typedef unsigned long uint32;
typedef unsigned int uint16;
typedef unsigned char uint8;

typedef signed long int32;
typedef signed int int16;
typedef signed char int8;

typedef int bool;
#define NEAR near
#define FAR far
#define true 1
#define false 0
#else
#include <stdint.h>
typedef uint32_t uint32;
typedef uint16_t uint16;
typedef uint8_t uint8;
typedef int32_t int32;
typedef int16_t int16;
typedef int8_t int8;
typedef int bool;
#define NEAR near
#define FAR far
#define true 1
#define false 0
#endif

#endif
