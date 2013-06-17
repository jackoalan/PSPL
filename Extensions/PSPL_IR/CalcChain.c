//
//  CalcChain.c
//  PSPL
//
//  Created by Jack Andersen on 6/13/13.
//
//

#include <stdio.h>
#include <math.h>
#include <PSPL/PSPL_IR.h>
#include <PSPLExtension.h>

#pragma mark Common

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



/* Common, statically encoded chain head */
enum head_use {
    HEAD_USE_NONE = 0,
    HEAD_USE_POS  = 1,
    HEAD_USE_NORM = 2,
    HEAD_USE_UV   = 3
};

typedef struct {
    
    // Enumerated applied use of this chain
    uint8_t head_use;
    
    // Index for UV attribute (if HEAD_USE_UV)
    uint8_t uv_idx;
    
    // Chain link count
    uint8_t link_count;
    
    // Padding
    uint8_t padding;
    
} pspl_calc_chain_head_t;

/* Common, statically encoded link */
typedef struct {
    
    // Transformation matrix implementing link
    pspl_matrix34_t matrix;
    
    // Pad for 4x4-matrix-using APIs
    pspl_vector4_t matrix_pad;
    
} pspl_calc_static_link_t;
typedef DEF_BI_OBJ_TYPE(pspl_calc_static_link_t) pspl_calc_static_link_bi_t;

/* Reference 3x4 matrix multiplier
 * (This may be accelerated with SIMD later) */
static void matrix34_mul(pspl_matrix34_t* a, pspl_matrix34_t* b, pspl_matrix34_t* c) {
    
    // Row 0
    c->matrix[0][0] =
    a->matrix[0][0] * b->matrix[0][0] +
    a->matrix[0][1] * b->matrix[1][0] +
    a->matrix[0][2] * b->matrix[2][0];
    
    c->matrix[0][1] =
    a->matrix[0][0] * b->matrix[0][1] +
    a->matrix[0][1] * b->matrix[1][1] +
    a->matrix[0][2] * b->matrix[2][1];
    
    c->matrix[0][2] =
    a->matrix[0][0] * b->matrix[0][2] +
    a->matrix[0][1] * b->matrix[1][2] +
    a->matrix[0][2] * b->matrix[2][2];
    
    c->matrix[0][3] =
    a->matrix[0][0] * b->matrix[0][3] +
    a->matrix[0][1] * b->matrix[1][3] +
    a->matrix[0][2] * b->matrix[2][3] +
    a->matrix[0][3];
    
    // Row 1
    c->matrix[1][0] =
    a->matrix[1][0] * b->matrix[0][0] +
    a->matrix[1][1] * b->matrix[1][0] +
    a->matrix[1][2] * b->matrix[2][0];
    
    c->matrix[1][1] =
    a->matrix[1][0] * b->matrix[0][1] +
    a->matrix[1][1] * b->matrix[1][1] +
    a->matrix[1][2] * b->matrix[2][1];
    
    c->matrix[1][2] =
    a->matrix[1][0] * b->matrix[0][2] +
    a->matrix[1][1] * b->matrix[1][2] +
    a->matrix[1][2] * b->matrix[2][2];
    
    c->matrix[1][3] =
    a->matrix[1][0] * b->matrix[0][3] +
    a->matrix[1][1] * b->matrix[1][3] +
    a->matrix[1][2] * b->matrix[2][3] +
    a->matrix[1][3];
    
    // Row 2
    c->matrix[2][0] =
    a->matrix[2][0] * b->matrix[0][0] +
    a->matrix[2][1] * b->matrix[1][0] +
    a->matrix[2][2] * b->matrix[2][0];
    
    c->matrix[2][1] =
    a->matrix[2][0] * b->matrix[0][1] +
    a->matrix[2][1] * b->matrix[1][1] +
    a->matrix[2][2] * b->matrix[2][1];
    
    c->matrix[2][2] =
    a->matrix[2][0] * b->matrix[0][2] +
    a->matrix[2][1] * b->matrix[1][2] +
    a->matrix[2][2] * b->matrix[2][2];
    
    c->matrix[2][3] =
    a->matrix[2][0] * b->matrix[0][3] +
    a->matrix[2][1] * b->matrix[1][3] +
    a->matrix[2][2] * b->matrix[2][3] +
    a->matrix[2][3];
    
}

/* Reference 3x4 vector-matrix multiplier
 * (This may be accelerated with SIMD later) */
static void vec_matrix34_mul(pspl_vector3_t* a, pspl_matrix34_t* b, pspl_vector3_t* c) {
    
    c->vector[0] =
    a->vector[0] * b->matrix[0][0] +
    a->vector[1] * b->matrix[0][1] +
    a->vector[2] * b->matrix[0][2] +
    b->matrix[0][3];
    
    c->vector[1] =
    a->vector[0] * b->matrix[1][0] +
    a->vector[1] * b->matrix[1][1] +
    a->vector[2] * b->matrix[1][2] +
    b->matrix[1][3];
    
    c->vector[2] =
    a->vector[0] * b->matrix[2][0] +
    a->vector[1] * b->matrix[2][1] +
    a->vector[2] * b->matrix[2][2] +
    b->matrix[2][3];
    
}

