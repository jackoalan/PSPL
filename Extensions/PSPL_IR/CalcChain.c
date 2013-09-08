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

/* Identity matrix */
static const pspl_matrix34_t identity_mtx = {1.0,0.0,0.0,0.0,0.0,1.0,0.0,0.0,0.0,0.0,1.0,0.0};

/* Perspective matrix generation */
#define PI_OVER_360	 0.0087266462599716478846184538424431
static void build_persp_matrix(pspl_matrix34_t m, const pspl_perspective_t* persp) {
    
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
    
    m[0][0] = w;
    m[1][0] = 0;
    m[2][0] = 0;
    
    m[0][1] = 0;
    m[1][1] = h;
    m[2][1] = 0;
    
    m[0][2] = 0;
    m[1][2] = 0;
    m[2][2] = q;
    
    m[0][3] = 0;
    m[1][3] = 0;
    m[2][3] = qn;
    
}

/* Rotation matrix generation */
static void build_rotation_matrix(pspl_matrix34_t m, const pspl_rotation_t* rotation) {
    
    float cos_ang = cosf(rotation->angle);
    float sin_ang = sinf(rotation->angle);
    
    float x = rotation->axis[0];
    float y = rotation->axis[1];
    float z = rotation->axis[2];

    float sq_x = x * x;
    float sq_y = y * y;
    float sq_z = z * z;
    
    m[0][0] = cos_ang+sq_x*(1-cos_ang);
    m[0][1] = x*y*(1-cos_ang)-z*sin_ang;
    m[0][2] = x*z*(1-cos_ang)+y+sin_ang;
    m[0][3] = 0.0;
    
    m[1][0] = y*x*(1-cos_ang)+z*sin_ang;
    m[1][1] = cos_ang+sq_y*(1-cos_ang);
    m[1][2] = y*z*(1-cos_ang)-x*sin_ang;
    m[1][3] = 0.0;
    
    m[2][0] = z*x*(1-cos_ang)-y*sin_ang;
    m[2][1] = z*y*(1-cos_ang)+x*sin_ang;
    m[2][2] = cos_ang+sq_z*(1-cos_ang);
    m[2][3] = 0.0;
    
}


/* Reference 3x4 matrix multiplier
 * (This may be accelerated with SIMD later) */
static void matrix34_mul(pspl_matrix34_t a, pspl_matrix34_t b, pspl_matrix34_t c) {
    
    // Row 0
    c[0][0] =
    a[0][0] * b[0][0] +
    a[0][1] * b[1][0] +
    a[0][2] * b[2][0];
    
    c[0][1] =
    a[0][0] * b[0][1] +
    a[0][1] * b[1][1] +
    a[0][2] * b[2][1];
    
    c[0][2] =
    a[0][0] * b[0][2] +
    a[0][1] * b[1][2] +
    a[0][2] * b[2][2];
    
    c[0][3] =
    a[0][0] * b[0][3] +
    a[0][1] * b[1][3] +
    a[0][2] * b[2][3] +
    a[0][3];
    
    // Row 1
    c[1][0] =
    a[1][0] * b[0][0] +
    a[1][1] * b[1][0] +
    a[1][2] * b[2][0];
    
    c[1][1] =
    a[1][0] * b[0][1] +
    a[1][1] * b[1][1] +
    a[1][2] * b[2][1];
    
    c[1][2] =
    a[1][0] * b[0][2] +
    a[1][1] * b[1][2] +
    a[1][2] * b[2][2];
    
    c[1][3] =
    a[1][0] * b[0][3] +
    a[1][1] * b[1][3] +
    a[1][2] * b[2][3] +
    a[1][3];
    
    // Row 2
    c[2][0] =
    a[2][0] * b[0][0] +
    a[2][1] * b[1][0] +
    a[2][2] * b[2][0];
    
    c[2][1] =
    a[2][0] * b[0][1] +
    a[2][1] * b[1][1] +
    a[2][2] * b[2][1];
    
    c[2][2] =
    a[2][0] * b[0][2] +
    a[2][1] * b[1][2] +
    a[2][2] * b[2][2];
    
    c[2][3] =
    a[2][0] * b[0][3] +
    a[2][1] * b[1][3] +
    a[2][2] * b[2][3] +
    a[2][3];
    
}

/* Reference 3x4 matrix-vector multiplier
 * (This may be accelerated with SIMD later) */
