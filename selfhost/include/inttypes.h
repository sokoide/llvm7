#ifndef LLVM7_INTTYPES_H
#define LLVM7_INTTYPES_H

#include <stdint.h>

#define INT8_C(val) (val)
#define UINT8_C(val) (val##U)
#define INT16_C(val) (val)
#define UINT16_C(val) (val##U)
#define INT32_C(val) (val)
#define UINT32_C(val) (val##U)
#define INT64_C(val) (val##LL)
#define UINT64_C(val) (val##ULL)

#endif /* LLVM7_INTTYPES_H */
