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
    pspl_runtime_platform_shader_object_t native_shader;
    
    // Opaque object pointer used by PSPL's runtime internals
    const void* pspl_shader_internals;
    
} pspl_runtime_shader_object_t;


/* Hook function types */
struct _pspl_runtime_platform;
typedef int(*pspl_runtime_platform_init_hook)();
typedef void(*pspl_runtime_platform_shutdown_hook)();
typedef void(*pspl_runtime_platform_load_shader_object_hook)(pspl_runtime_shader_object_t* shader);
typedef void(*pspl_runtime_platform_unload_shader_object_hook)(pspl_runtime_shader_object_t* shader);
typedef void(*pspl_runtime_platform_bind_shader_object_hook)(pspl_runtime_shader_object_t* shader);

/* Hooks structure (implemented by platform extension)
 * containing function references to set up platform runtime 
 * and load shader objects */
typedef struct {
    
    // Platform init (called once at runtime init)
    pspl_runtime_platform_init_hook init_hook;
    
    // Shutdown (called once at runtime shutdown)
    pspl_runtime_platform_shutdown_hook shutdown_hook;
    
    // Shader object load (platform implementation needs to initialise `native_shader`)
    pspl_runtime_platform_load_shader_object_hook load_shader_object_hook;
    
    // Shader object unload (platform implementation needs to free `native_shader`)
    pspl_runtime_platform_unload_shader_object_hook unload_shader_object_hook;
    
    // Shader object bind
    // (platform implementation needs to set GPU into using specified shader object)
    pspl_runtime_platform_bind_shader_object_hook bind_shader_object_hook;
    
} pspl_runtime_platform_hooks_t;

#endif

/* Runtime platform description structure */
typedef struct _pspl_runtime_platform {
    
    // Short platform name
    const char* platform_name;
    
    // Platform description
    const char* platform_desc;
    
    // Native byte-order [PSPL_LITTLE_ENDIAN, PSPL_BIG_ENDIAN]
    uint8_t byte_order;
    
    // Padding
    uint8_t padding[3];
    
    // Platform runtime hooks
#   ifdef PSPL_RUNTIME
    pspl_runtime_platform_hooks_t* runtime_hooks;
#   endif
    
    
} pspl_runtime_platform_t;



#endif