static void matrix34_vec_mul(pspl_matrix34_t a, pspl_vector3_t b, pspl_vector3_t c) {
    
    c[0] =
    b[0] * a[0][0] +
    b[1] * a[0][1] +
    b[2] * a[0][2] +
    a[0][3];
    
    c[1] =
    b[0] * a[1][0] +
    b[1] * a[1][1] +
    b[2] * a[1][2] +
    a[1][3];
    
    c[2] =
    b[0] * a[2][0] +
    b[1] * a[2][1] +
    b[2] * a[2][2] +
    a[2][3];
    
}

/* Matrix byte-swapper */
static void matrix34_byte_swap(pspl_matrix34_t out, const pspl_matrix34_t in) {
    int i,j;
    for (i=0 ; i<3 ; ++i)
        for (j=0 ; j<4 ; ++j)
            out[i][j] = swap_float(in[i][j]);
}

/* Vector4 byte-swapper */
static void vector4_byte_swap(pspl_vector4_t out, const pspl_vector4_t in) {
    int i;
    for (i=0 ; i<4 ; ++i)
        out[i] = swap_float(in[i]);
}


/* Embedded object indexes */
enum object_idxs {
    CALC_POS          = 0,
    CALC_UV0          = 1
};

typedef struct {
    
    // Chain link count
    uint16_t link_count;
    
    // String table offset
    uint16_t str_table_off;
    
} pspl_calc_chain_head_t;
typedef DEF_BI_OBJ_TYPE(pspl_calc_chain_head_t) pspl_calc_chain_head_bi_t;


/* Common, statically encoded link */
typedef struct {
    
    // A 16-bit zero value if indeed static
    uint16_t zero;
    
    // Cache valid (set when concatenation matrix is up-to-date)
    uint16_t cache_valid;
    
    
    // Transformation matrix implementing link
    pspl_matrix34_t matrix;
    
    // Pad for 4x4-matrix-using APIs
    pspl_vector4_t matrix_pad;
    
    
    
    // Cached, concatenated transformation matrix (pre-allocated for runtime)
    pspl_matrix34_t concat_matrix;
    
    // Pad for 4x4-matrix-using APIs
    pspl_vector4_t concat_matrix_pad;
    
} pspl_calc_static_link_t;
typedef DEF_BI_OBJ_TYPE(pspl_calc_static_link_t) pspl_calc_static_link_bi_t;

/* Common, dynamically encoded link */
typedef struct {
    
    // A 16-bit one value if indeed dynamic
    uint16_t one;
    
    // Offset value into string table of dynamic name
    uint16_t name_offset;
    
    // Cache valid (set when concatenation matrix is up-to-date)
    uint16_t cache_valid;
    
    uint16_t padding;
    
    
    // Transformation matrix implementing link (pre-allocated for runtime)
    pspl_matrix34_t matrix;
    
    // Pad for 4x4-matrix-using APIs (pre-allocated for runtime)
    pspl_vector4_t matrix_pad;
    
    
    
    // Cached, concatenated transformation matrix (pre-allocated for runtime)
    pspl_matrix34_t concat_matrix;
    
    // Pad for 4x4-matrix-using APIs
    pspl_vector4_t concat_matrix_pad;
    
} pspl_calc_dynamic_link_t;
typedef DEF_BI_OBJ_TYPE(pspl_calc_dynamic_link_t) pspl_calc_dynamic_link_bi_t;


#if PSPL_TOOLCHAIN
#pragma mark Toolchain

