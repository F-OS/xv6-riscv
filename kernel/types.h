#ifndef KERNEL_TYPES_H
#define KERNEL_TYPES_H

typedef unsigned int uint;
typedef unsigned short ushort;
typedef unsigned char uchar;

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;
typedef unsigned long uint64;

typedef uint64 pde_t;

#define NELEM(x) (sizeof(x) / sizeof((x)[0]))
#define NULL ((void *)0)

typedef _Bool bool;
#define true 1
#define false 0

#endif // KERNEL_TYPES_H