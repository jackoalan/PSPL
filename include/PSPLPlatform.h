//
//  PSPLPlatform.h
//  PSPL
//
//  Created by Jack Andersen on 6/9/13.
//
//

#ifndef PSPL_PSPLPlatform_h
#define PSPL_PSPLPlatform_h
#if PSPL_TOOLCHAIN

#include <PSPLExtension.h>

enum CHAIN_LINK_TYPE {
    LINK_TYPE_MATRIX34,
    LINK_TYPE_VECTOR4,
    LINK_TYPE_VECTOR3,
    LINK_TYPE_PERSPECTIVE
};

enum CHAIN_LINK_USE {
    LINK_USE_STATIC,
    LINK_USE_DYNAMIC
};

union CHAIN_LINK_VALUE {
    // Static values
    pspl_matrix34_t matrix;
    pspl_vector4_t vector4;
    pspl_vector3_t vector3;
    pspl_perspective_t perspective;
    
    // Dynamic name-binding
    const char* link_dynamic_bind_name;
};



/* Calculation chain type */
typedef struct {
    pspl_malloc_context_t mem_ctx;
} pspl_calc_chain_t;

/* Routines provided by `PSPL-IR` toolchain extension */
extern void pspl_ir_calc_chain_init(pspl_calc_chain_t* chain);
extern void pspl_ir_calc_chain_add_link(pspl_calc_chain_t* chain,
                                        enum CHAIN_LINK_TYPE link_type,
                                        enum CHAIN_LINK_USE link_use,
                                        union CHAIN_LINK_VALUE link_value);

#endif
#endif
