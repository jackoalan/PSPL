//
//  PSPLToolchainExtension.h
//  PSPL
//
//  Created by Jack Andersen on 4/15/13.
//
//

#ifndef PSPL_PSPLToolchainExtension_h
#define PSPL_PSPLToolchainExtension_h

#include <PSPL/PSPL.h>


/* First, our common object structure (classed with 4-byte typestring) */
typedef struct _pspl_object {
    // 4-byte type
    uint8_t type[4];
    
    // Size of the entire object (sum of all inheriting structure sizes)
    DECL_BI_U32(size);
    
} pspl_object_t;


#endif
