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

#pragma mark Runtime Platform Hook Types

typedef int(*pspl_runtime_platform_init_hook)();
typedef void(*pspl_runtime_platform_shutdown_hook)();
typedef void(*pspl_runtime_platform_load_shader_object_hook)(pspl_shader_object_t* shader);
typedef void(*pspl_runtime_platform_unload_shader_object_hook)(pspl_shader_object_t* shader);
typedef void(*pspl_runtime_platform_bind_shader_object_hook)(pspl_shader_object_t* shader);


#pragma mark Main Runtime Platform Structure (every platform needs one)

/* Main toolchain platform structure */
typedef struct _pspl_runtime_platform {
    
    // All fields are optional and may be set `NULL`
    
    // Hook fields
    
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
    
} pspl_runtime_platform_t;

#endif // PSPL_RUNTIME
#endif
