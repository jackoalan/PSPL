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


#pragma mark Runtime Platform Hook Types



#pragma mark Main Runtime Platform Structure (every platform needs one)

/* Main toolchain platform structure */
typedef struct _pspl_runtime_platform {
    
    // All fields are optional and may be set `NULL`
    
    // Hook fields
    //pspl_toolchain_init_hook init_hook;
    //pspl_toolchain_platform_receive_hook receive_hook;
    //pspl_toolchain_platform_generate_hook generate_hook;
    
} pspl_runtime_platform_t;


#endif // PSPL_RUNTIME
#endif
