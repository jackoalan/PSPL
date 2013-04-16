//
//  PSPLValue.h
//  PSPL
//
//  Created by Jack Andersen on 4/15/13.
//
//

#ifndef PSPL_PSPLValue_h
#define PSPL_PSPLValue_h

#include <stdint.h>

/* Macros for producing bi-endian value declarations of each numeric 
 * type available to PSPL and its extensions. Also includes assignment
 * macros to perform the necessary swap.
 *
 * The DECL_* macros are designed to be used in structures so that the
 * little-endian version is always declared just before the big-endian version.
 * The DECL_* macros also include an optional additional macro to set declaration
 * keywords (static, extern, const, etc...) */

/* It is a paradox to compile to a bi-endian target */
#if __BIG_ENDIAN__ && __LITTLE_ENDIAN__
#  error PSPL cannot compile to a target that is both big and little endian
#endif

/* We must have at least one type of target as well */
#if !__BIG_ENDIAN__ && !__LITTLE_ENDIAN__
#  error PSPL cannot compile to a target that is neither big or nor little endian
#endif



/* First, our swappers */

#define CAST(val,type) (*((type##*)(&val)))

#define SWAP_16(val) ((CAST(val,uint16_t) << 8) | (CAST(val,uint16_t) >> 8))

#define SWAP_32_PRE(val) (((CAST(val,uint32_t) << 8) & 0xFF00FF00 ) | ((CAST(val,uint32_t) >> 8) & 0xFF00FF))
#define SWAP_32(val) ((SWAP_32_PRE(val) << 16) | (SWAP_32_PRE(val) >> 16))

#define SWAP_64_PRE1(val) (((CAST(val,uint64_t) << 8) & 0xFF00FF00FF00FF00ULL) | ((CAST(val,uint64_t) >> 8) & 0x00FF00FF00FF00FFULL))
#define SWAP_64_PRE2(val) (((SWAP_64_PRE1(val) << 16) & 0xFFFF0000FFFF0000ULL) | ((SWAP_64_PRE1(val) >> 16) & 0x0000FFFF0000FFFFULL))
#define SWAP_64(val) ((SWAP_64_PRE2(val) << 32) | (SWAP_64_PRE2(val) >> 32))



/* 16-bit signed */

#if __LITTLE_ENDIAN__
#  define DECL_BI_S16(name,...) __VA_ARGS__ int16_t name; __VA_ARGS__ int16_t name##_swap;
#  define DECL_BI_S16_SET(name,val,...) __VA_ARGS__ int16_t name = val; __VA_ARGS__ int16_t name##_swap = CAST(SWAP_16(val),int16_t);
#elif __BIG_ENDIAN__
#  define DECL_BI_S16(name,...) __VA_ARGS__ int16_t name##_swap; __VA_ARGS__ int16_t name;
#  define DECL_BI_S16_SET(name,val,...) __VA_ARGS__ int16_t name##_swap = CAST(SWAP_16(val),int16_t); __VA_ARGS__ int16_t name = val;
#endif


/* 16-bit unsigned */

#if __LITTLE_ENDIAN__
#  define DECL_BI_U16(name,...) uint16_t name; uint16_t name##_swap;
#  define DECL_BI_U16_SET(name,val,...) uint16_t name = val; uint16_t name##_swap = SWAP_16(val);
#elif __BIG_ENDIAN__
#  define DECL_BI_U16(name,...) uint16_t name##_swap; uint16_t name;
#  define DECL_BI_U16_SET(name,val,...) uint16_t name##_swap = SWAP_16(val); uint16_t name = val;
#endif



/* 32-bit signed */

#if __LITTLE_ENDIAN__
#  define DECL_BI_S32(name,...) __VA_ARGS__ int32_t name; __VA_ARGS__ int32_t name##_swap;
#  define DECL_BI_S32_SET(name,val,...) __VA_ARGS__ int32_t name = val; __VA_ARGS__ int32_t name##_swap = CAST(SWAP_32(val),int32_t);
#elif __BIG_ENDIAN__
#  define DECL_BI_S32(name,...) __VA_ARGS__ int32_t name##_swap; __VA_ARGS__ int32_t name;
#  define DECL_BI_S32_SET(name,val,...) __VA_ARGS__ int32_t name##_swap = CAST(SWAP_32(val),int32_t); __VA_ARGS__ int32_t name = val;
#endif


/* 32-bit unsigned */

