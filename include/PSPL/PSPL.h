//
//  PSPL.h
//  PSPL
//
//  Created by Jack Andersen on 4/15/13.
//
//

#ifndef PSPL_PSPL_h
#define PSPL_PSPL_h

/* Our method of defining numeric values in a bi-endian manner */
#include <PSPL/PSPLValue.h>



/* First, our common object structure (classed with 4-byte typestring) */
typedef struct _pspl_object {
    // 4-byte type
    uint8_t type[4];
    
    // Size of the entire object (sum of all inheriting structure sizes)
    DECL_BI_U32(size);
    
} pspl_object_t;


#pragma mark Runtime Init

void PSPL_Init();


#pragma mark


#endif