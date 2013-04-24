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


/* Common object structure (classed with 4-byte typestring) 
 * This data structure is present within the `psplb` file and 
 * when loaded into RAM */
typedef struct {
    // 4-byte object class identifier
    uint8_t class_id[4];
    
    // Size of the entire object (sum of all inheriting structure sizes)
    DECL_BI_STRUCT(uint32_t) size;
    
    
    
} pspl_object_t;


/* Runtime platform description structure */
typedef struct {
    // Runtime name
    
} pspl_runtime_platform_t;


#pragma mark Runtime Init

void PSPL_Init();


#endif
