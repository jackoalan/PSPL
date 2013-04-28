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

/* Endianness enumerations */
#define PSPL_LITTLE_ENDIAN 1
#define PSPL_BIG_ENDIAN 2
#define PSPL_BI_ENDIAN 3

/* Runtime platform description structure */
typedef struct {
    
    // Short platform name
    const char* platform_name;
    
    // Platform description
    const char* platform_desc;
    
    // Native byte-order [PSPL_LITTLE_ENDIAN, PSPL_BIG_ENDIAN]
    uint8_t byte_order;
    
    // Padding
    uint8_t padding[3];
    
} pspl_runtime_platform_t;


#pragma mark Runtime Init

void PSPL_Init();


#endif
