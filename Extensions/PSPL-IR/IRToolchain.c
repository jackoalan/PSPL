//
//  IRToolchain.c
//  PSPL
//
//  Created by Jack Andersen on 6/9/13.
//
//

#include <stdio.h>
#include <PSPLPlatform.h>

/* Calculation chain-link type */
typedef struct {
    
    // Link data type
    enum CHAIN_LINK_TYPE link_type;
    
    // How the link value is obtained
    enum CHAIN_LINK_USE link_use;
    
    // Value
    union CHAIN_LINK_VALUE link_value;
    
    // Cached pre-transform value
    union CHAIN_LINK_VALUE pre_link_value;
    
} pspl_calc_link_t;


