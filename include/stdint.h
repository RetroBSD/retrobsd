
#ifndef _STDINT_H
#define _STDINT_H

typedef signed char         int8_t;
typedef short int           int16_t;
typedef int                 int32_t;
typedef long long           int64_t;

typedef unsigned char       uint8_t;
typedef unsigned short int  uint16_t;
typedef unsigned int        uint32_t;
typedef unsigned long long  uint64_t;

#define INT8_C(x)	x
#define UINT8_C(x)	x##U

#define INT16_C(x)	x
#define UINT16_C(x)	x##U

#define INT32_C(x)	x
#define UINT32_C(x)     x##U

#define INT64_C(x)	x##LL
#define UINT64_C(x)     x##ULL

#define INTMAX_C(x)	x##LL
#define UINTMAX_C(x)	x##ULL

#endif
