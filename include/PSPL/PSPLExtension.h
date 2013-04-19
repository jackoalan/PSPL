//
//  PSPLExtension.h
//  PSPL
//
//  Created by Jack Andersen on 4/15/13.
//
//

#ifndef PSPL_PSPLExtension_h
#define PSPL_PSPLExtension_h


#include <PSPL/PSPL.h>
#include <PSPL/Toolchain/PSPLToolchainExtension.h>
#include <PSPL/Runtime/PSPLRuntimeExtension.h>


/* Forward decl of class struct type */
typedef struct _pspl_class pspl_class_t;


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
    const pspl_class_t class_array[];
    
    // NULL-terminated array of composed `pspl_toolchain_extension_t`
    // (NULL if accessed from runtime)
    const pspl_toolchain_extension_t toolchain_extension_array[];
    
    // NULL-terminated array of composed `pspl_runtime_extension_t`
    // (NULL if accessed from toolchain)
    const pspl_runtime_extension_t runtime_extension_array[];
    
} pspl_extension_t;


/* Common object class structure 
 *
 * This structure is initialised by any extensions wishing to define 
 * an archivable C-structure storable within a PSPL package.
 */
struct _pspl_class {
    // 4-byte class identifier (present in class instances for association)
    uint8_t class_id[4];
    
    // Pointer to extension defining class
    _pspl_extension_t* class_extension;
    
    // Size of the entire object (sum of all inheriting structure sizes)
    uint32_t size;
    
};


#endif