/* Reference 3x4 matrix-vector multiplier
 * (This may be accelerated with SIMD later) */
static void matrix34_vec_mul(pspl_matrix34_t* a, pspl_vector3_t* b, pspl_matrix34_t* c) {
    
    memcpy(c, a, sizeof(pspl_matrix34_t));
    
    c->matrix[0][3] += b->vector[0];
    c->matrix[1][3] += b->vector[1];
    c->matrix[2][3] += b->vector[2];
    
}

#if PSPL_TOOLCHAIN
#pragma mark Toolchain

/* Calculation chain-link type */
typedef struct {
    
    // Link data type
    enum CHAIN_LINK_TYPE link_type;
    
    // How the link value is obtained
    enum CHAIN_LINK_USE link_use;
    
    // Value
    union CHAIN_LINK_VALUE link_value;
    
} pspl_calc_link_t;

/* Identity matrix */
static const pspl_matrix34_t identity_mtx = {1.0,0.0,0.0,0.0,0.0,1.0,0.0,0.0,0.0,0.0,1.0,0.0};

/* Zero vector */
static const pspl_vector3_t zero_vec3 = {0.0,0.0,0.0};

/* Default perspective */
static const pspl_perspective_t default_persp = {
    .fov = 45.0,
    .aspect = 1.0,
    .near = 1.0,
    .far = 1000.0,
    .post_translate_x = 0.0,
    .post_translate_y = 0.0
};

void pspl_calc_chain_init(pspl_calc_chain_t* chain) {
    chain->perspective_use = LINK_USE_NONE;
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
}

void pspl_calc_chain_add_dynamic_transform(pspl_calc_chain_t* chain,
                                           const char* bind_name) {
    pspl_calc_link_t* link = pspl_malloc_malloc(&chain->mem_ctx, sizeof(pspl_calc_link_t));
    
    link->link_type = LINK_TYPE_MATRIX34;
    link->link_use = LINK_USE_DYNAMIC;
    strlcpy(link->link_value.link_dynamic_bind_name, bind_name, IR_NAME_LEN);
    memcpy(&link->link_value.matrix, &identity_mtx, sizeof(pspl_matrix34_t));
}

void pspl_calc_chain_add_static_translation(pspl_calc_chain_t* chain,
                                            const pspl_vector3_t* vector) {
    pspl_calc_link_t* link = pspl_malloc_malloc(&chain->mem_ctx, sizeof(pspl_calc_link_t));
    
    link->link_type = LINK_TYPE_VECTOR3;
    link->link_use = LINK_USE_STATIC;
    memcpy(&link->link_value.vector3, vector, sizeof(pspl_vector3_t));
}

void pspl_calc_chain_add_dynamic_translation(pspl_calc_chain_t* chain,
                                             const char* bind_name) {
    pspl_calc_link_t* link = pspl_malloc_malloc(&chain->mem_ctx, sizeof(pspl_calc_link_t));
    
    link->link_type = LINK_TYPE_VECTOR3;
    link->link_use = LINK_USE_DYNAMIC;
    strlcpy(link->link_value.link_dynamic_bind_name, bind_name, IR_NAME_LEN);
    memcpy(&link->link_value.vector3, &zero_vec3, sizeof(pspl_vector3_t));
}

void pspl_calc_chain_add_static_perspective(pspl_calc_chain_t* chain,
                                            const pspl_perspective_t* persp) {
    
    chain->perspective_use = LINK_USE_STATIC;
    memcpy(&chain->perspective, persp, sizeof(pspl_perspective_t));
    
    build_persp_matrix(&chain->perspective_matrix, persp);
}

void pspl_calc_chain_add_dynamic_perspective(pspl_calc_chain_t* chain,
                                             const char* bind_name) {
    pspl_calc_link_t* link = pspl_malloc_malloc(&chain->mem_ctx, sizeof(pspl_calc_link_t));
    
    chain->perspective_use = LINK_USE_DYNAMIC;
    strlcpy(link->link_value.link_dynamic_bind_name, bind_name, IR_NAME_LEN);
    memcpy(&chain->perspective, &default_persp, sizeof(pspl_perspective_t));
    
    build_persp_matrix(&chain->perspective_matrix, &default_persp);
}


void pspl_calc_chain_send(pspl_calc_chain_t* chain,
                          const pspl_platform_t** platforms) {
    unsigned cur_embed_idx = 0;
    
    int i;
    for (i=0 ; i<chain->mem_ctx.object_num ; ++i) {
        pspl_calc_link_t* link = chain->mem_ctx.object_arr[i];
        
        if (link->link_use == LINK_USE_STATIC) {
            
        }
        
    }
    
}

#elif PSPL_RUNTIME
#pragma mark Runtime

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
