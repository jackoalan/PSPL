//
//  PMDLRuntimeProcessing.c
//  PSPL
//
//  Created by Jack Andersen on 7/22/13.
//
//

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <PSPLExtension.h>
#include <PSPLRuntime.h>
#include <PMDLRuntime.h>
#include "PMDLCommon.h"
#include "PMDLRuntimeProcessing.h"

/* Homogenous transformation matrix bottom row */
static const pspl_vector4_t HOMOGENOUS_BOTTOM_VECTOR = {.f[0]=0, .f[1]=0, .f[2]=0, .f[3]=1};

/* Logical XOR */
#define LXOR(a,b) (((a) || (b)) && !((a) && (b)))


/* Platform headers */
#if PSPL_RUNTIME_PLATFORM_GL2
#   if __APPLE__
#       if TARGET_OS_MAC
#           include <OpenGL/glext.h>
#       elif TARGET_OS_IPHONE
#           include <OpenGLES/ES2/glext.h>
#       endif
#   else
#       include <GL/glext.h>
#   endif

/* OpenGL VAO extension required; 
 * attempt to include gl3.h (with standard VAO support) 
 * if no supported extension found */
#   define GL2INC 1
#   if GL_APPLE_vertex_array_object
#       define GLVAO(name) name##APPLE
#   elif GL_OES_vertex_array_object
#       define GLVAO(name) name##OES
#   else
#       if __APPLE__
#           include <OpenGL/gl3.h>
#           include <OpenGL/gl3ext.h>
#       else
#           include <GL/gl3.h>
#           include <GL/gl3ext.h>
#       endif
#       undef GL2INC
#       define GLVAO(name) name
#   endif

#   if GL2INC
#       if __APPLE__
#           include <TargetConditionals.h>
#           if TARGET_OS_MAC
#               include <OpenGL/gl.h>
#           elif TARGET_OS_IPHONE
#               include <OpenGLES/ES2/gl.h>
#           endif
#       else
#           include <GL/gl.h>
#       endif
#   endif


#elif PSPL_RUNTIME_PLATFORM_D3D11
#   include <d3d11.h>

#elif PSPL_RUNTIME_PLATFORM_GX
#   include <ogc/gx.h>
#   include <ogc/cache.h>

#endif


/* If using OpenGL OR Direct3D, we're using the General Draw Format */
#if PSPL_RUNTIME_PLATFORM_GL2 || PSPL_RUNTIME_PLATFORM_D3D11
#   define PMDL_GENERAL 1
    static const char* DRAW_FMT = "_GEN";
#elif PSPL_RUNTIME_PLATFORM_GX
#   define PMDL_GX 1
    static const char* DRAW_FMT = "__GX";
#endif

/* Static identity matrix */
static const pspl_matrix44_t IDENTITY_MATRIX = {
    .m[0][0] = 1, .m[0][1] = 0, .m[0][2] = 0, .m[0][3] = 0,
    .m[1][0] = 0, .m[1][1] = 1, .m[1][2] = 0, .m[1][3] = 0,
    .m[2][0] = 0, .m[2][1] = 0, .m[2][2] = 1, .m[2][3] = 0,
    .m[3][0] = 0, .m[3][1] = 0, .m[3][2] = 0, .m[3][3] = 1
};

/* Static zero vector */
static const pspl_vector4_t ZERO_VECTOR = {
    .f[0] = 0, .f[1] = 0, .f[2] = 0, .f[3] = 0
};

/* RH->LH swizzle matrix */
static const pspl_matrix34_t LH_SWIZZLE_MATRIX = {
    .m[0][0] = -1, .m[0][1] = 0, .m[0][2] = 0, .m[0][3] = 0,
    .m[1][0] = 0, .m[1][1] = 0, .m[1][2] = 1, .m[1][3] = 0,
    .m[2][0] = 0, .m[2][1] = 1, .m[2][2] = 0, .m[2][3] = 0,
};


/* PAR Enum */
enum pmdl_par {
    PMDL_PAR0 = 0,
    PMDL_PAR1 = 1,
    PMDL_PAR2 = 2
};

/* PMDL Collection Header */
#pragma pack(1)
typedef struct __attribute__ ((__packed__)) {
    
    uint16_t uv_count;
    uint16_t bone_count;
    uint32_t vert_buf_off;
    uint32_t vert_buf_len;
    uint32_t elem_buf_off;
    uint32_t elem_buf_len;
    uint32_t draw_idx_off;
    
} pmdl_col_header;
#pragma pack()


/* PMDL Mesh header */
typedef struct {
    
    float mesh_aabb[2][3];
    int32_t shader_index;
    const pspl_runtime_psplc_t* shader_pointer;
    
} pmdl_mesh_header;


#if PMDL_GENERAL
    enum pmdl_prim_type {
        PMDL_POINTS          = 0,
        PMDL_TRIANGLES       = 1,
        PMDL_TRIANGLE_FANS   = 2,
        PMDL_TRIANGLE_STRIPS = 3,
        PMDL_LINES           = 4,
        PMDL_LINE_STRIPS     = 5
    };
    /* PMDL General Primitive */
    typedef struct {
        uint32_t prim_type;
        uint32_t prim_start_idx;
        uint32_t prim_count;
    } pmdl_general_prim;
    typedef struct {
        uint32_t skin_idx;
        pmdl_general_prim prim;
    } pmdl_general_prim_par1;

    struct gl_bufs_t {
        GLuint vao, vert_buf, elem_buf;
    };
#elif PMDL_GX
    typedef struct {
        uint32_t dl_offset, dl_length;
    } pmdl_gx_mesh;

    typedef struct {
        f32* position_stage;
        f32* normal_stage;
    } pmdl_gx_par1_vertbuf_head;

    typedef struct {
        u32 bone_count;
        f32 identity_blend;
        struct pmdl_gx_par1_vert_head_bone {
            u32 bone_idx;
            f32 bone_weight;
        } bone_arr[];
    } pmdl_gx_par1_vert_head;
#endif


#pragma mark Master Init

pspl_matrix44_t IDENTITY_MATS[PMDL_MAX_BONES];
void pmdl_master_init() {
    int i;
    for (i=0 ; i<PMDL_MAX_BONES ; ++i)
        pmdl_matrix44_identity(&IDENTITY_MATS[i]);
}


#pragma mark PMDL Loading / Unloading

