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


/* Common object class structure 
 *
 * This structure is initialised by any extensions wishing to add*/
typedef struct _pspl_object_class {
    // 4-byte type
    uint8_t type[4];
    
    // Size of the entire object (sum of all inheriting structure sizes)
    DECL_BI_U32(size);
    
} pspl_object_t;


#endif
