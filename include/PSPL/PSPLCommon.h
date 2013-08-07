//
//  PSPLCommon.h
//  PSPL
//
//  Created by Jack Andersen on 4/15/13.
//
//

#ifndef PSPL_PSPL_h
#define PSPL_PSPL_h

#include <string.h>

/* PSPL's method of defining numeric values in a bi-endian manner */
#include <PSPL/PSPLValue.h>

/* Round up to nearest 4 multiple */
#define ROUND_UP_4(val) (((val)%4)?((((val)>>2)<<2)+4):(val))

/* Round up to nearest 32 multiple */
#define ROUND_UP_32(val) (((val)%32)?((((val)>>5)<<5)+32):(val))

/* Feature enable state enum */
enum pspl_feature {
    PLATFORM = 0,
    DISABLED = 1,
    ENABLED  = 2
};

/* Blend factor enum */
enum pspl_blend_factor {
    SRC_COLOUR = 0,
    DST_COLOUR = 1,
    SRC_ALPHA  = 2,
    DST_ALPHA  = 3,
    ONE_MINUS  = 4
};

/* Common 4-component vector representation */
typedef float pspl_vector4_t[4];

/* Common 3-component vector representation */
typedef float pspl_vector3_t[3];

/* Common 3x4 matrix (homogenous) representation; row major */
typedef float pspl_matrix34_t[3][4];

/* Common 4x4 matrix representation; row major */
typedef float pspl_matrix44_t[4][4];

/* Common perspective/orthographic transform representation */
#undef near
#undef far
typedef struct {
    float fov;
    float aspect;
    float near;
    float far;
    float post_translate_x;
    float post_translate_y;
} pspl_perspective_t;
typedef struct {
    float top;
    float bottom;
    float left;
    float right;
    float near;
    float far;
    float post_translate_x;
    float post_translate_y;
} pspl_orthographic_t;

/* Common camera view representation - 3 orthogonal vectors */
typedef struct {
    pspl_vector3_t pos, look, up;
} pspl_camera_view_t;

/* Common rotation representation */
typedef struct {
    pspl_vector3_t axis;
    float angle;
} pspl_rotation_t;

/* Common 8-bit-per-channel colour representation */
typedef struct {
    double r,g,b,a;
} pspl_colour_t;

/**
 * @file PSPL/PSPLCommon.h
 * @brief General Toolchain *and* Runtime Public API Bits
 * @defgroup pspl Common APIs
 * @defgroup pspl_malloc Memory management context
 * @defgroup pspl_hash Hash Manipulation Routines
 * @ingroup PSPL
 * @{
 */

/**
 * Endianness enumerations
 */
enum pspl_endianness {
    PSPL_UNSPEC_ENDIAN = 0,
    PSPL_LITTLE_ENDIAN = 1,
    PSPL_BIG_ENDIAN    = 2,
    PSPL_BI_ENDIAN     = 3
};

#pragma mark Error Reporting

#ifdef __clang__
void pspl_error(int exit_code, const char* brief, const char* msg, ...) __printflike(3, 4);
void pspl_warn(const char* brief, const char* msg, ...) __printflike(2, 3);
#else
/**
 * Raise globally-recognised error condition (terminating the entire program)
 *
 * @param exit_code Error code to use with `exit`
 * @param brief Short string briefly describing error
 * @param msg Format string elaborating error details
 */
void pspl_error(int exit_code, const char* brief, const char* msg, ...);
/**
 * Raise globally-recognised warning
 *
 * @param brief Short string briefly describing warning
 * @param msg Format string elaborating warning details
 */
void pspl_warn(const char* brief, const char* msg, ...);
#endif


#pragma mark Malloc Zone Context

/**
 * Malloc context type 
 */
/* Malloc context */
typedef struct _malloc_context {
    unsigned int object_num;
    unsigned int object_cap;
    void** object_arr;
} pspl_malloc_context_t;

/**
 * Init Malloc Context for tracking allocated objects
 *
 * @param context Context object to populate
 */
void pspl_malloc_context_init(pspl_malloc_context_t* context);

/**
 * Destroy Malloc Context and free tracked memory objects with it
 *
 * @param context Context object to destroy
 */
void pspl_malloc_context_destroy(pspl_malloc_context_t* context);

/**
 * Allocate memory object and track within context
 *
 * @param context Context object to add memory object within
 * @return Newly-allocated memory object pointer
 */
