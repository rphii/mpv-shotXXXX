#ifndef VECTOR_H

#include "vec.h"

#include <stdint.h>

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t  i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef u8 V3u8[3];

#if 0
VEC_INCLUDE(Vu8,  vu8,  uint8_t,  BY_VAL);
VEC_INCLUDE(Vu16, vu16, uint16_t, BY_VAL);
VEC_INCLUDE(Vu32, vu32, uint32_t, BY_VAL);
VEC_INCLUDE(Vu64, vu64, uint64_t, BY_VAL);
#endif

typedef struct Str Str;
VEC_INCLUDE(VStr, vstr, Str, BY_REF, BASE);
VEC_INCLUDE(VrStr, vrstr, Str, BY_REF, BASE);

void vrstr_sort(VrStr *vec, size_t *counts);

#define VECTOR_H
#endif

