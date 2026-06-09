/* THUOS — freestanding fixed-width types.
 * The kernel does not link against a host libc, so we define our own
 * minimal type vocabulary here instead of pulling in <stdint.h>. */
#ifndef THUOS_TYPES_H
#define THUOS_TYPES_H

typedef unsigned char      uint8_t;
typedef signed char        int8_t;
typedef unsigned short     uint16_t;
typedef signed short       int16_t;
typedef unsigned int       uint32_t;
typedef signed int         int32_t;
typedef unsigned long long uint64_t;
typedef signed long long   int64_t;

/* 32-bit kernel: pointers and sizes are 32-bit wide. */
typedef uint32_t           size_t;
typedef uint32_t           uintptr_t;

#ifndef NULL
#define NULL ((void *)0)
#endif

typedef enum { false = 0, true = 1 } bool;

#endif /* THUOS_TYPES_H */