/* This routine will load collection data */
static int pmdl_init_collections(const pspl_runtime_arc_file_t* pmdl_file) {
    void* file_data = pmdl_file->file_data;
    pmdl_header* header = file_data;
    
    // Shader hash array
    pspl_hash* shader_hashes = file_data + header->shader_table_offset + sizeof(uint32_t);
    
    // Init collections
    void* collection_buf = file_data + header->collection_offset;
    pmdl_col_header* collection_header = collection_buf;
    unsigned i;
    for (i=0 ; i<header->collection_count; ++i) {
        void* index_buf = collection_buf + collection_header->draw_idx_off;
        
        // Mesh count
        uint32_t mesh_count = *(uint32_t*)index_buf;
        uint32_t index_buf_offset = *(uint32_t*)(index_buf+4);
        
        // Mesh head array; lookup shader objects and cache pointers
        pmdl_mesh_header* mesh_heads = index_buf+8;
        unsigned j;
        for (j=0 ; j<mesh_count ; ++j) {
            pmdl_mesh_header* mesh_head = &mesh_heads[j];
            if (mesh_head->shader_index < 0)
                mesh_head->shader_pointer = NULL;
            else
                mesh_head->shader_pointer =
                pspl_runtime_get_psplc_from_hash(pmdl_file->parent, &shader_hashes[mesh_head->shader_index], 0);
        }
        index_buf += index_buf_offset;
        
#       if PSPL_RUNTIME_PLATFORM_GL2
            struct gl_bufs_t* gl_bufs = index_buf;
        
            // VAO
            GLVAO(glGenVertexArrays)(1, &gl_bufs->vao);
            GLVAO(glBindVertexArray)(gl_bufs->vao);
            
            // VBOs
            glGenBuffers(2, (GLuint*)&gl_bufs->vert_buf);
            glBindBuffer(GL_ARRAY_BUFFER, gl_bufs->vert_buf);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gl_bufs->elem_buf);
            glBufferData(GL_ARRAY_BUFFER, collection_header->vert_buf_len,
                         collection_buf + collection_header->vert_buf_off, GL_STATIC_DRAW);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, collection_header->elem_buf_len,
                         collection_buf + collection_header->elem_buf_off, GL_STATIC_DRAW);
            
            // Attributes
            GLsizei buf_stride = 24 + collection_header->uv_count*8 + collection_header->bone_count*4;
            
            glEnableVertexAttribArray(0); // Position
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, buf_stride, 0);
        
            glEnableVertexAttribArray(1); // Normal
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, buf_stride, (GLvoid*)12);
        
            GLuint idx = 2;
            for (j=0 ; j<collection_header->uv_count ; ++j) { // UVs
                glEnableVertexAttribArray(idx);
                glVertexAttribPointer(idx, 2, GL_FLOAT, GL_FALSE, buf_stride, (GLvoid*)(GLsizeiptr)(24+8*j));
                ++idx;
            }
            
            GLsizeiptr weight_offset = 24 + collection_header->uv_count*8;
            if (collection_header->bone_count) {
                for (j=0 ; j<(collection_header->bone_count/4) ; ++j) { // Bone Weight Coefficients
                    glEnableVertexAttribArray(idx);
                    glVertexAttribPointer(idx, 4, GL_FLOAT, GL_FALSE, buf_stride, (GLvoid*)(weight_offset+16*j));
                    ++idx;
                }
            }
        
#       elif PSPL_RUNTIME_PLATFORM_D3D11
        
#       elif PSPL_RUNTIME_PLATFORM_GX
        
            // PAR1 prep
            if (header->sub_type_num == '1') {
                
                void* vert_buf = collection_buf + collection_header->vert_buf_off;
                uint32_t par1_off = *(uint32_t*)vert_buf;
                uint32_t pn_count = *(uint32_t*)(vert_buf+4);
                
                void* par1_cur = vert_buf + par1_off;
                pmdl_gx_par1_vertbuf_head* vertbuf_head = par1_cur;
                vertbuf_head->position_stage = pspl_allocate_media_block(sizeof(f32)*3*pn_count);
                vertbuf_head->normal_stage = pspl_allocate_media_block(sizeof(f32)*3*pn_count);
                
            }
        
#       endif
        
        // ADVANCE!!
        collection_header += sizeof(pmdl_col_header);
    }
    
    return 0;
    
}

/* This routine will unload collection data */
static void pmdl_destroy_collections(void* file_data) {
    pmdl_header* header = file_data;

    // Init collections
    void* collection_buf = file_data + header->collection_offset;
    pmdl_col_header* collection_header = collection_buf;
    int i;
    for (i=0 ; i<header->collection_count; ++i) {
        void* index_buf = collection_buf + collection_header->draw_idx_off;
        
#       if PSPL_RUNTIME_PLATFORM_GL2
            struct gl_bufs_t* gl_bufs = index_buf;
            GLVAO(glBindVertexArray)(gl_bufs->vao);
        
            glGetIntegerv(GL_ARRAY_BUFFER_BINDING, (GLint*)&gl_bufs->vert_buf);
            glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, (GLint*)&gl_bufs->elem_buf);
            glDeleteBuffers(2, (GLuint*)&gl_bufs->vert_buf);
            GLVAO(glDeleteVertexArrays)(1, &gl_bufs->vao);
        
#       elif PSPL_RUNTIME_PLATFORM_D3D11
        
#       elif PSPL_RUNTIME_PLATFORM_GX
        
            // PAR1 destroy
            if (header->sub_type_num == '1') {
                
                void* vert_buf = collection_buf + collection_header->vert_buf_off;
                uint32_t par1_off = *(uint32_t*)vert_buf;
                
                void* par1_cur = vert_buf + par1_off;
                pmdl_gx_par1_vertbuf_head* vertbuf_head = par1_cur;
                pspl_free_media_block(vertbuf_head->position_stage);
                pspl_free_media_block(vertbuf_head->normal_stage);
                
            }
        
#       endif
        
        // ADVANCE!!
        collection_header += sizeof(pmdl_col_header);
    }
        
}

