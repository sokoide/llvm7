#ifndef LLVM7_STDINT_H
#define LLVM7_STDINT_H

#include <stddef.h>

typedef signed char int8_t;
typedef unsigned char uint8_t;
typedef short int16_t;
typedef unsigned short uint16_t;
typedef int int32_t;
typedef unsigned int uint32_t;
typedef long long int64_t;
typedef unsigned long long uint64_t;

#define INT8_MIN (-128)
#define INT8_MAX 127
#define UINT8_MAX 255u

#define INT16_MIN (-32768)
#define INT16_MAX 32767
#define UINT16_MAX 65535u

#define INT32_MIN (-2147483647 - 1)
#define INT32_MAX 2147483647
#define UINT32_MAX 4294967295u

#define INT64_MAX 9223372036854775807LL
#define INT64_MIN (-INT64_MAX - 1)
#define UINT64_MAX 18446744073709551615ULL

#endif /* LLVM7_STDINT_H */
