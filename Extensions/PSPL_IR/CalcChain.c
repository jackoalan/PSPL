//
//  CalcChain.c
//  PSPL
//
//  Created by Jack Andersen on 6/13/13.
//
//

#include <stdio.h>
#include <math.h>
#include "CalcChain.h"
#include <PSPLExtension.h>

#define PI_OVER_360	 0.0087266462599716478846184538424431

static void build_persp_matrix(pspl_matrix34_t* m, const pspl_perspective_t* persp) {
    
    float xymax = persp->near * tanf(persp->fov * PI_OVER_360);
    float ymin = -xymax;
    float xmin = -xymax;
    
    float width = xymax - xmin;
    float height = xymax - ymin;
    
    float depth = persp->far - persp->near;
    float q = -(persp->far + persp->near) / depth;
    float qn = -2 * (persp->far * persp->near) / depth;
    
    
    float w = 2 * persp->near / width;
    w = w / persp->aspect;
    float h = 2 * persp->near / height;
    
    m->matrix[0][0] = w;
    m->matrix[1][0] = 0;
    m->matrix[2][0] = 0;
    
    m->matrix[0][1] = 0;
    m->matrix[1][1] = h;
    m->matrix[2][1] = 0;
    
    m->matrix[0][2] = 0;
    m->matrix[1][2] = 0;
    m->matrix[2][2] = q;
    
    m->matrix[0][3] = 0;
    m->matrix[1][3] = 0;
    m->matrix[2][3] = qn;
    
}

#if PSPL_TOOLCHAIN

/* Calculation chain-link type */
typedef struct {
    
    // Link data type
    enum CHAIN_LINK_TYPE link_type;
    
    // How the link value is obtained
    enum CHAIN_LINK_USE link_use;
    
    // Value
    union CHAIN_LINK_VALUE link_value;
    
    // Cached matrix value
    pspl_matrix34_t link_matrix_value;
    
} pspl_calc_link_t;

/* Identity matrix */
static const pspl_matrix34_t identity_mtx = {1.0,0.0,0.0,0.0,0.0,1.0,0.0,0.0,0.0,0.0,1.0,0.0};

/* Zero vector */
static const pspl_vector3_t zero_vec3 = {0.0,0.0,0.0};

/* Default perspective */
static const pspl_perspective_t default_persp = {
    .fov = 45.0,
    .aspect = 16.0/9.0,
    .near = 1.0,
    .far = 1000.0,
    .post_translate_x = 0.0,
    .post_translate_y = 0.0
};

void pspl_calc_chain_init(pspl_calc_chain_t* chain) {
    pspl_malloc_context_init(&chain->mem_ctx);
}

void pspl_calc_chain_destroy(pspl_calc_chain_t* chain) {
    pspl_malloc_context_destroy(&chain->mem_ctx);
}

void pspl_calc_chain_add_static_transform(pspl_calc_chain_t* chain,
                                          const pspl_matrix34_t* matrix) {
    pspl_calc_link_t* link = pspl_malloc_malloc(&chain->mem_ctx, sizeof(pspl_calc_link_t));
    
    link->link_type = LINK_TYPE_MATRIX34;
    link->link_use = LINK_USE_STATIC;
    memcpy(&link->link_value.matrix, matrix, sizeof(pspl_matrix34_t));
    
    memcpy(&link->link_matrix_value, matrix, sizeof(pspl_matrix34_t));
}

void pspl_calc_chain_add_dynamic_transform(pspl_calc_chain_t* chain,
                                           const char* bind_name) {
    pspl_calc_link_t* link = pspl_malloc_malloc(&chain->mem_ctx, sizeof(pspl_calc_link_t));
    
    link->link_type = LINK_TYPE_MATRIX34;
    link->link_use = LINK_USE_DYNAMIC;
    strlcpy(link->link_value.link_dynamic_bind_name, bind_name, 128);
    memcpy(&link->link_value.matrix, &identity_mtx, sizeof(pspl_matrix34_t));
    
    memcpy(&link->link_matrix_value, &identity_mtx, sizeof(pspl_matrix34_t));
}

void pspl_calc_chain_add_static_translation(pspl_calc_chain_t* chain,
                                            const pspl_vector3_t* vector) {
    pspl_calc_link_t* link = pspl_malloc_malloc(&chain->mem_ctx, sizeof(pspl_calc_link_t));
    
    link->link_type = LINK_TYPE_VECTOR3;
    link->link_use = LINK_USE_STATIC;
    memcpy(&link->link_value.vector3, vector, sizeof(pspl_vector3_t));
    
    memcpy(&link->link_matrix_value, &identity_mtx, sizeof(pspl_matrix34_t));
    link->link_matrix_value.matrix[0][3] = vector->vector[0];
    link->link_matrix_value.matrix[1][3] = vector->vector[1];
    link->link_matrix_value.matrix[2][3] = vector->vector[2];
}

void pspl_calc_chain_add_dynamic_translation(pspl_calc_chain_t* chain,
                                             const char* bind_name) {
    pspl_calc_link_t* link = pspl_malloc_malloc(&chain->mem_ctx, sizeof(pspl_calc_link_t));
    
    link->link_type = LINK_TYPE_VECTOR3;
    link->link_use = LINK_USE_DYNAMIC;
    strlcpy(link->link_value.link_dynamic_bind_name, bind_name, 128);
    memcpy(&link->link_value.vector3, &zero_vec3, sizeof(pspl_vector3_t));
    
    memcpy(&link->link_matrix_value, &identity_mtx, sizeof(pspl_matrix34_t));
}

void pspl_calc_chain_add_static_perspective(pspl_calc_chain_t* chain,
                                            const pspl_perspective_t* persp) {
    pspl_calc_link_t* link = pspl_malloc_malloc(&chain->mem_ctx, sizeof(pspl_calc_link_t));
    
    link->link_type = LINK_TYPE_PERSPECTIVE;
    link->link_use = LINK_USE_STATIC;
    memcpy(&link->link_value.perspective, persp, sizeof(pspl_perspective_t));
    
    build_persp_matrix(&link->link_matrix_value, persp);
}

void pspl_calc_chain_add_dynamic_perspective(pspl_calc_chain_t* chain,
                                             const char* bind_name) {
    pspl_calc_link_t* link = pspl_malloc_malloc(&chain->mem_ctx, sizeof(pspl_calc_link_t));
    
    link->link_type = LINK_TYPE_PERSPECTIVE;
    link->link_use = LINK_USE_DYNAMIC;
    strlcpy(link->link_value.link_dynamic_bind_name, bind_name, 128);
    memcpy(&link->link_value.perspective, &default_persp, sizeof(pspl_perspective_t));
    
    build_persp_matrix(&link->link_matrix_value, &default_persp);
}


void pspl_calc_chain_send(pspl_calc_chain_t* chain,
                          const pspl_platform_t** platforms) {
    
    
}

#elif PSPL_RUNTIME

void pspl_calc_chain_bind(const void* chain_data) {
    
}

void pspl_calc_chain_set_dynamic_transform(const char* bind_name,
                                           const pspl_matrix34_t* matrix) {
    
}

void pspl_calc_chain_set_dynamic_translation(const char* bind_name,
                                             const pspl_vector3_t* vector) {
    
}

void pspl_calc_chain_set_dynamic_perspective(const char* bind_name,
                                             const pspl_perspective_t* persp) {
    
}


#endif
