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

/**
 * @file PSPL/PSPLExtension.h
 * @brief Primary Extension and Platform Structures
 * @defgroup pspl_extension Objects and Routines for authoring PSPL extensions
 * @ingroup pspl_extension
 * @{
 */

/** 
 * Common extension definition structure
 *
 * Instances of this structure are generated before compile time
 * via CMake. The structure is shared between the *Toolchain* and
 * *Runtime* portions of PSPL, thereby maintaining a common extension
 * namespace.
 */
typedef struct _pspl_extension {
    const char* extension_name; /**< Unique extension name (short) */
    const char* extension_desc; /**< Description of extension (for built-in help) */
#   ifdef PSPL_TOOLCHAIN
    // Extension's toolchain extension definition object
    const struct _pspl_toolchain_extension* toolchain_extension;
#   endif
    
#   ifdef PSPL_RUNTIME
    // Extension's runtime extension definition object
    const struct _pspl_runtime_extension* runtime_extension;
#   endif
    
} pspl_extension_t;

/**
 * @}
 * @defgroup pspl_platform Objects and Routines for authoring PSPL platforms
 * @ingroup pspl_platform
 * @{
 */

/** 
 * Common platform description structure 
 *
 * Instances of this structure are generated before compile time
 * via CMake. The structure is shared between the *Toolchain* and
 * *Runtime* portions of PSPL, thereby maintaining a common platform
 * namespace.
 */
typedef struct _pspl_platform {
    const char* platform_name; /**< Unique platform name (short) */
    const char* platform_desc; /**< Description of platform (for built-in help) */
    
    // Native byte-order [PSPL_LITTLE_ENDIAN, PSPL_BIG_ENDIAN]
    uint8_t byte_order;
    
    // Padding
    uint8_t padding[3];
    
    // Platform toolchain structure
#   ifdef PSPL_TOOLCHAIN
    const struct _pspl_toolchain_platform* toolchain_platform;
#   endif
    
    // Platform runtime structure
#   ifdef PSPL_RUNTIME
    const struct _pspl_runtime_platform* runtime_platform;
#   endif
    
    
} pspl_platform_t;

/** @} */

#include <PSPL/PSPL.h>



#endif