/* This routine will validate and load PMDL data into GPU */
int pmdl_init(pmdl_t* pmdl) {
    char hash[PSPL_HASH_STRING_LEN];
    
    // First, validate header members
    pmdl_header* header = pmdl->file_ptr->file_data;
    
    // Magic
    if (memcmp(header->magic, "PMDL", 4)) {
        pspl_hash_fmt(hash, &pmdl->file_ptr->hash);
        pspl_warn("Unable to init PMDL", "file `%s` has invalid magic; skipping", hash);
        return -1;
    }
    
    // Endianness
#   if __LITTLE_ENDIAN__
        char* endian_str = "_LIT";
#   elif __BIG_ENDIAN__
        char* endian_str = "_BIG";
#   endif
    if (memcmp(header->endianness, endian_str, 4)) {
        pspl_hash_fmt(hash, &pmdl->file_ptr->hash);
        pspl_warn("Unable to init PMDL", "file `%s` has invalid endianness; skipping", hash);
        return -1;
    }
    
    // Pointer size
    if (header->pointer_size != sizeof(void*)) {
        pspl_hash_fmt(hash, &pmdl->file_ptr->hash);
        pspl_warn("Unable to init PMDL", "file `%s` pointer size doesn't match (%d != %lu); skipping",
                  hash, header->pointer_size, (unsigned long)sizeof(void*));
        return -1;
    }
    
    // Draw Format
    if (memcmp(header->draw_format, DRAW_FMT, 4)) {
        pspl_hash_fmt(hash, &pmdl->file_ptr->hash);
        pspl_warn("Unable to init PMDL", "file `%s` has `%.4s` draw format, expected `%.4s` format; skipping",
                  hash, header->draw_format, DRAW_FMT);
        return -1;
    }
    
    
    // Decode PAR sub-type
    if (memcmp(header->sub_type_prefix, "PAR", 3)) {
        pspl_hash_fmt(hash, &pmdl->file_ptr->hash);
        pspl_warn("Unable to init PMDL", "invalid PAR type specified in `%s`; skipping", hash);
        return -1;
    }
    enum pmdl_par sub_type;
    if (header->sub_type_num == '0')
        sub_type = PMDL_PAR0;
    else if (header->sub_type_num == '1')
        sub_type = PMDL_PAR1;
    else if (header->sub_type_num == '2')
        sub_type = PMDL_PAR2;
    else {
        pspl_hash_fmt(hash, &pmdl->file_ptr->hash);
        pspl_warn("Unable to init PMDL", "invalid PAR type specified in `%s`; skipping", hash);
        return -1;
    }
    
    // Load shaders into GPU
    int i;
    uint32_t shader_count = *(uint32_t*)(pmdl->file_ptr->file_data + header->shader_table_offset);
    pspl_hash* shader_array = pmdl->file_ptr->file_data + header->shader_table_offset + sizeof(uint32_t);
    for (i=0 ; i<shader_count ; ++i)
        pspl_runtime_get_psplc_from_hash(pmdl->file_ptr->parent, &shader_array[i], 1);
    
    // Init rigging context (if PAR1)
    if (header->sub_type_num == '1')
        pmdl_rigging_init(&pmdl->rigging_ptr, pmdl->file_ptr->file_data + sizeof(pmdl_header),
                          pmdl->file_ptr->file_data + header->bone_table_offset);
    
    // Load collections into GPU
    pmdl_init_collections(pmdl->file_ptr);
#   if PMDL_GX
        DCStoreRange(pmdl->file_ptr->file_data, pmdl->file_ptr->file_len);
#   endif
    
    return 0;

}

/* This routine will unload data from GPU */
void pmdl_destroy(pmdl_t* pmdl) {
    pmdl_header* header = pmdl->file_ptr->file_data;

    // Unload shaders from GPU
    int i;
    uint32_t shader_count = *(uint32_t*)(pmdl->file_ptr->file_data + header->shader_table_offset);
    pspl_hash* shader_array = pmdl->file_ptr->file_data + header->shader_table_offset + sizeof(uint32_t);
    for (i=0 ; i<shader_count ; ++i) {
        const pspl_runtime_psplc_t* shader_obj =
        pspl_runtime_get_psplc_from_hash(pmdl->file_ptr->parent, &shader_array[i], 0);
        pspl_runtime_release_psplc(shader_obj);
    }
    
    // Destroy rigging context (if PAR1)
    if (header->sub_type_num == '1')
        pmdl_rigging_destroy(pmdl->rigging_ptr);
    
    // Unload collections from GPU
    pmdl_destroy_collections(pmdl->file_ptr->file_data);
    
}


#pragma mark PMDL Action lookup

/* Lookup PMDL rigging action */
const pmdl_action* pmdl_action_lookup(const pmdl_t* pmdl, const char* action_name) {
    int i;
    
    for (i=0 ; i<pmdl->rigging_ptr->action_count ; ++i) {
        const pmdl_action* action = &pmdl->rigging_ptr->action_array[i];
        if (!strcmp(action_name, action->action_name))
            return action;
    }
    
    return NULL;
    
}


#pragma mark Context Init

static void _pmdl_set_default_context(pmdl_draw_context_t* ctx) {
    
    int i;
    memset(ctx->texcoord_mtx, 0, 8*sizeof(pspl_matrix34_t));
    for (i=0 ; i<PMDL_MAX_TEXCOORD_MATS ; ++i) {
        ctx->texcoord_mtx[i].m[0][0] = 1;
        ctx->texcoord_mtx[i].m[1][1] = -1;
    }
    
    memset(ctx->model_mtx.m, 0, sizeof(pspl_matrix34_t));
    ctx->model_mtx.m[0][0] = 1;
    ctx->model_mtx.m[1][1] = 1;
    ctx->model_mtx.m[2][2] = 1;
    ctx->camera_view.pos.f[0] = 0;
    ctx->camera_view.pos.f[1] = 3;
    ctx->camera_view.pos.f[2] = 0;
    ctx->camera_view.look.f[0] = 0;
    ctx->camera_view.look.f[1] = 0;
    ctx->camera_view.look.f[2] = 0;
    ctx->camera_view.up.f[0] = 0;
    ctx->camera_view.up.f[1] = 0;
    ctx->camera_view.up.f[2] = 1;
    ctx->projection_type = PMDL_PERSPECTIVE;
    ctx->projection.perspective.fov = 55;
    ctx->projection.perspective.far = 5;
    ctx->projection.perspective.near = 1;
    ctx->projection.perspective.aspect = 1.3333;
    ctx->projection.perspective.post_translate_x = 0;
    ctx->projection.perspective.post_translate_y = 0;
    
}

/* Routine to allocate and return a new draw context */
pmdl_draw_context_t* pmdl_new_draw_context() {
    
    pmdl_draw_context_t* ctx = pspl_allocate_indexing_block(sizeof(pmdl_draw_context_t));
    _pmdl_set_default_context(ctx);
    return ctx;
    
}

/* Routine to free draw context */
void pmdl_free_draw_context(pmdl_draw_context_t* context) {
    pspl_free_indexing_block(context);
}

