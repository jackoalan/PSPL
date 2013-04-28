//
//  PSPLExtension.h
//  PSPL
//
//  Created by Jack Andersen on 4/15/13.
//
//

#ifndef PSPL_PSPLExtension_h
#define PSPL_PSPLExtension_h


/* Common extension definition structure
 * 
 * This structure provides a means to compose object classes,
 * toolchain extensions and runtime extensions together into a
 * common namespace. This namespace persists between the offline
 * toolchain and within the runtime later.
 */
typedef struct _pspl_extension {
    // Unique name of extension
    const char* extension_name;
    
    // Description of extension (for built-in help)
    const char* extension_desc;
    
    #ifdef PSPL_TOOLCHAIN
    // Extension's toolchain extension definition object
    const struct _pspl_toolchain_extension* toolchain_extension;
    #endif
    
    #ifdef PSPL_RUNTIME
    // Extension's runtime extension definition object
    const struct _pspl_runtime_extension* runtime_extension;
    #endif
    
} pspl_extension_t;


#include <PSPL/PSPL.h>
#include <PSPL/PSPLToolchainExtension.h>
#include <PSPL/PSPLRuntimeExtension.h>


#endif
