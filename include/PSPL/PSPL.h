//
//  PSPL.h
//  PSPL
//
//  Created by Jack Andersen on 4/15/13.
//
//

#ifndef PSPL_PSPL_h
#define PSPL_PSPL_h

/* PSPL's method of defining numeric values in a bi-endian manner */
#include <PSPL/PSPLValue.h>

/* Endianness enumerations */
#define PSPL_LITTLE_ENDIAN 1
#define PSPL_BIG_ENDIAN 2
#define PSPL_BI_ENDIAN 3

/* Runtime-only structures */
#define PSPL_RUNTIME
#ifdef PSPL_RUNTIME

/* Shader runtime object (from PSPLC) 
 * Holds state information about object during runtime */
typedef struct {
    
    
    
} pspl_runtime_shader_object_t;


/* Hook function types */
struct _pspl_runtime_platform;
typedef int(*pspl_runtime_platform_init_hook)(struct _pspl_runtime_platform* platform);
typedef void(*pspl_runtime_platform_shutdown_hook)(struct _pspl_runtime_platform* platform);
typedef void(*pspl_runtime_platform_load_shader_object_hook);


/* Hooks structure (implemented by platform extension)
 * containing function references to set up platform runtime 
 * and load shader objects */
typedef struct {
    
    // Platform init (called once at runtime init)
    
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
    #ifdef PSPL_RUNTIME
    pspl_runtime_platform_hooks_t* runtime_hooks;
    #endif
    
    
} pspl_runtime_platform_t;



#endif
