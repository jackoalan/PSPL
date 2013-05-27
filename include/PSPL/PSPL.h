//
//  PSPL.h
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

/* Endianness enumerations */
#define PSPL_UNSPEC_ENDIAN 0
#define PSPL_LITTLE_ENDIAN 1
#define PSPL_BIG_ENDIAN    2
#define PSPL_BI_ENDIAN     3

/* PSPL stored hash type */
typedef struct {
    uint8_t hash[20];
} pspl_hash;
static inline int pspl_hash_cmp(const pspl_hash* a, const pspl_hash* b) {
    return memcmp(a, b, sizeof(pspl_hash));
}
static inline void pspl_hash_cpy(pspl_hash* dest, const pspl_hash* src) {
    memcpy(dest, src, sizeof(pspl_hash));
}
#define PSPL_HASH_STRING_LEN 41
extern void pspl_hash_fmt(char* out, const pspl_hash* hash);
extern void pspl_hash_parse(pspl_hash* out, const char* hash_str);

/* Runtime-only structures */
#ifdef PSPL_RUNTIME

/* Shader runtime object (from PSPLC) 
 * Holds state information about object during runtime */
typedef struct {
    
    // Hash of PSPLC object
    pspl_hash hash;
    
    // Platform-specific shader object
    // (initialised by load hook, freed by unload hook, bound by bind hook)
    pspl_platform_shader_object_t native_shader;
    
    // Opaque object pointer used by PSPL's runtime internals
    const void* pspl_shader_internals;
    
} pspl_shader_object_t;

#endif

#endif
