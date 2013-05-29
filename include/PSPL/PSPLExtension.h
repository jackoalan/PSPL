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
 * @ingroup PSPL
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
    const struct _pspl_toolchain_extension* toolchain_extension; /**< Extension toolchain substructure */
#   endif
#   ifdef PSPL_RUNTIME
    const struct _pspl_runtime_extension* runtime_extension; /**< Extension runtime substructure */
#   endif
} pspl_extension_t;


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
    uint8_t byte_order; /**< Native byte-order [PSPL_LITTLE_ENDIAN, PSPL_BIG_ENDIAN] */
    uint8_t padding[3]; /**< Full-word padding */
#   ifdef PSPL_TOOLCHAIN
    const struct _pspl_toolchain_platform* toolchain_platform; /**< Platform toolchain substructure */
#   endif
#   ifdef PSPL_RUNTIME
    const struct _pspl_runtime_platform* runtime_platform; /**< Platform runtime substructure */
#   endif
} pspl_platform_t;

/** @} */

#include <PSPL/PSPL.h>



#endif