/* Routine to allocate and return a (NULL-terminated) array of new draw contexts */
pmdl_draw_context_t* pmdl_new_draw_context_array(unsigned count) {
    
    pmdl_draw_context_t* array = pspl_allocate_indexing_block(sizeof(pmdl_draw_context_t) * count);

    int i;
    for (i=0 ; i<count ; ++i)
        _pmdl_set_default_context(&array[i]);
    
    return array;
    
}

/* Routine to free said (NULL-terminated) array */
void pmdl_free_draw_context_array(pmdl_draw_context_t* array) {
    pspl_free_indexing_block(array);
}


#pragma mark Context Representation and Frustum Testing

/* Invalidate context transformation cache (if values updated) */
void pmdl_update_context(pmdl_draw_context_t* ctx, enum pmdl_invalidate_bits inv_bits) {
    
    if (!inv_bits)
        return;
    
    if (inv_bits & PMDL_INVALIDATE_VIEW) {
        pmdl_matrix_lookat(&ctx->cached_view_mtx, ctx->camera_view);
    }
    
    if (inv_bits & (PMDL_INVALIDATE_MODEL | PMDL_INVALIDATE_VIEW)) {
        
        pmdl_matrix34_mul(&ctx->model_mtx, &ctx->cached_view_mtx, &ctx->cached_modelview_mtx.m34);
        pmdl_matrix34_mul(&ctx->cached_modelview_mtx.m34, (pspl_matrix34_t*)&LH_SWIZZLE_MATRIX, &ctx->cached_modelview_mtx.m34);
        pmdl_vector4_cpy(HOMOGENOUS_BOTTOM_VECTOR, ctx->cached_modelview_mtx.v[3]);
        
        pmdl_matrix34_invxpose(&ctx->cached_modelview_mtx.m34, &ctx->cached_modelview_invxpose_mtx.m34);
        pmdl_vector4_cpy(HOMOGENOUS_BOTTOM_VECTOR, ctx->cached_modelview_invxpose_mtx.v[3]);

    }
    
    if (inv_bits & PMDL_INVALIDATE_PROJECTION) {
        if (ctx->projection_type == PMDL_PERSPECTIVE) {
            pmdl_matrix_perspective(&ctx->cached_projection_mtx, ctx->projection.perspective);
            ctx->f_tanv = tanf(DegToRad(ctx->projection.perspective.fov));
            ctx->f_tanh = ctx->f_tanv * ctx->projection.perspective.aspect;
        } else if (ctx->projection_type == PMDL_ORTHOGRAPHIC) {
            pmdl_matrix_orthographic(&ctx->cached_projection_mtx, ctx->projection.orthographic);
        }
    }
    
}

/* Perform AABB frustum test */
static int pmdl_aabb_frustum_test(const pmdl_draw_context_t* ctx, float aabb[2][3]) {
    
    // Setup straddle-test state
    enum {
        TOO_FAR = 1,
        TOO_NEAR = 1<<1,
        TOO_LEFT = 1<<2,
        TOO_RIGHT = 1<<3,
        TOO_UP = 1<<4,
        TOO_DOWN = 1<<5
    } straddle = 0;
    
    // Compute using 8 points of AABB
    int i;
    for (i=0 ; i<8 ; ++i) {
        
        // Select point
        pspl_vector3_t point = {.f[0]=aabb[i/4][0], .f[1]=aabb[(i/2)&1][1], .f[2]=aabb[i&1][2]};
        
        // Transform point into view->point vector
        pmdl_vector3_matrix_mul(&ctx->cached_modelview_mtx.m34, &point, &point);
        
        // If orthographic, perform some simple tests
        if (ctx->projection_type == PMDL_ORTHOGRAPHIC) {
            
            if (-point.f[2] > ctx->projection.orthographic.far) {
                straddle |= TOO_FAR;
                continue;
            }
            if (-point.f[2] < ctx->projection.orthographic.near) {
                straddle |= TOO_NEAR;
                continue;
            }
            
            if (point.f[0] > ctx->projection.orthographic.right) {
                straddle |= TOO_RIGHT;
                continue;
            }
            if (point.f[0] < ctx->projection.orthographic.left) {
                straddle |= TOO_LEFT;
                continue;
            }
            
            if (point.f[1] > ctx->projection.orthographic.top) {
                straddle |= TOO_UP;
                continue;
            }
            if (point.f[1] < ctx->projection.orthographic.bottom) {
                straddle |= TOO_DOWN;
                continue;
            }
            
            // Point is in frustum
            return 1;
            
        }
        
        // Perspective testing otherwise
        else if (ctx->projection_type == PMDL_PERSPECTIVE) {
            
            // See if vector exceeds far plane or comes too near
            if (-point.f[2] > ctx->projection.perspective.far) {
                straddle |= TOO_FAR;
                continue;
            }
            if (-point.f[2] < ctx->projection.perspective.near) {
                straddle |= TOO_NEAR;
                continue;
            }
            
            // Use radar-test to see if point's Y coordinate is within frustum
            float y_threshold = ctx->f_tanv * (-point.f[2]);
            if (point.f[1] > y_threshold) {
                straddle |= TOO_UP;
                continue;
            }
            if (point.f[1] < -y_threshold) {
                straddle |= TOO_DOWN;
                continue;
            }
            
            // Same for X coordinate
            float x_threshold = ctx->f_tanh * (-point.f[2]);
            if (point.f[0] > x_threshold) {
                straddle |= TOO_RIGHT;
                continue;
            }
            if (point.f[0] < -x_threshold) {
                straddle |= TOO_LEFT;
                continue;
            }
            
            // Point is in frustum
            return 1;
            
        }
        
    }
    
    // Perform evaluation of straddle results as last resort
    int p = 0;
    if (straddle & TOO_FAR && straddle & TOO_NEAR)
        ++p;
    if (straddle & TOO_LEFT && straddle & TOO_RIGHT)
        ++p;
    if (straddle & TOO_UP && straddle & TOO_DOWN)
        ++p;
    
    int d = 0;
    if (LXOR(straddle & TOO_FAR, straddle & TOO_NEAR))
        ++d;
    if (LXOR(straddle & TOO_LEFT, straddle & TOO_RIGHT))
        ++d;
    if (LXOR(straddle & TOO_UP, straddle & TOO_DOWN))
        ++d;
    
    return ((p-d) >= 1);
    
}


#pragma mark PMDL Drawing

