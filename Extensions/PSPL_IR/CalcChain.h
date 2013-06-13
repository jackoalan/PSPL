//
//  CalcChain.h
//  PSPL
//
//  Created by Jack Andersen on 6/13/13.
//
//

#ifndef PSPL_CalcChain_h
#define PSPL_CalcChain_h

#include <PSPL/PSPLCommon.h>

#if PSPL_TOOLCHAIN

enum CHAIN_LINK_TYPE {
    LINK_TYPE_IDENTITY,
    LINK_TYPE_MATRIX34,
    LINK_TYPE_VECTOR4,
    LINK_TYPE_VECTOR3,
    LINK_TYPE_PERSPECTIVE
};

enum CHAIN_LINK_USE {
    LINK_USE_STATIC,
    LINK_USE_DYNAMIC,
    LINK_USE_GPU
};

union CHAIN_LINK_VALUE {
    
    // Static values
    pspl_matrix34_t matrix;
    pspl_vector3_t vector3;
    pspl_perspective_t perspective;
    
    // Dynamic name-binding
    char link_dynamic_bind_name[128];
    
    // GPU data-source binding
    // (Used in fragment stage for dynamic GPU data-handling features)
    enum {
        LINK_GPU_POSITION,
        LINK_GPU_NORMAL,
        LINK_GPU_VERT_UV
    } link_gpu_bind;
    unsigned link_gpu_vert_uv_idx;
    
};



/* Calculation chain type */
typedef struct {
    pspl_malloc_context_t mem_ctx;
} pspl_calc_chain_t;

/* Routines provided by `PSPL-IR` toolchain extension */
void pspl_calc_chain_init(pspl_calc_chain_t* chain);
void pspl_calc_chain_destroy(pspl_calc_chain_t* chain);
void pspl_calc_chain_add_static_transform(pspl_calc_chain_t* chain,
                                          const pspl_matrix34_t* matrix);
void pspl_calc_chain_add_dynamic_transform(pspl_calc_chain_t* chain,
                                           const char* bind_name);
void pspl_calc_chain_add_static_translation(pspl_calc_chain_t* chain,
                                            const pspl_vector3_t* vector);
void pspl_calc_chain_add_dynamic_translation(pspl_calc_chain_t* chain,
                                             const char* bind_name);
void pspl_calc_chain_add_static_perspective(pspl_calc_chain_t* chain,
                                            const pspl_perspective_t* persp);
void pspl_calc_chain_add_dynamic_perspective(pspl_calc_chain_t* chain,
                                             const char* bind_name);
void pspl_calc_chain_send(pspl_calc_chain_t* chain,
                          const pspl_platform_t** platforms);

#elif PSPL_RUNTIME

/* Routines provided by `PSPL-IR` runtime extension */
void pspl_calc_chain_bind(const void* chain_data);
void pspl_calc_chain_set_dynamic_transform(const char* bind_name,
                                           const pspl_matrix34_t* matrix);
void pspl_calc_chain_set_dynamic_translation(const char* bind_name,
                                             const pspl_vector3_t* vector);
void pspl_calc_chain_set_dynamic_perspective(const char* bind_name,
                                             const pspl_perspective_t* persp);

#endif
#endif
