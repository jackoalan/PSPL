//
//  PSPLRuntimeExtension.h
//  PSPL
//
//  Created by Jack Andersen on 4/15/13.
//
//

#ifndef PSPL_PSPLRuntimeExtension_h
#define PSPL_PSPLRuntimeExtension_h
#ifdef PSPL_RUNTIME

#include <PSPL/PSPLExtension.h>

#pragma mark Hook Types

/* Embedded data object type */
typedef struct {
    size_t object_len;
    const void* object_data;
} pspl_data_object_t;

typedef void(*pspl_runtime_shutdown_hook)();
typedef void(*pspl_runtime_load_object_hook)(psplc_object_t* object);
typedef void(*pspl_runtime_unload_object_hook)(psplc_object_t* object);
typedef void(*pspl_runtime_bind_object_hook)(psplc_object_t* object);


#pragma mark Extension/Platform Load API

/* These *must* be called within the `load` hook of platform or extension */

/* Get embedded data object for extension by key (to hash) */
const pspl_data_object_t* pspl_get_embedded_data_object_from_key(psplc_object_t* object,
                                                                 const char* key);

/* Get embedded object for extension by hash */
const pspl_data_object_t* pspl_get_embedded_data_object_from_hash(psplc_object_t* object,
                                                                  pspl_hash* hash);

/* Get embedded object for extension by index */
const pspl_data_object_t* pspl_get_embedded_data_object_from_index(psplc_object_t* object,
                                                                   uint32_t index);


#pragma mark Main Runtime Extension Structure

/* Main toolchain platform structure */
typedef struct _pspl_runtime_extension {
    
    // All fields are optional and may be set `NULL`
    
    // Hook fields
    
    // Extension init (called once at runtime init)
    void(*init_hook)(const pspl_extension_t* extension);
    
    // Shutdown (called once at runtime shutdown)
    pspl_runtime_shutdown_hook shutdown_hook;
    
    // Object load
    pspl_runtime_load_object_hook load_object_hook;
    
    // Object unload
    pspl_runtime_unload_object_hook unload_object_hook;
    
    // Object bind
    pspl_runtime_bind_object_hook bind_object_hook;
    
} pspl_runtime_extension_t;



#pragma mark Main Runtime Platform Structure (every platform needs one)

/* Main toolchain platform structure */
typedef struct _pspl_runtime_platform {
    
    // All fields are optional and may be set `NULL`
    
    // Hook fields
    
    // Platform init (called once at runtime init)
    void(*init_hook)(const pspl_platform_t* platform);
    
    // Shutdown (called once at runtime shutdown)
    pspl_runtime_shutdown_hook shutdown_hook;
    
    // Object load (platform implementation needs to initialise `native_shader`)
    pspl_runtime_load_object_hook load_object_hook;
    
    // Object unload (platform implementation needs to free `native_shader`)
    pspl_runtime_unload_object_hook unload_object_hook;
    
    // Object bind
    // (platform implementation needs to set GPU into using specified shader object)
    pspl_runtime_bind_object_hook bind_object_hook;
    
} pspl_runtime_platform_t;

#endif // PSPL_RUNTIME
#endif
