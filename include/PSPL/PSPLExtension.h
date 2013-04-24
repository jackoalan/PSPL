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
    
    // NULL-terminated array of composed `pspl_class_t` object classes
    const struct _pspl_class* class_array;
    
    #ifdef PSPL_TOOLCHAIN
    // Extension's toolchain extension definition object
    const struct _pspl_toolchain_extension* toolchain_extension;
    #endif
    
    #ifdef PSPL_RUNTIME
    // Extension's runtime extension definition object
    const struct _pspl_runtime_extension* runtime_extension;
    #endif
    
} pspl_extension_t;


/* Common object class structure 
 *
 * This structure is initialised by any extensions wishing to define 
 * an archivable C-structure storable within a PSPL package.
 */
typedef struct _pspl_class {
    // 4-byte class identifier (present in class instances for association)
    uint8_t class_id[4];
    
    // Pointer to extension defining class
    struct _pspl_toolchain_extension* class_extension;
    
    // Size of the entire object (sum of all inheriting structure sizes)
    uint32_t size;
    
} pspl_class_t;

#include <PSPL/PSPL.h>
#ifdef PSPL_TOOLCHAIN
#include <PSPL/PSPLToolchainExtension.h>
#endif
#ifdef PSPL_RUNTIME
#include <PSPL/PSPLRuntimeExtension.h>
#endif


#endif