#if __LITTLE_ENDIAN__
#  define DECL_BI_U32(name,...) __VA_ARGS__ uint32_t name; __VA_ARGS__ uint32_t name##_swap;
#  define DECL_BI_U32_SET(name,val,...) __VA_ARGS__ uint32_t name = val; __VA_ARGS__ uint32_t name##_swap = SWAP_32(val);
#elif __BIG_ENDIAN__
#  define DECL_BI_U32(name,...) __VA_ARGS__ uint32_t name##_swap; __VA_ARGS__ uint32_t name;
#  define DECL_BI_U32_SET(name,val,...) __VA_ARGS__ uint32_t name##_swap = SWAP_32(val); __VA_ARGS__ uint32_t name = val;
#endif



/* 64-bit signed */

#if __LITTLE_ENDIAN__
#  define DECL_BI_S64(name,...) __VA_ARGS__ int64_t name; __VA_ARGS__ int64_t name##_swap;
#  define DECL_BI_S64_SET(name,val,...) __VA_ARGS__ int64_t name = val; __VA_ARGS__ int64_t name##_swap = CAST(SWAP_64(val),int64_t);
#elif __BIG_ENDIAN__
#  define DECL_BI_S64(name,...) __VA_ARGS__ int64_t name##_swap; __VA_ARGS__ int64_t name;
#  define DECL_BI_S64_SET(name,val,...) __VA_ARGS__ int64_t name##_swap = CAST(SWAP_64(val),int64_t); __VA_ARGS__ int64_t name = val;
#endif


/* 64-bit unsigned */

#if __LITTLE_ENDIAN__
#  define DECL_BI_U64(name,...) __VA_ARGS__ uint64_t name; __VA_ARGS__ uint64_t name##_swap;
#  define DECL_BI_U64_SET(name,val,...) __VA_ARGS__ uint64_t name = val; __VA_ARGS__ uint64_t name##_swap = SWAP_64(val);
#elif __BIG_ENDIAN__
#  define DECL_BI_U64(name,...) __VA_ARGS__ uint64_t name##_swap; __VA_ARGS__ uint64_t name;
#  define DECL_BI_U64_SET(name,val,...) __VA_ARGS__ uint64_t name##_swap = SWAP_64(val); __VA_ARGS__ uint64_t name = val;
#endif




/* Single-precision float */

#if __LITTLE_ENDIAN__
#  define DECL_BI_FLOAT(name,...) __VA_ARGS__ float name; __VA_ARGS__ float name##_swap;
#  define DECL_BI_FLOAT_SET(name,val,...) __VA_ARGS__ float name = val; __VA_ARGS__ float name##_swap = CAST(SWAP_32(val),float);
#elif __BIG_ENDIAN__
#  define DECL_BI_FLOAT(name,...) __VA_ARGS__ float name##_swap; __VA_ARGS__ float name;
#  define DECL_BI_FLOAT_SET(name,val,...) __VA_ARGS__ float name##_swap = CAST(SWAP_32(val),float); __VA_ARGS__ float name = val;
#endif




/* Double-precision float */

#if __LITTLE_ENDIAN__
#  define DECL_BI_DOUBLE(name,...) __VA_ARGS__ double name; __VA_ARGS__ double name##_swap;
#  define DECL_BI_DOUBLE_SET(name,val,...) __VA_ARGS__ double name = val; __VA_ARGS__ double name##_swap = CAST(SWAP_64(val),double);
#elif __BIG_ENDIAN__
#  define DECL_BI_DOUBLE(name,...) __VA_ARGS__ double name##_swap; __VA_ARGS__ double name;
#  define DECL_BI_DOUBLE_SET(name,val,...) __VA_ARGS__ double name##_swap = CAST(SWAP_64(val),double); __VA_ARGS__ double name = val;
#endif




/* Now, some assignment convenience macros */

#define SET_BI_S16(name,val) name = val; name##_swap = CAST(SWAP_16(val),int16_t);
#define SET_BI_U16(name,val) name = val; name##_swap = SWAP_16(val);

#define SET_BI_S32(name,val) name = val; name##_swap = CAST(SWAP_32(val),int32_t);
#define SET_BI_U32(name,val) name = val; name##_swap = SWAP_32(val);

#define SET_BI_S64(name,val) name = val; name##_swap = CAST(SWAP_64(val),int64_t);
#define SET_BI_U64(name,val) name = val; name##_swap = SWAP_64(val);

#define SET_BI_FLOAT(name,val) name = val; name##_swap = CAST(SWAP_32(val),float);

#define SET_BI_DOUBLE(name,val) name = val; name##_swap = CAST(SWAP_64(val),double);




#endif // PSPL_PSPLValue_h