void pspl_calc_chain_init(pspl_calc_chain_t* chain) {
    pspl_malloc_context_init(&chain->mem_ctx);
}
void pspl_calc_chain_destroy(pspl_calc_chain_t* chain) {
    pspl_malloc_context_destroy(&chain->mem_ctx);
}
void pspl_calc_chain_add_static_transform(pspl_calc_chain_t* chain,
                                          const pspl_matrix34_t matrix) {
    pspl_calc_link_t* link = pspl_malloc(&chain->mem_ctx, sizeof(pspl_calc_link_t));
    link->use = LINK_USE_STATIC;
    memcpy(link->matrix_value, matrix, sizeof(pspl_matrix34_t));
}
void pspl_calc_chain_add_dynamic_transform(pspl_calc_chain_t* chain,
                                           const char* bind_name) {
    pspl_calc_link_t* link = pspl_malloc(&chain->mem_ctx, sizeof(pspl_calc_link_t));
    link->use = LINK_USE_DYNAMIC;
    strlcpy(link->link_dynamic_bind_name, bind_name, IR_NAME_LEN);
}
void pspl_calc_chain_add_static_scale(pspl_calc_chain_t* chain,
                                      const pspl_vector3_t scale_vector) {
    pspl_calc_link_t* link = pspl_malloc(&chain->mem_ctx, sizeof(pspl_calc_link_t));
    link->use = LINK_USE_STATIC;
    memcpy(link->matrix_value, &identity_mtx, sizeof(pspl_matrix34_t));
    link->matrix_value[0][0] = scale_vector[0];
    link->matrix_value[1][1] = scale_vector[1];
    link->matrix_value[2][2] = scale_vector[2];
}
void pspl_calc_chain_add_dynamic_scale(pspl_calc_chain_t* chain,
                                       const char* bind_name) {
    pspl_calc_link_t* link = pspl_malloc(&chain->mem_ctx, sizeof(pspl_calc_link_t));
    link->use = LINK_USE_DYNAMIC;
    strlcpy(link->link_dynamic_bind_name, bind_name, IR_NAME_LEN);
}
void pspl_calc_chain_add_static_rotation(pspl_calc_chain_t* chain,
                                         const pspl_rotation_t* rotation) {
    pspl_calc_link_t* link = pspl_malloc(&chain->mem_ctx, sizeof(pspl_calc_link_t));
    link->use = LINK_USE_STATIC;
    build_rotation_matrix(link->matrix_value, rotation);
}
void pspl_calc_chain_add_dynamic_rotation(pspl_calc_chain_t* chain,
                                          const char* bind_name) {
    pspl_calc_link_t* link = pspl_malloc(&chain->mem_ctx, sizeof(pspl_calc_link_t));
    link->use = LINK_USE_DYNAMIC;
    strlcpy(link->link_dynamic_bind_name, bind_name, IR_NAME_LEN);
}
void pspl_calc_chain_add_static_translation(pspl_calc_chain_t* chain,
                                            const pspl_vector3_t trans_vector) {
    pspl_calc_link_t* link = pspl_malloc(&chain->mem_ctx, sizeof(pspl_calc_link_t));
    link->use = LINK_USE_STATIC;
    memcpy(link->matrix_value, &identity_mtx, sizeof(pspl_matrix34_t));
    link->matrix_value[0][3] = trans_vector[0];
    link->matrix_value[1][3] = trans_vector[1];
    link->matrix_value[2][3] = trans_vector[2];
}
void pspl_calc_chain_add_dynamic_translation(pspl_calc_chain_t* chain,
                                             const char* bind_name) {
    pspl_calc_link_t* link = pspl_malloc(&chain->mem_ctx, sizeof(pspl_calc_link_t));
    link->use = LINK_USE_DYNAMIC;
    strlcpy(link->link_dynamic_bind_name, bind_name, IR_NAME_LEN);
}


