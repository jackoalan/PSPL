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

/* Macros for producing bi-endian record declarations of each numeric 
 * type available to PSPL and its extensions. They are intended to facilitate
 * `psplc` file generation when multiple platforms with different byte-orderings 
 * are to be targeted.
 * Assignment macros are also available to perform the necessary swaps. */

/* It is a paradox to compile to a bi-endian target */
#if __BIG_ENDIAN__ && __LITTLE_ENDIAN__
#  error PSPL cannot compile to a target that is both big and little endian
#endif

/* We must have at least one type of target as well */
#if !__BIG_ENDIAN__ && !__LITTLE_ENDIAN__
#  error PSPL cannot compile to a target that is neither big or nor little endian
#endif


/* First, a means to declare a bi-endian structure type
 * from an already-declared type. */
#if __LITTLE_ENDIAN__

#define DECL_BI_OBJ_TYPE(source_type) struct {\
    union {\
        source_type little;\
        source_type native;\
    };\
    union {\
        source_type big;\
        source_type swapped;\
    };\
}

#elif __BIG_ENDIAN__

#define DECL_BI_OBJ_TYPE(source_type) struct {\
    union {\
        source_type little;\
        source_type swapped;\
    };\
    union {\
        source_type big;\
        source_type native;\
    };\
}

#endif



/* Now our swappers */

#define CAST(val,type) (*((type*)(&val)))


// Byte swap unsigned short
static uint16_t swap_uint16( uint16_t val )
{
    return (val << 8) | (val >> 8 );
}

// Byte swap short
static int16_t swap_int16( int16_t val )
{
    return (val << 8) | ((val >> 8) & 0xFF);
}

// Byte swap unsigned int
static uint32_t swap_uint32( uint32_t val )
{
    val = ((val << 8) & 0xFF00FF00 ) | ((val >> 8) & 0xFF00FF );
    return (val << 16) | (val >> 16);
}

// Byte swap int
static int32_t swap_int32( int32_t val )
{
    val = ((val << 8) & 0xFF00FF00) | ((val >> 8) & 0xFF00FF );
    return (val << 16) | ((val >> 16) & 0xFFFF);
}

// Byte swap 64 int
static int64_t swap_int64( int64_t val )
{
    val = ((val << 8) & 0xFF00FF00FF00FF00ULL ) | ((val >> 8) & 0x00FF00FF00FF00FFULL );
    val = ((val << 16) & 0xFFFF0000FFFF0000ULL ) | ((val >> 16) & 0x0000FFFF0000FFFFULL );
    return (val << 32) | ((val >> 32) & 0xFFFFFFFFULL);
}

// Byte swap unsigned 64 int
static uint64_t swap_uint64( uint64_t val )
{
    val = ((val << 8) & 0xFF00FF00FF00FF00ULL ) | ((val >> 8) & 0x00FF00FF00FF00FFULL );
    val = ((val << 16) & 0xFFFF0000FFFF0000ULL ) | ((val >> 16) & 0x0000FFFF0000FFFFULL );
    return (val << 32) | (val >> 32);
}

// Byte swap float
static float swap_float( float val )
{
    uint32_t uval = swap_uint32(CAST(val, uint32_t));
    return CAST(uval, float);
}

// Byte swap double
static double swap_double( double val )
{
    uint64_t uval = swap_uint64(CAST(val, uint64_t));
    return CAST(uval, double);
}




/* Now, some assignment convenience macros */

#define SET_BI_S16(record,field,val) record.native.field = val; record.swapped.field = swap_int16(val)
#define SET_BI_U16(record,field,val) record.native.field = val; record.swapped.field = swap_uint16(val)

#define SET_BI_S32(record,field,val) record.native.field = val; record.swapped.field = swap_int32(val)
#define SET_BI_U32(record,field,val) record.native.field = val; record.swapped.field = swap_uint32(val)

#define SET_BI_S64(record,field,val) record.native.field = val; record.swapped.field = swap_int64(val)
#define SET_BI_U64(record,field,val) record.native.field = val; record.swapped.field = swap_uint64(val)

#define SET_BI_FLOAT(record,field,val) record.native.field = val; record.swapped.field = swap_float(val)

#define SET_BI_DOUBLE(record,field,val) record.native.field = val; record.swapped.field = swap_double(val)


/* Now, an automagic one (using GCC-defined builtin type detector) */

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wconversion"

#define SET_BI(record,field,val)\
if(__builtin_types_compatible_p(typeof(record.native.field), int16_t))\
    {SET_BI_S16(record,field,val);}\
else if(__builtin_types_compatible_p(typeof(record.native.field), uint16_t))\
    {SET_BI_U16(record,field,val);}\
else if(__builtin_types_compatible_p(typeof(record.native.field), int32_t))\
    {SET_BI_S32(record,field,val);}\
else if(__builtin_types_compatible_p(typeof(record.native.field), uint32_t))\
    {SET_BI_U32(record,field,val);}\
else if(__builtin_types_compatible_p(typeof(record.native.field), int64_t))\
    {SET_BI_S64(record,field,val);}\
else if(__builtin_types_compatible_p(typeof(record.native.field), uint64_t))\
    {SET_BI_U64(record,field,val);}\
else if(__builtin_types_compatible_p(typeof(record.native.field), float))\
    {SET_BI_FLOAT(record,field,val);}\
else if(__builtin_types_compatible_p(typeof(record.native.field), double))\
    {SET_BI_DOUBLE(record,field,val);}\
else\
    fprintf(stderr, "Incompatible type set to using `SET_BI` in \"" __FILE__ "\":%u\n", __LINE__)

#pragma clang diagnostic pop



#endif // PSPL_PSPLValue_h