void* pspl_malloc_malloc(pspl_malloc_context_t* context, size_t size);

#if PSPL_RUNTIME_PLATFORM_GX
void* pspl_malloc_memalign(pspl_malloc_context_t* context, size_t size, size_t align);
#endif

/**
 * Free memory object from within context
 *
 * @param context Context object to free memory object from
 * @param ptr Previously allocated memory object
 */
void pspl_malloc_free(pspl_malloc_context_t* context, void* ptr);


#pragma mark Hash Stuff

/**
 * PSPL stored hash type 
 */
typedef union {
    uint8_t b[20];
    uint32_t w[5];
} pspl_hash;

/**
 * Compare the contents of two hash objects
 *
 * @param a First Hash
 * @param b Second Hash
 * @return 0 if identical, positive or negative otherwise
 */
static inline int pspl_hash_cmp(const pspl_hash* a, const pspl_hash* b) {
    int j;
    for (j=0 ; j<(sizeof(pspl_hash)/4) ; ++j)
        if (a->w[j] != b->w[j])
            return -1;
    return 0;
}

/**
 * Copy hash to another location in memory
 *
 * @param dest Hash location to copy to
 * @param src Hash location to copy from
 */
static inline void pspl_hash_cpy(pspl_hash* dest, const pspl_hash* src) {
    int j;
    for (j=0 ; j<(sizeof(pspl_hash)/4) ; ++j)
        dest->w[j] = src->w[j];
}

/**
 * Length of string-buffer filled by `pspl_hash_fmt`
 */
#define PSPL_HASH_STRING_LEN 41

/**
 * Write hash data as string
 *
 * @param out String-buffer to fill (allocated to length of `PSPL_HASH_STRING_LEN`)
 * @param hash Hash object to be read
 */
extern void pspl_hash_fmt(char* out, const pspl_hash* hash);

/**
 * Parse hash string as hash object
 *
 * @param hash Hash object to be populated
 * @param out String-buffer containing hash as generated by `pspl_hash_fmt`
 */
extern void pspl_hash_parse(pspl_hash* out, const char* hash_str);

/**
 * Common extension definition structure
 *
 * Instances of this structure are generated before compile time
 * via CMake. The structure is shared between the *Toolchain* and
 * *Runtime* portions of PSPL, thereby maintaining a common extension
 * namespace.
 */
typedef struct _pspl_extension {
    const char* extension_name; /**< Unique extension name (short) */
    const char* extension_desc; /**< Description of extension (for built-in help) */
#   ifdef PSPL_TOOLCHAIN
    /**< Extension toolchain substructure, populated by platform author, named `<EXTNAME>_toolext` */
    const struct _pspl_toolchain_extension* toolchain_extension;
#   endif
#   ifdef PSPL_RUNTIME
    /**< Extension runtime substructure, populated by platform author, named `<EXTNAME>_runext` */
    const struct _pspl_runtime_extension* runtime_extension;
    /**< Extension index; assigned by CMake, used for extension-table lookups */
    int runtime_extension_index;
#   endif
} pspl_extension_t;


/**
 * Common platform description structure
 *
 * Instances of this structure are generated before compile time
 * via CMake. The structure is shared between the *Toolchain* and
 * *Runtime* portions of PSPL, thereby maintaining a common platform
 * namespace.
 */
typedef struct _pspl_platform {
    const char* platform_name; /**< Unique platform name (short) */
    const char* platform_desc; /**< Description of platform (for built-in help) */
    uint8_t byte_order; /**< Native byte-order [PSPL_UNSPEC_ENDIAN, PSPL_LITTLE_ENDIAN, PSPL_BIG_ENDIAN] */
    uint8_t padding[3]; /**< Full-word padding */
#   ifdef PSPL_TOOLCHAIN
    /**< Platform toolchain substructure, populated by platform author, named `<PLATNAME>_toolplat` */
    const struct _pspl_toolchain_platform* toolchain_platform;
#   endif
#   ifdef PSPL_RUNTIME
    /**< Platform runtime substructure, populated by platform author, named `<PLATNAME>_runplat` */
    const struct _pspl_runtime_platform* runtime_platform;
#   endif
} pspl_platform_t;


/* Windows Support */
#ifdef _WIN32

char* strtok_r(char *str,
               const char *delim,
               char **nextp);

#endif

/** @} */


#endif