/* Resolve primitive enumeration */
#if PSPL_RUNTIME_PLATFORM_GL2
static inline GLenum resolve_prim(uint32_t prim) {
    if (prim == PMDL_TRIANGLE_STRIPS)
        return GL_TRIANGLE_STRIP;
    else if (prim == PMDL_TRIANGLE_FANS)
        return GL_TRIANGLE_FAN;
    else if (prim == PMDL_TRIANGLES)
        return GL_TRIANGLES;
    else if (prim == PMDL_LINE_STRIPS)
        return GL_LINE_STRIP;
    else if (prim == PMDL_LINES)
        return GL_LINES;
    return GL_POINTS;
}
#elif PSPL_RUNTIME_PLATFORM_D3D11
static inline int resolve_prim(uint32_t prim) {
    
}
#endif


/* Set NULL shader (some sort of fixed-function preset) */
#if PSPL_RUNTIME_PLATFORM_GL2
static inline void null_shader(const pmdl_draw_context_t* ctx) {
    glUseProgram(0);
}
#elif PSPL_RUNTIME_PLATFORM_GX
static inline void null_shader(pmdl_draw_context_t* ctx) {
    
}
#elif PSPL_RUNTIME_PLATFORM_D3D11
static inline void null_shader() {
    
}
#endif

/* This routine will draw PAR0 PMDLs */
static void pmdl_draw_par0(pmdl_draw_context_t* ctx, const pmdl_t* pmdl) {
    pmdl_header* header = pmdl->file_ptr->file_data;

    int i,j,k;
    
#   if PMDL_GX
        // Load in GX transformation context here
        GX_LoadPosMtxImm(ctx->cached_modelview_mtx.m, GX_PNMTX0);
        GX_LoadNrmMtxImm(ctx->cached_modelview_invxpose_mtx.m, GX_PNMTX0);
        GX_LoadTexMtxImm(ctx->cached_modelview_invxpose_mtx.m, GX_TEXMTX9, GX_MTX3x4);
        
        unsigned gx_loaded_texcoord_mats = 0;
        
        GX_LoadProjectionMtx(ctx->cached_projection_mtx.m,
                             (ctx->projection_type == PMDL_PERSPECTIVE)?
                             GX_PERSPECTIVE:GX_ORTHOGRAPHIC);
    
#   endif
    
    
    void* collection_buf = pmdl->file_ptr->file_data + header->collection_offset;
    pmdl_col_header* collection_header = collection_buf;
    for (i=0 ; i<header->collection_count; ++i) {
        
        void* index_buf = collection_buf + collection_header->draw_idx_off;
        
        // Mesh count and index buf offset
        uint32_t mesh_count = *(uint32_t*)index_buf;
        uint32_t index_buf_offset = *(uint32_t*)(index_buf+4);
        
        // Mesh head array
        pmdl_mesh_header* mesh_heads = index_buf+8;
        index_buf += index_buf_offset;
        
#       if PMDL_GENERAL
        
            // Platform specific index buffer array
#           if PSPL_RUNTIME_PLATFORM_GL2
                struct gl_bufs_t* gl_bufs = index_buf;
                GLVAO(glBindVertexArray)(gl_bufs->vao);
                glBindBuffer(GL_ARRAY_BUFFER, gl_bufs->vert_buf);
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gl_bufs->elem_buf);
#           elif PSPL_RUNTIME_PLATFORM_D3D11
            
#           endif
            index_buf += header->pointer_size*3;

            
            for (j=0 ; j<mesh_count ; ++j) {
                pmdl_mesh_header* mesh_head = &mesh_heads[j];
                
                // Primitive count
                uint32_t prim_count = *(uint32_t*)index_buf;
                index_buf += sizeof(uint32_t);
                
                // Frustum test
                if (!pmdl_aabb_frustum_test(ctx, mesh_head->mesh_aabb)) {
                    index_buf += sizeof(pmdl_general_prim) * prim_count;
                    continue;
                }
                
                // Apply mesh context
                const pspl_runtime_psplc_t* shader_obj = NULL;
                if (mesh_head->shader_index < 0) {
                    if (ctx->default_shader)
                        shader_obj = ctx->default_shader;
                    else
                        null_shader(ctx);
                } else
                    shader_obj = mesh_head->shader_pointer;
                
                if (shader_obj) {
                    pspl_runtime_bind_psplc(shader_obj);
                    
#                   if PSPL_RUNTIME_PLATFORM_GL2
                        glUniformMatrix4fv(shader_obj->native_shader.mv_mtx_uni, 1, GL_FALSE, (GLfloat*)ctx->cached_modelview_mtx.m);
                        glUniformMatrix4fv(shader_obj->native_shader.mv_invxpose_uni, 1, GL_FALSE, (GLfloat*)ctx->cached_modelview_invxpose_mtx.m);
                        glUniformMatrix4fv(shader_obj->native_shader.proj_mtx_uni, 1, GL_FALSE, (GLfloat*)ctx->cached_projection_mtx.m);
                        glUniformMatrix4fv(shader_obj->native_shader.tc_genmtx_arr,
                                           shader_obj->native_shader.config->texgen_count,
                                           GL_FALSE, (GLfloat*)ctx->texcoord_mtx);
                    
#                   elif PSPL_RUNTIME_PLATFORM_D3D11
                    
#                   endif
                }
                
                // Iterate primitives
                for (k=0 ; k<prim_count ; ++k) {
                    pmdl_general_prim* prim = index_buf;
                    index_buf += sizeof(pmdl_general_prim);
#                   if PSPL_RUNTIME_PLATFORM_GL2
                        glDrawElements(resolve_prim(prim->prim_type), prim->prim_count, GL_UNSIGNED_SHORT,
                                       (GLvoid*)(GLsizeiptr)(prim->prim_start_idx*2));
#                   elif PSPL_RUNTIME_PLATFORM_D3D11
                    
#                   endif
                    
                }
                
            }
            

        
#       elif PMDL_GX
        
            // Set GX Attribute Table
            GX_ClearVtxDesc();
            
            GX_SetVtxDesc(GX_VA_POS, GX_INDEX16);
            GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
        
            GX_SetVtxDesc(GX_VA_NRM, GX_INDEX16);
            GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_NRM, GX_NRM_XYZ, GX_F32, 0);

            for (j=0 ; j<collection_header->uv_count ; ++j) {
                GX_SetVtxDesc(GX_VA_TEX0+j, GX_INDEX16);
                GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0+j, GX_TEX_ST, GX_F32, 0);
            }
            
            
            // Load in GX buffer context here
            void* vert_buf = collection_buf + collection_header->vert_buf_off;
            uint32_t vert_count = *(uint32_t*)vert_buf;
            uint32_t loop_vert_count = *(uint32_t*)(vert_buf+4);
            vert_buf += 32;
            GX_SetArray(GX_VA_POS, vert_buf, 12);
            vert_buf += vert_count * 12;
            GX_SetArray(GX_VA_NRM, vert_buf, 12);
            vert_buf += vert_count * 12;
        
            for (j=0 ; j<collection_header->uv_count ; ++j) {
                GX_SetArray(GX_VA_TEX0+j, vert_buf, 8);
                vert_buf += loop_vert_count * 8;
            }
        
            GX_InvVtxCache();
        
            // Offset anchor for display list buffers
            void* buf_anchor = index_buf;
        
            // Iterate each mesh
            for (j=0 ; j<mesh_count ; ++j) {
                pmdl_mesh_header* mesh_head = &mesh_heads[j];
                pmdl_gx_mesh* gx_mesh = index_buf;
                
                // Frustum test
                if (!pmdl_aabb_frustum_test(ctx, mesh_head->mesh_aabb)) {
                    index_buf += sizeof(pmdl_gx_mesh);
                    continue;
                }
                
                // Apply mesh context
                const pspl_runtime_psplc_t* shader_obj = NULL;
                if (mesh_head->shader_index < 0) {
                    if (ctx->default_shader)
                        shader_obj = ctx->default_shader;
                    else
                        null_shader(ctx);
                } else
                    shader_obj = mesh_head->shader_pointer;
                
                if (shader_obj) {
                    
                    // Load remaining texture coordinate matrices here
                    if (shader_obj->native_shader.texgen_count > gx_loaded_texcoord_mats) {
                        for (k=gx_loaded_texcoord_mats ; k<shader_obj->native_shader.texgen_count ; ++k) {
                            GX_LoadTexMtxImm(ctx->texcoord_mtx[k].m, GX_TEXMTX0 + (k*3), GX_MTX2x4);
                            if (shader_obj->native_shader.using_texcoord_normal)
                                GX_LoadTexMtxImm(ctx->texcoord_mtx[k].m, GX_DTTMTX0 + (k*3), GX_MTX3x4);
                        }
                        gx_loaded_texcoord_mats = shader_obj->native_shader.texgen_count;
                    }
                    
                    if (shader_obj->native_shader.using_texcoord_normal)
                        GX_LoadTexMtxImm(ctx->cached_modelview_invxpose_mtx.m, GX_TEXMTX9, GX_MTX3x4);

                    
                    // Execute shader Display List
                    pspl_runtime_bind_psplc(shader_obj);

                }
                
                //GX_SetTexCoordGen2(GX_TEXCOORD0, GX_TG_MTX3x4, GX_TG_NRM, GX_TEXMTX9, GX_TRUE, GX_DTTMTX0);
                //GX_SetNumTexGens(1);
                
                /*
                GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLORNULL);
                GX_SetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
                GX_SetTevColorIn(GX_TEVSTAGE0, GX_CC_TEXC, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO);
                //GX_SetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
                //GX_SetTevAlphaIn(GX_TEVSTAGE0, GX_CA_TEXA, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO);
                
                GX_SetTevOrder(GX_TEVSTAGE1, GX_TEXCOORD0, GX_TEXMAP1, GX_COLORNULL);
                GX_SetTevColorOp(GX_TEVSTAGE1, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
                GX_SetTevColorIn(GX_TEVSTAGE1, GX_CC_TEXC, GX_CC_ZERO, GX_CC_ZERO, GX_CC_CPREV);
                //GX_SetTevAlphaOp(GX_TEVSTAGE1, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
                //GX_SetTevAlphaIn(GX_TEVSTAGE1, GX_CA_TEXA, GX_CA_ZERO, GX_CA_ZERO, GX_CA_APREV);
                */
                 
                //GX_SetNumTevStages(2);
                
                
                // Draw Mesh
                GX_CallDispList(buf_anchor + gx_mesh->dl_offset, gx_mesh->dl_length);
                
                index_buf += sizeof(pmdl_gx_mesh);
                
            }
        