static void fold_chain(pspl_calc_chain_t* chain, void** big_data_out,
                       void** little_data_out, size_t* size_out) {
    int i,j;
    
    // Calculate size of data chain
    unsigned link_count = 0;
    size_t chain_size = sizeof(pspl_calc_chain_head_t);
    pspl_calc_link_t* prev_link = NULL;
    for (i=chain->mem_ctx.object_num-1 ; i>=0 ; --i) {
        pspl_calc_link_t* link = chain->mem_ctx.object_arr[i];
        if (link->use == LINK_USE_STATIC && !(prev_link && prev_link->use == LINK_USE_STATIC)) {
            chain_size += sizeof(pspl_calc_static_link_t);
            ++link_count;
        } else if (link->use == LINK_USE_DYNAMIC) {
            chain_size += sizeof(pspl_calc_dynamic_link_t);
            ++link_count;
        }
        prev_link = link;
    }
    
    // Calculate size of string table
    pspl_malloc_context_t string_table_ctx;
    pspl_malloc_context_init(&string_table_ctx);
    size_t str_table_size = 0;
    for (i=chain->mem_ctx.object_num-1 ; i>=0 ; --i) {
        pspl_calc_link_t* link = chain->mem_ctx.object_arr[i];
        if (link->use == LINK_USE_DYNAMIC) {
            uint8_t found = 0;
            for (j=0 ; j<string_table_ctx.object_num ; ++j) {
                if (!strcmp(string_table_ctx.object_arr[j] + sizeof(uint32_t),
                            link->link_dynamic_bind_name)) {
                    found = 1;
                    break;
                }
            }
            if (!found) {
                void* buf = pspl_malloc(&string_table_ctx, sizeof(uint32_t) +
                                               strlen(link->link_dynamic_bind_name)+1);
                uint32_t* offset = buf;
                *offset = (uint32_t)str_table_size;
                char* string = buf + sizeof(uint32_t);
                strlcpy(string, link->link_dynamic_bind_name,
                        strlen(link->link_dynamic_bind_name)+1);
                str_table_size += strlen(link->link_dynamic_bind_name)+1;
            }
        }
    }
    
    // Allocate data
    void* big_data = malloc(chain_size+str_table_size);
    void* little_data = malloc(chain_size+str_table_size);
    void* big_cur = big_data;
    void* little_cur = little_data;
    
    // Head
    pspl_calc_chain_head_bi_t head;
    SET_BI_U16(head, link_count, link_count);
    SET_BI_U16(head, str_table_off, chain_size);
    memcpy(big_cur, &head.big, sizeof(pspl_calc_chain_head_t));
    big_cur += sizeof(pspl_calc_chain_head_t);
    memcpy(little_cur, &head.little, sizeof(pspl_calc_chain_head_t));
    little_cur += sizeof(pspl_calc_chain_head_t);
    
    // Concatenate links into data chain
    prev_link = NULL;
    for (i=chain->mem_ctx.object_num-1 ; i>=0 ; --i) {
        pspl_calc_link_t* link = chain->mem_ctx.object_arr[i];
        
        if (link->use == LINK_USE_STATIC) {
            
            pspl_calc_static_link_bi_t trans_static;
            memset(&trans_static, 0, sizeof(pspl_calc_static_link_bi_t));
            SET_BI_U16(trans_static, zero, 0);
            SET_BI_U16(trans_static, cache_valid, 0);
            memcpy(&trans_static.native.matrix, link->matrix_value, sizeof(pspl_matrix34_t));
            matrix34_byte_swap(trans_static.swapped.matrix, trans_static.native.matrix);
            trans_static.native.matrix_pad[0] = 0;
            trans_static.native.matrix_pad[1] = 0;
            trans_static.native.matrix_pad[2] = 0;
            trans_static.native.matrix_pad[3] = 1;
            vector4_byte_swap(trans_static.swapped.matrix_pad, trans_static.native.matrix_pad);
            trans_static.native.concat_matrix_pad[0] = 0;
            trans_static.native.concat_matrix_pad[1] = 0;
            trans_static.native.concat_matrix_pad[2] = 0;
            trans_static.native.concat_matrix_pad[3] = 1;
            vector4_byte_swap(trans_static.swapped.concat_matrix_pad, trans_static.native.concat_matrix_pad);
            
            if (prev_link && prev_link->use == LINK_USE_DYNAMIC) {
                big_cur += sizeof(pspl_calc_static_link_t);
                little_cur += sizeof(pspl_calc_static_link_t);
            } else if (prev_link && prev_link->use == LINK_USE_STATIC) {
#               if defined(__LITTLE_ENDIAN__)
                pspl_calc_static_link_t* prev = little_cur;
#               elif defined(__BIG_ENDIAN__)
                pspl_calc_static_link_t* prev = big_cur;
#               endif
                
                pspl_matrix34_t concat_result;
                matrix34_mul(prev->matrix, trans_static.native.matrix, concat_result);
                memcpy(&trans_static.native.matrix, &concat_result, sizeof(pspl_matrix34_t));
                matrix34_byte_swap(trans_static.swapped.matrix, trans_static.native.matrix);
            }
            
            memcpy(big_cur, &trans_static.big, sizeof(pspl_calc_static_link_t));
            memcpy(little_cur, &trans_static.little, sizeof(pspl_calc_static_link_t));
            
            
        } else if (link->use == LINK_USE_DYNAMIC) {
            
            if (prev_link && prev_link->use == LINK_USE_STATIC) {
                big_cur += sizeof(pspl_calc_static_link_t);
                little_cur += sizeof(pspl_calc_static_link_t);
            }
            
            pspl_calc_dynamic_link_bi_t trans_dynamic;
            memset(&trans_dynamic, 0, sizeof(pspl_calc_dynamic_link_bi_t));
            SET_BI_U16(trans_dynamic, one, 1);
            for (j=0 ; j<string_table_ctx.object_num ; ++j) {
                if (!strcmp(string_table_ctx.object_arr[j] + sizeof(uint32_t),
                            link->link_dynamic_bind_name)) {
                    SET_BI_U16(trans_dynamic, name_offset, *(uint32_t*)string_table_ctx.object_arr[j]);
                    break;
                }
            }
            SET_BI_U16(trans_dynamic, cache_valid, 0);
            memcpy(&trans_dynamic.native.matrix, &identity_mtx, sizeof(pspl_matrix34_t));
            matrix34_byte_swap(trans_dynamic.swapped.matrix, trans_dynamic.native.matrix);
            trans_dynamic.native.matrix_pad[0] = 0;
            trans_dynamic.native.matrix_pad[1] = 0;
            trans_dynamic.native.matrix_pad[2] = 0;
            trans_dynamic.native.matrix_pad[3] = 1;
            vector4_byte_swap(trans_dynamic.swapped.matrix_pad, trans_dynamic.native.matrix_pad);
            trans_dynamic.native.concat_matrix_pad[0] = 0;
            trans_dynamic.native.concat_matrix_pad[1] = 0;
            trans_dynamic.native.concat_matrix_pad[2] = 0;
            trans_dynamic.native.concat_matrix_pad[3] = 1;
            vector4_byte_swap(trans_dynamic.swapped.concat_matrix_pad, trans_dynamic.native.concat_matrix_pad);
            memcpy(big_cur, &trans_dynamic.big, sizeof(pspl_calc_dynamic_link_t));
            big_cur += sizeof(pspl_calc_dynamic_link_t);
            memcpy(little_cur, &trans_dynamic.little, sizeof(pspl_calc_dynamic_link_t));
            little_cur += sizeof(pspl_calc_dynamic_link_t);
            
            
        }
        
        prev_link = link;
    }
    
    // Write string table
    for (j=0 ; j<string_table_ctx.object_num ; ++j) {
        char* string = string_table_ctx.object_arr[j] + sizeof(uint32_t);
        strcpy(little_cur, string);
        strcpy(big_cur, string);
        little_cur += strlen(string) + 1;
        big_cur += strlen(string) + 1;
    }
    
    pspl_malloc_context_destroy(&string_table_ctx);
    
    *big_data_out = big_data;
    *little_data_out = little_data;
    *size_out = chain_size+str_table_size;
}
void pspl_calc_chain_write_position(pspl_calc_chain_t* chain) {
    void* big_data;
    void* little_data;
    size_t size;
    fold_chain(chain, &big_data, &little_data, &size);
    pspl_embed_integer_keyed_object(NULL, CALC_POS, little_data, big_data, size);
}
void pspl_calc_chain_write_uv(pspl_calc_chain_t* chain, unsigned uv_idx) {
    void* big_data;
    void* little_data;
    size_t size;
    fold_chain(chain, &big_data, &little_data, &size);
    pspl_embed_integer_keyed_object(NULL, CALC_UV0+uv_idx, little_data, big_data, size);
}

