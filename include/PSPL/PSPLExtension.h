//
//  PSPLExtension.h
//  PSPL
//
//  Created by Jack Andersen on 4/15/13.
//
//

#ifndef PSPL_PSPLExtension_h
#define PSPL_PSPLExtension_h

#include <stdint.h>

/* Common extension definition structure */
typedef struct _pspl_extension {
    // Unique name of extension
    const char* extension_name;
    
    // Description of extension (for built-in help)
    const char* extension_desc;
    
#   ifdef PSPL_TOOLCHAIN
    // Extension's toolchain extension definition object
    const struct _pspl_toolchain_extension* toolchain_extension;
#   endif
    
#   ifdef PSPL_RUNTIME
    // Extension's runtime extension definition object
    const struct _pspl_runtime_extension* runtime_extension;
#   endif
    
} pspl_extension_t;


/* Common platform description structure */
typedef struct _pspl_runtime_platform {
    
    // Short platform name
    const char* platform_name;
    
    // Platform description
    const char* platform_desc;
    
    // Native byte-order [PSPL_LITTLE_ENDIAN, PSPL_BIG_ENDIAN]
    uint8_t byte_order;
    
    // Padding
    uint8_t padding[3];
    
    // Platform toolchain structure
#   ifdef PSPL_TOOLCHAIN
    const struct _pspl_toolchain_platform* runtime_hooks;
#   endif
    
    // Platform runtime structure
#   ifdef PSPL_RUNTIME
    const struct _pspl_runtime_platform* runtime_hooks;
#   endif
    
    
} pspl_platform_t;


#include <PSPL/PSPL.h>
#include <PSPL/PSPLToolchainExtension.h>
#include <PSPL/PSPLRuntimeExtension.h>


#endif