#       endif
        
        // ADVANCE!!
        collection_header += sizeof(pmdl_col_header);
        
    }
}

/* This routine will draw PAR1 PMDLs */
void pmdl_draw_rigged(const pmdl_draw_context_t* ctx, const pmdl_t* pmdl,
                      const pmdl_animation_ctx* anim_ctx) {
    pmdl_header* header = pmdl->file_ptr->file_data;
    if (header->sub_type_num != '1')
        return;

    int i,j,k,l;
    
    
#   if PMDL_GX
        // Load in GX transformation context here
        GX_LoadPosMtxImm(ctx->cached_modelview_mtx.m, GX_PNMTX0);
        GX_LoadNrmMtxImm(ctx->cached_modelview_invxpose_mtx.m, GX_PNMTX0);
        GX_LoadTexMtxImm(ctx->cached_modelview_invxpose_mtx.m, GX_TEXMTX9, GX_MTX3x4);
        
        unsigned gx_loaded_texcoord_mats = 0;
        
        GX_LoadProjectionMtx(ctx->cached_projection_mtx.m,
                             (ctx->projection_type == PMDL_PERSPECTIVE)?
                             GX_PERSPECTIVE:GX_ORTHOGRAPHIC);
    
#   endif
    
    
    void* collection_buf = pmdl->file_ptr->file_data + header->collection_offset;
    pmdl_col_header* collection_header = collection_buf;
    for (i=0 ; i<header->collection_count; ++i) {
        
        void* index_buf = collection_buf + collection_header->draw_idx_off;
        
        // Mesh count and index buf offset
        uint32_t mesh_count = *(uint32_t*)index_buf;
        uint32_t index_buf_offset = *(uint32_t*)(index_buf+4);
        
        // Mesh head array
        pmdl_mesh_header* mesh_heads = index_buf+8;
        index_buf += index_buf_offset;
        
#       if PMDL_GENERAL
        
            // Platform specific index buffer array
#           if PSPL_RUNTIME_PLATFORM_GL2
                struct gl_bufs_t* gl_bufs = index_buf;
                GLVAO(glBindVertexArray)(gl_bufs->vao);
                glBindBuffer(GL_ARRAY_BUFFER, gl_bufs->vert_buf);
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gl_bufs->elem_buf);
#           elif PSPL_RUNTIME_PLATFORM_D3D11
            
#           endif
            index_buf += header->pointer_size*3;

            
            for (j=0 ; j<mesh_count ; ++j) {
                pmdl_mesh_header* mesh_head = &mesh_heads[j];
                
                // Primitive count
                uint32_t prim_count = *(uint32_t*)index_buf;
                index_buf += sizeof(uint32_t);
                
                // Frustum test
                if (!pmdl_aabb_frustum_test(ctx, mesh_head->mesh_aabb)) {
                    index_buf += sizeof(pmdl_general_prim_par1) * prim_count;
                    continue;
                }
                
                // Apply mesh context
                const pspl_runtime_psplc_t* shader_obj = NULL;
                if (mesh_head->shader_index < 0) {
                    if (ctx->default_shader)
                        shader_obj = ctx->default_shader;
                    else
                        null_shader(ctx);
                } else
                    shader_obj = mesh_head->shader_pointer;
                
                if (shader_obj) {
                    pspl_runtime_bind_psplc(shader_obj);
                    
#                   if PSPL_RUNTIME_PLATFORM_GL2
                        glUniformMatrix4fv(shader_obj->native_shader.mv_mtx_uni, 1, GL_FALSE, (GLfloat*)ctx->cached_modelview_mtx.m);
                        glUniformMatrix4fv(shader_obj->native_shader.mv_invxpose_uni, 1, GL_FALSE, (GLfloat*)ctx->cached_modelview_invxpose_mtx.m);
                        glUniformMatrix4fv(shader_obj->native_shader.proj_mtx_uni, 1, GL_FALSE, (GLfloat*)ctx->cached_projection_mtx.m);
                        glUniformMatrix4fv(shader_obj->native_shader.tc_genmtx_arr,
                                           shader_obj->native_shader.config->texgen_count,
                                           GL_FALSE, (GLfloat*)ctx->texcoord_mtx);
                    
#                   elif PSPL_RUNTIME_PLATFORM_D3D11
                    
#                   endif
                }
                
                // Iterate primitives
                int32_t last_skin_index = -1;
                for (k=0 ; k<prim_count ; ++k) {
                    pmdl_general_prim_par1* prim = index_buf;
                    index_buf += sizeof(pmdl_general_prim_par1);
                    
                    // Load evaluated bones from skinning entry into GPU
                    pspl_matrix44_t bone_mats[PMDL_MAX_BONES];
                    pspl_vector4_t bone_bases[PMDL_MAX_BONES];
                    if (last_skin_index != prim->skin_idx) {
                        last_skin_index = prim->skin_idx;
                        if (anim_ctx) {
                            const pmdl_skin_entry* skin_entry =
                            &anim_ctx->parent_ctx->skin_entry_array[last_skin_index];
                            for (l=0 ; l<skin_entry->bone_count ; ++l) {
                                const pmdl_bone* bone = skin_entry->bone_array[l];
                                const pmdl_fk_playback* bone_fk = &anim_ctx->fk_instance_array[bone->bone_index];
                                pmdl_matrix34_cpy(bone_fk->bone_matrix->v, bone_mats[l].v);
                                pmdl_vector4_cpy(HOMOGENOUS_BOTTOM_VECTOR, bone_mats[l].v[3]);
                                pmdl_vector4_cpy(bone->base_vector->v, bone_bases[l].v);
                            }
#                           if PSPL_RUNTIME_PLATFORM_GL2
                                glUniformMatrix4fv(shader_obj->native_shader.bone_mat_uni,
                                                   skin_entry->bone_count, GL_FALSE, (GLfloat*)bone_mats);
                                glUniform4fv(shader_obj->native_shader.bone_base_uni, skin_entry->bone_count,
                                             (GLfloat*)bone_bases);
#                           elif PSPL_RUNTIME_PLATFORM_D3D11
                            
#                           endif
                            
                        } else {
#                           if PSPL_RUNTIME_PLATFORM_GL2
                                glUniformMatrix4fv(shader_obj->native_shader.bone_mat_uni, PMDL_MAX_BONES,
                                                   GL_FALSE, (GLfloat*)IDENTITY_MATS);
#                           elif PSPL_RUNTIME_PLATFORM_D3D11
                            
#                           endif
                        }

                    }
                    
#                   if PSPL_RUNTIME_PLATFORM_GL2
                        glDrawElements(resolve_prim(prim->prim.prim_type), prim->prim.prim_count, GL_UNSIGNED_SHORT,
                                       (GLvoid*)(GLsizeiptr)(prim->prim.prim_start_idx*2));
#                   elif PSPL_RUNTIME_PLATFORM_D3D11
                    
#                   endif
                    
                        

                }
                
            }
        

        
#       elif PMDL_GX
        
            // Set GX Attribute Table
            GX_ClearVtxDesc();
            
            GX_SetVtxDesc(GX_VA_POS, GX_INDEX16);
            GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
        
            GX_SetVtxDesc(GX_VA_NRM, GX_INDEX16);
            GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_NRM, GX_NRM_XYZ, GX_F32, 0);

            for (j=0 ; j<collection_header->uv_count ; ++j) {
                GX_SetVtxDesc(GX_VA_TEX0+j, GX_INDEX16);
                GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0+j, GX_TEX_ST, GX_F32, 0);
            }
        
        //printf("PRE RIG\n");
            
            
            // Load in GX buffer context here
            void* vert_buf = collection_buf + collection_header->vert_buf_off;
            uint32_t par1_off = *(uint32_t*)vert_buf;
            void* par1_cur = vert_buf + par1_off;
        
            uint32_t vert_count = *(uint32_t*)(vert_buf+4);
            uint32_t loop_vert_count = *(uint32_t*)(vert_buf+8);
            vert_buf += 32;
        
            if (anim_ctx) {
            
                // Compute rigged vertex mesh
                pmdl_gx_par1_vertbuf_head* vertbuf_head = par1_cur;
                par1_cur += sizeof(pmdl_gx_par1_vertbuf_head);
                
                // Position and normal stage
                f32* vert_cur = vert_buf;
                f32* norm_cur = (vert_buf + (vert_count*12));
            //printf("VERT COUNT %u\n", vert_count);
                for (j=0 ; j<vert_count ; ++j) {
                    
                    f32* target_vert = &vertbuf_head->position_stage[3*j];
                    f32* target_norm = &vertbuf_head->normal_stage[3*j];
                    pmdl_gx_par1_vert_head* vert_head = par1_cur;
                    
                    // Initialise mesh stage with identity blend
                    guVecScale((guVector*)vert_cur, (guVector*)target_vert, vert_head->identity_blend);
                    guVecScale((guVector*)norm_cur, (guVector*)target_norm, vert_head->identity_blend);
                    
                    // Iterate bones and deform mesh stage accordingly
                    unsigned k;
                    for (k=0 ; k<vert_head->bone_count ; ++k) {
                        
                        struct pmdl_gx_par1_vert_head_bone* vhb = &vert_head->bone_arr[k];
                        const pmdl_bone* bone = &pmdl->rigging_ptr->bone_array[vhb->bone_idx];
                        
                        guVector transformed_vert, transformed_norm;
                        guVecSub((guVector*)vert_cur, (guVector*)bone->base_vector->f, &transformed_vert);
                        
                        guVecMultiply(anim_ctx->fk_instance_array[vhb->bone_idx].bone_matrix->m, &transformed_vert, &transformed_vert);
                        guVecMultiply(anim_ctx->fk_instance_array[vhb->bone_idx].bone_matrix->m, (guVector*)norm_cur, &transformed_norm);
                        
                        guVecScale(&transformed_vert, &transformed_vert, vhb->bone_weight);
                        guVecScale(&transformed_norm, &transformed_norm, vhb->bone_weight);
                        
                        guVecAdd((guVector*)target_vert, &transformed_vert, (guVector*)target_vert);
                        guVecAdd((guVector*)target_norm, &transformed_norm, (guVector*)target_norm);

                    }
                    
                    guVecNormalize((guVector*)target_norm);
                    
                    // ADVANCE!!
                    par1_cur += 8*vert_head->bone_count + 8;
                    vert_cur += 3;
                    norm_cur += 3;
                    
                }
            
            //printf("POST RIG\n");
            
                GX_SetArray(GX_VA_POS, vertbuf_head->position_stage, 12);
                vert_buf += vert_count * 12;
                GX_SetArray(GX_VA_NRM, vertbuf_head->normal_stage, 12);
                vert_buf += vert_count * 12;
            
            } else {
                
                GX_SetArray(GX_VA_POS, vert_buf, 12);
                vert_buf += vert_count * 12;
                GX_SetArray(GX_VA_NRM, vert_buf, 12);
                vert_buf += vert_count * 12;
                
            }
        
            for (j=0 ; j<collection_header->uv_count ; ++j) {
                GX_SetArray(GX_VA_TEX0+j, vert_buf, 8);
                vert_buf += loop_vert_count * 8;
            }
        
            GX_InvVtxCache();
        
        //printf("SET ARRAYS\n");
        
            // Offset anchor for display list buffers
            void* buf_anchor = index_buf;
        
            // Iterate each mesh
            for (j=0 ; j<mesh_count ; ++j) {
                pmdl_mesh_header* mesh_head = &mesh_heads[j];
                pmdl_gx_mesh* gx_mesh = index_buf;
                
                // Frustum test
                if (!pmdl_aabb_frustum_test(ctx, mesh_head->mesh_aabb)) {
                    index_buf += sizeof(pmdl_gx_mesh);
                    continue;
                }
                
                // Apply mesh context
                const pspl_runtime_psplc_t* shader_obj = NULL;
                if (mesh_head->shader_index < 0) {
                    if (ctx->default_shader)
                        shader_obj = ctx->default_shader;
                    else
                        null_shader(ctx);
                } else
                    shader_obj = mesh_head->shader_pointer;
                
                if (shader_obj) {
                    
                    // Load remaining texture coordinate matrices here
                    if (shader_obj->native_shader.texgen_count > gx_loaded_texcoord_mats) {
                        for (k=gx_loaded_texcoord_mats ; k<shader_obj->native_shader.texgen_count ; ++k) {
                            GX_LoadTexMtxImm(ctx->texcoord_mtx[k].m, GX_TEXMTX0 + (k*3), GX_MTX2x4);
                            if (shader_obj->native_shader.using_texcoord_normal)
                                GX_LoadTexMtxImm(ctx->texcoord_mtx[k].m, GX_DTTMTX0 + (k*3), GX_MTX3x4);
                        }
                        gx_loaded_texcoord_mats = shader_obj->native_shader.texgen_count;
                    }
                    
                    if (shader_obj->native_shader.using_texcoord_normal)
                        GX_LoadTexMtxImm(ctx->cached_modelview_invxpose_mtx.m, GX_TEXMTX9, GX_MTX3x4);

                    
                    // Execute shader Display List
                    //printf("PRE IR\n");
                    pspl_runtime_bind_psplc(shader_obj);
                    //printf("POST IR\n");

                }
                
                //GX_SetTexCoordGen2(GX_TEXCOORD0, GX_TG_MTX3x4, GX_TG_NRM, GX_TEXMTX9, GX_TRUE, GX_DTTMTX0);
                //GX_SetNumTexGens(1);
                
                /*
                GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLORNULL);
                GX_SetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
                GX_SetTevColorIn(GX_TEVSTAGE0, GX_CC_TEXC, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO);
                //GX_SetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
                //GX_SetTevAlphaIn(GX_TEVSTAGE0, GX_CA_TEXA, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO);
                
                GX_SetTevOrder(GX_TEVSTAGE1, GX_TEXCOORD0, GX_TEXMAP1, GX_COLORNULL);
                GX_SetTevColorOp(GX_TEVSTAGE1, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
                GX_SetTevColorIn(GX_TEVSTAGE1, GX_CC_TEXC, GX_CC_ZERO, GX_CC_ZERO, GX_CC_CPREV);
                //GX_SetTevAlphaOp(GX_TEVSTAGE1, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
                //GX_SetTevAlphaIn(GX_TEVSTAGE1, GX_CA_TEXA, GX_CA_ZERO, GX_CA_ZERO, GX_CA_APREV);
                */
                 
                //GX_SetNumTevStages(2);
                
                
                // Draw Mesh
                //printf("PRE DRAW\n");
                GX_CallDispList(buf_anchor + gx_mesh->dl_offset, gx_mesh->dl_length);
                //printf("POST DRAW\n");
                
                index_buf += sizeof(pmdl_gx_mesh);
                
            }
        
#       endif
        
        // ADVANCE!!
        collection_header += sizeof(pmdl_col_header);
        
    }
}

/* This routine will draw PAR2 PMDLs */
static void pmdl_draw_par2(pmdl_draw_context_t* ctx, const pmdl_t* pmdl) {
    
}

/* This is the main draw dispatch routine */
void pmdl_draw(pmdl_draw_context_t* ctx, const pmdl_t* pmdl) {
    pmdl_header* header = pmdl->file_ptr->file_data;

    // Master Frustum test
    if (!pmdl_aabb_frustum_test(ctx, header->master_aabb))
        return;

    
    // Select draw routine based on sub-type
    if (header->sub_type_num == '0')
        pmdl_draw_par0(ctx, pmdl);
    if (header->sub_type_num == '1')
        pmdl_draw_rigged(ctx, pmdl, NULL);
    else if (header->sub_type_num == '2')
        pmdl_draw_par2(ctx, pmdl);
    
        
}