#elif PSPL_RUNTIME
#pragma mark Runtime

static pspl_matrix34_t* cache_chain(void* chain_data) {
    pspl_calc_chain_head_t* head = chain_data;
    void* data_cur = chain_data + sizeof(pspl_calc_chain_head_t);
    int i;
    
    if (!head->link_count)
        return NULL;
    
    int cache_invalid = 0;
    pspl_matrix34_t* prev_mtx = NULL;
    for (i=0 ; i<head->link_count ; ++i) {
        uint16_t* decider = data_cur;
        if (*decider) {
            // Dynamic
            pspl_calc_dynamic_link_t* link = data_cur;
            if (cache_invalid || !link->cache_valid) {
                cache_invalid = 1;
                if (prev_mtx)
                    matrix34_mul(*prev_mtx, link->matrix, link->concat_matrix);
                link->cache_valid = 1;
            }
            if (prev_mtx)
                prev_mtx = &link->concat_matrix;
            else
                prev_mtx = &link->matrix;
            data_cur += sizeof(pspl_calc_dynamic_link_t);
        } else {
            // Static
            pspl_calc_static_link_t* link = data_cur;
            if (cache_invalid || !link->cache_valid) {
                cache_invalid = 1;
                if (prev_mtx)
                    matrix34_mul(*prev_mtx, link->matrix, link->concat_matrix);
                link->cache_valid = 1;
            }
            if (prev_mtx)
                prev_mtx = &link->concat_matrix;
            else
                prev_mtx = &link->matrix;
            data_cur += sizeof(pspl_calc_static_link_t);
        }
    }
    
    return prev_mtx;
}

static void set_chain_dynamic(void* chain_data, const char* bind_name,
                              const pspl_matrix34_t matrix) {
    pspl_calc_chain_head_t* head = chain_data;
    void* data_cur = chain_data + sizeof(pspl_calc_chain_head_t);
    int i;
    
    if (!head->link_count)
        return;
    
    for (i=0 ; i<head->link_count ; ++i) {
        uint16_t* decider = data_cur;
        if (*decider) {
            // Dynamic
            pspl_calc_dynamic_link_t* link = data_cur;
            if (!strcasecmp(chain_data + head->str_table_off +
                            link->name_offset, bind_name)) {
                memcpy(&link, matrix, sizeof(pspl_matrix34_t));
                link->cache_valid = 0;
            }
            data_cur += sizeof(pspl_calc_dynamic_link_t);
        } else
            data_cur += sizeof(pspl_calc_static_link_t);
    }
}

static void* CHAIN_POS = NULL;
static void* CHAIN_UV[MAX_CALC_UVS] = {NULL};

static int enumerate_chains(pspl_data_object_t* data_object, uint32_t key, void* user_ptr) {
    if (key == CALC_POS)
        CHAIN_POS = data_object->object_data;
    else
        CHAIN_UV[key-CALC_UV0] = data_object->object_data;
    return 0;
}
void pspl_calc_chain_bind(const pspl_runtime_psplc_t* object) {
    CHAIN_POS = NULL;
    memset(CHAIN_UV, 0, sizeof(void*)*MAX_CALC_UVS);
    pspl_runtime_enumerate_integer_embedded_data_objects(object, enumerate_chains, NULL);
}

void pspl_calc_chain_set_dynamic_transform(const char* bind_name,
                                           const pspl_matrix34_t matrix) {
    if (CHAIN_POS)
        set_chain_dynamic(CHAIN_POS, bind_name, matrix);
    
    int i;
    for (i=0 ; i<MAX_CALC_UVS ; ++i) {
        if (CHAIN_UV[i])
            set_chain_dynamic(CHAIN_UV[i], bind_name, matrix);
    }
}
void pspl_calc_chain_set_dynamic_scale(const char* bind_name,
                                       const pspl_vector3_t scale_vector) {
    pspl_matrix34_t mtx;
    memcpy(mtx, &identity_mtx, sizeof(pspl_matrix34_t));
    mtx[0][0] = scale_vector[0];
    mtx[1][1] = scale_vector[1];
    mtx[2][2] = scale_vector[2];
    pspl_calc_chain_set_dynamic_transform(bind_name, mtx);
}
void pspl_calc_chain_set_dynamic_rotation(const char* bind_name,
                                          const pspl_rotation_t* rotation) {
    pspl_matrix34_t mtx;
    build_rotation_matrix(mtx, rotation);
    pspl_calc_chain_set_dynamic_transform(bind_name, mtx);
}
void pspl_calc_chain_set_dynamic_translation(const char* bind_name,
                                             const pspl_vector3_t vector) {
    pspl_matrix34_t mtx;
    memcpy(mtx, &identity_mtx, sizeof(pspl_matrix34_t));
    mtx[0][3] = vector[0];
    mtx[1][3] = vector[1];
    mtx[2][3] = vector[2];
    pspl_calc_chain_set_dynamic_transform(bind_name, mtx);
}

/* Platform-implemented matrix loading routines */
extern void pspl_ir_load_pos_mtx(pspl_matrix34_t* mtx);
extern void pspl_ir_load_norm_mtx(pspl_matrix34_t* mtx);
extern void pspl_ir_load_uv_mtx(pspl_matrix34_t* mtx);
extern void pspl_ir_load_finish();

void pspl_calc_chain_flush() {
    if (CHAIN_POS)
        pspl_ir_load_pos_mtx(cache_chain(CHAIN_POS));
    
    // TODO: NORM MTX
    
    int i;
    for (i=0 ; i<MAX_CALC_UVS ; ++i) {
        if (CHAIN_UV[i])
            pspl_ir_load_uv_mtx(cache_chain(CHAIN_UV[i]));
    }
    
    pspl_ir_load_finish();
}




#endif
