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
#include "PMDLRuntimeLinAlgebra.h"


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

#endif


/* If using OpenGL OR Direct3D, we're using the General Draw Format */
#if PSPL_RUNTIME_PLATFORM_GL2 || PSPL_RUNTIME_PLATFORM_D3D11
#   define PMDL_GENERAL 1
    static const char* DRAW_FMT = "_GEN";
#elif PSPL_RUNTIME_PLATFORM_GX
#   define PMDL_GX 1
    static const char* DRAW_FMT = "__GX";
#endif



/* PAR Enum */
enum pmdl_par {
    PMDL_PAR0 = 0,
    PMDL_PAR1 = 1,
    PMDL_PAR2 = 2
};

/* PMDL Header */
typedef struct __attribute__ ((__packed__)) {
    
    char magic[4];
    char endianness[4];
    uint32_t pointer_size;
    char sub_type_prefix[3];
    char sub_type_num;
    char draw_format[4];
    float master_aabb[2][3];
    
    uint32_t collection_offset;
    uint32_t collection_count;
    uint32_t shader_table_offset;
    uint32_t bone_table_offset;
    
    uint32_t padding;
    
} pmdl_header;

/* PMDL Collection Header */
typedef struct __attribute__ ((__packed__)) {
    
    uint16_t uv_count;
    uint16_t bone_count;
    uint32_t vert_buf_off;
    uint32_t vert_buf_len;
    uint32_t elem_buf_off;
    uint32_t elem_buf_len;
    uint32_t draw_idx_off;
    
} pmdl_col_header;

/* PMDL Mesh header */
typedef struct {
    
    float mesh_aabb[2][3];
    int32_t shader_index;
    
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
#endif


#pragma mark PMDL Loading / Unloading

#if PMDL_GENERAL
    /* This routine will load collection data for GENERAL draw format */
    static int pmdl_init_collections_general(void* file_data) {
        pmdl_header* header = file_data;
        
        // Init collections
        void* collection_buf = file_data + header->collection_offset;
        pmdl_col_header* collection_header = collection_buf;
        int i;
        for (i=0 ; i<header->collection_count; ++i) {
            void* index_buf = collection_buf + collection_header->draw_idx_off;
            
            // Mesh count
            uint32_t mesh_count = *(uint32_t*)index_buf;
            index_buf += sizeof(uint32_t);
            
            // Mesh head array
            index_buf += sizeof(pmdl_mesh_header) * mesh_count;
            
#           if PSPL_RUNTIME_PLATFORM_GL2
                // VAO
                GLuint* vao = (GLuint*)index_buf;
                GLVAO(glGenVertexArrays)(1, vao);
                GLVAO(glBindVertexArray)(*vao);
                
                // VBOs
                struct {
                    GLuint vert_buf, elem_buf;
                } gl_bufs;
                glGenBuffers(2, (GLuint*)&gl_bufs);
                glBindBuffer(GL_ARRAY_BUFFER, gl_bufs.vert_buf);
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gl_bufs.elem_buf);
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
                
                unsigned j;
                GLuint idx = 2;
                for (j=0 ; j<collection_header->uv_count ; ++j) { // UVs
                    glEnableVertexAttribArray(idx);
                    glVertexAttribPointer(idx, 2, GL_FLOAT, GL_FALSE, buf_stride, (GLvoid*)(GLsizeiptr)(24+8*j));
                    ++idx;
                }
                
                GLsizeiptr weight_offset = 24 + collection_header->uv_count*8;
                for (j=0 ; j<collection_header->bone_count ; ++j) { // Bone Weight Coefficients
                    glEnableVertexAttribArray(idx);
                    glVertexAttribPointer(idx, 2, GL_FLOAT, GL_FALSE, buf_stride, (GLvoid*)(weight_offset+4*j));
                    ++idx;
                }
            
#           elif PSPL_RUNTIME_PLATFORM_D3D11
            
#           endif
            
            // ADVANCE!!
            collection_header += sizeof(pmdl_col_header);
        }
        
        return 0;
        
    }

    /* This routine will unload collection data for GENERAL draw format */
    static void pmdl_destroy_collections_general(void* file_data) {
        pmdl_header* header = file_data;

        // Init collections
        void* collection_buf = file_data + header->collection_offset;
        pmdl_col_header* collection_header = collection_buf;
        int i;
        for (i=0 ; i<header->collection_count; ++i) {
            void* index_buf = collection_buf + collection_header->draw_idx_off;
            
#           if PSPL_RUNTIME_PLATFORM_GL2
                GLuint* vao = (GLuint*)index_buf;
                GLVAO(glBindVertexArray)(*vao);
                
                struct {
                    GLuint vert_buf, elem_buf;
                } gl_bufs;
                glGetIntegerv(GL_ARRAY_BUFFER_BINDING, (GLint*)&gl_bufs.vert_buf);
                glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, (GLint*)&gl_bufs.elem_buf);
                glDeleteBuffers(2, (GLuint*)&gl_bufs);
                GLVAO(glDeleteVertexArrays)(1, vao);
            
#           elif PSPL_RUNTIME_PLATFORM_D3D11
            
#           endif
            
            // ADVANCE!!
            collection_header += sizeof(pmdl_col_header);
        }
            
    }
#endif

/* This routine will validate and load PMDL data into GPU */
int pmdl_init(pspl_runtime_arc_file_t* pmdl_file) {
    char hash[PSPL_HASH_STRING_LEN];
    
    // First, validate header members
    pmdl_header* header = pmdl_file->file_data;
    
    // Magic
    if (memcmp(header->magic, "PMDL", 4)) {
        pspl_hash_fmt(hash, &pmdl_file->hash);
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
        pspl_hash_fmt(hash, &pmdl_file->hash);
        pspl_warn("Unable to init PMDL", "file `%s` has invalid endianness; skipping", hash);
        return -1;
    }
    
    // Pointer size
    if (header->pointer_size < sizeof(void*)) {
        pspl_hash_fmt(hash, &pmdl_file->hash);
        pspl_warn("Unable to init PMDL", "file `%s` pointer size too small (%d < %lu); skipping",
                  hash, header->pointer_size, (unsigned long)sizeof(void*));
        return -1;
    }
    
    // Draw Format
    if (memcmp(header->draw_format, DRAW_FMT, 4)) {
        pspl_hash_fmt(hash, &pmdl_file->hash);
        pspl_warn("Unable to init PMDL", "file `%s` has `%.4s` draw format, expected `%.4s` format; skipping",
                  hash, header->draw_format, DRAW_FMT);
        return -1;
    }
    
    
    // Decode PAR sub-type
    if (memcmp(header->sub_type_prefix, "PAR", 3)) {
        pspl_hash_fmt(hash, &pmdl_file->hash);
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
        pspl_hash_fmt(hash, &pmdl_file->hash);
        pspl_warn("Unable to init PMDL", "invalid PAR type specified in `%s`; skipping", hash);
        return -1;
    }
    
    // Load shaders into GPU
    int i;
    uint32_t shader_count = *(uint32_t*)(pmdl_file->file_data + header->shader_table_offset);
    pspl_hash* shader_array = pmdl_file->file_data + header->shader_table_offset + sizeof(uint32_t);
    for (i=0 ; i<shader_count ; ++i)
        pspl_runtime_get_psplc_from_hash(pmdl_file->parent, &shader_array[i], 1);
    
    // Load collections into GPU
#   if PMDL_GENERAL
        return pmdl_init_collections_general(pmdl_file->file_data);
#   else
        return 0;
#   endif
    
    

}

/* This routine will unload data from GPU */
void pmdl_destroy(pspl_runtime_arc_file_t* pmdl_file) {
    pmdl_header* header = pmdl_file->file_data;

    // Unload shaders from GPU
    int i;
    uint32_t shader_count = *(uint32_t*)(pmdl_file->file_data + header->shader_table_offset);
    pspl_hash* shader_array = pmdl_file->file_data + header->shader_table_offset + sizeof(uint32_t);
    for (i=0 ; i<shader_count ; ++i) {
        const pspl_runtime_psplc_t* shader_obj =
        pspl_runtime_get_psplc_from_hash(pmdl_file->parent, &shader_array[i], 0);
        pspl_runtime_release_psplc(shader_obj);
    }
    
    // Unload collections from GPU
#   if PMDL_GENERAL
        pmdl_destroy_collections_general(pmdl_file->file_data);
#   endif
    
}


#pragma mark Context Representation and Frustum Testing

/* Invalidate context transformation cache (if values updated) */
void pmdl_update_context(pmdl_draw_context_t* ctx, enum pmdl_invalidate_bits inv_bits) {
    
    if (!inv_bits)
        return;
    
    if (inv_bits & PMDL_INVALIDATE_VIEW) {
        pmdl_matrix_lookat(ctx->cached_view_mtx, ctx->camera_view);
    }
    
    if (inv_bits & (PMDL_INVALIDATE_MODEL | PMDL_INVALIDATE_VIEW)) {
        pmdl_matrix34_mul(ctx->model_mtx, ctx->cached_view_mtx, ctx->cached_modelview_mtx);
#       if GL_ES_VERSION_2_0
            int i;
            for (i=0 ; i<3 ; ++i)
                ctx->cached_modelview_mtx[3][i] = 0;
            ctx->cached_modelview_mtx[3][3] = 1;
#       endif
        pmdl_matrix34_invxpose(ctx->cached_modelview_mtx, ctx->cached_modelview_invxpose_mtx);
#       if GL_ES_VERSION_2_0
            for (i=0 ; i<3 ; ++i)
                ctx->cached_modelview_invxpose_mtx[3][i] = 0;
            ctx->cached_modelview_invxpose_mtx[3][3] = 1;
#       endif
    }
    
    if (inv_bits & PMDL_INVALIDATE_PROJECTION) {
        if (ctx->projection_type == PMDL_PERSPECTIVE) {
            pmdl_matrix_perspective(ctx->cached_projection_mtx, ctx->projection.perspective);
            ctx->f_tanv = tanf(ctx->projection.perspective.fov);
            ctx->f_tanh = ctx->f_tanv * ctx->projection.perspective.aspect;
        } else if (ctx->projection_type == PMDL_ORTHOGRAPHIC) {
            pmdl_matrix_orthographic(ctx->cached_projection_mtx, ctx->projection.orthographic);
        }
    }
    
}

/* Perform AABB frustum test */
static int pmdl_aabb_frustum_test(pmdl_draw_context_t* ctx, float aabb[2][3]) {
    
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
        pspl_vector3_t point = {aabb[i/4][0], aabb[(i/2)&1][1], aabb[i&1][2]};
        
        // Transform point into view->point vector
        pmdl_vector3_matrix_mul(ctx->cached_modelview_mtx, point, point);
        
        // If orthographic, perform some simple tests
        if (ctx->projection_type == PMDL_ORTHOGRAPHIC) {
            
            if (point[2] > ctx->projection.orthographic.far) {
                straddle |= TOO_FAR;
                continue;
            }
            if (point[2] < ctx->projection.orthographic.near) {
                straddle |= TOO_NEAR;
                continue;
            }
            
            if (point[0] > ctx->projection.orthographic.right) {
                straddle |= TOO_RIGHT;
                continue;
            }
            if (point[0] < ctx->projection.orthographic.left) {
                straddle |= TOO_LEFT;
                continue;
            }
            
            if (point[1] > ctx->projection.orthographic.top) {
                straddle |= TOO_UP;
                continue;
            }
            if (point[1] < ctx->projection.orthographic.bottom) {
                straddle |= TOO_DOWN;
                continue;
            }
            
            // Point is in frustum
            return 1;
            
        }
        
        // Perspective testing otherwise
            
        // See if vector exceeds far plane or comes too near
        if (point[2] > ctx->projection.perspective.far) {
            straddle |= TOO_FAR;
            continue;
        }
        if (point[2] < ctx->projection.perspective.near) {
            straddle |= TOO_NEAR;
            continue;
        }
        
        // Use radar-test to see if point's Y coordinate is within frustum
        float y_threshold = ctx->f_tanv * point[2];
        if (point[1] > y_threshold) {
            straddle |= TOO_UP;
            continue;
        }
        if (point[1] < -y_threshold) {
            straddle |= TOO_DOWN;
            continue;
        }
        
        // Same for X coordinate
        float x_threshold = ctx->f_tanh * point[2];
        if (point[0] > x_threshold) {
            straddle |= TOO_RIGHT;
            continue;
        }
        if (point[0] < -x_threshold) {
            straddle |= TOO_LEFT;
            continue;
        }
        
        // Point is in frustum
        return 1;
        
    }
    
    // Perform evaluate straddle results as last resort
    if (straddle & TOO_FAR && straddle & TOO_NEAR)
        return 1;
    if (straddle & TOO_LEFT && straddle & TOO_RIGHT)
        return 1;
    if (straddle & TOO_UP && straddle & TOO_DOWN)
        return 1;
    
    return 0;
    
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
#elif PSPL_RUNTIME_PLATFORM_GX
static inline u8 resolve_prim(uint32_t prim) {
    
}
#elif PSPL_RUNTIME_PLATFORM_D3D11
static inline int resolve_prim(uint32_t prim) {
    
}
#endif


/* Set NULL shader (some sort of fixed-function preset) */
#if PSPL_RUNTIME_PLATFORM_GL2
static inline void null_shader() {
    glUseProgram(0);
}
#elif PSPL_RUNTIME_PLATFORM_GX
static inline void null_shader() {
    
}
#elif PSPL_RUNTIME_PLATFORM_D3D11
static inline void null_shader() {
    
}
#endif


/* This routine will draw PAR0 PMDLs */
static void pmdl_draw_par0(pmdl_draw_context_t* ctx, pspl_runtime_arc_file_t* pmdl_file) {
    pmdl_header* header = pmdl_file->file_data;
    
    // Shader hash array
    pspl_hash* shader_hashes = pmdl_file->file_data + header->shader_table_offset + sizeof(uint32_t);
    
    void* collection_buf = pmdl_file->file_data + header->collection_offset;
    pmdl_col_header* collection_header = collection_buf;
    int i,j,k;
    for (i=0 ; i<header->collection_count; ++i) {
        
        void* index_buf = collection_buf + collection_header->draw_idx_off;
        
        // Mesh count
        uint32_t mesh_count = *(uint32_t*)index_buf;
        index_buf += sizeof(uint32_t);
        
        // Mesh head array
        pmdl_mesh_header* mesh_heads = index_buf;
        index_buf += sizeof(pmdl_mesh_header) * mesh_count;
        
#       if PMDL_GENERAL
        
            // Platform specific index buffer array
#           if PSPL_RUNTIME_PLATFORM_GL2
                GLVAO(glBindVertexArray)(*(GLuint*)index_buf);
#           elif PSPL_RUNTIME_PLATFORM_D3D11
            
#           endif
            index_buf += header->pointer_size;
            
            
            // Iterate meshes
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
                if (mesh_head->shader_index < 0)
                    null_shader();
                else {
                    const pspl_runtime_psplc_t* shader_obj =
                    pspl_runtime_get_psplc_from_hash(pmdl_file->parent, &shader_hashes[mesh_head->shader_index], 0);
                    
                    if (shader_obj) {
                        pspl_runtime_bind_psplc(shader_obj);
                        
#                       if PSPL_RUNTIME_PLATFORM_GL2
#                           if GL_ES_VERSION_2_0
                                glUniformMatrix4fv(shader_obj->native_shader.mv_mtx_uni, 1, GL_FALSE, (GLfloat*)ctx->cached_modelview_mtx);
                                glUniformMatrix4fv(shader_obj->native_shader.mv_invxpose_uni, 1, GL_FALSE, (GLfloat*)ctx->cached_modelview_invxpose_mtx);
                                glUniformMatrix4fv(shader_obj->native_shader.tc_genmtx_arr, 8, GL_FALSE, (GLfloat*)ctx->texcoord_mtx);
#                           else
                                glUniformMatrix3x4fv(shader_obj->native_shader.mv_mtx_uni, 1, GL_FALSE, (GLfloat*)ctx->cached_modelview_mtx);
                                glUniformMatrix3x4fv(shader_obj->native_shader.mv_invxpose_uni, 1, GL_FALSE, (GLfloat*)ctx->cached_modelview_invxpose_mtx);
                                glUniformMatrix3x4fv(shader_obj->native_shader.tc_genmtx_arr, 8, GL_FALSE, (GLfloat*)ctx->texcoord_mtx);
#                           endif
                        
#                       elif PSPL_RUNTIME_PLATFORM_D3D11
                        
#                       endif
                    }
                }
                
                // Iterate primitives
                for (k=0 ; k<prim_count ; ++k) {
                    pmdl_general_prim* prim = index_buf;
                    index_buf += sizeof(pmdl_general_prim);
#                   if PSPL_RUNTIME_PLATFORM_GL2
                        glDrawElements(resolve_prim(prim->prim_type), prim->prim_count, GL_UNSIGNED_SHORT,
                                       (GLvoid*)(GLsizeiptr)prim->prim_start_idx);
#                   elif PSPL_RUNTIME_PLATFORM_D3D11
                    
#                   endif
                    
                }
                
            }
        

        
#       elif PMDL_GX
        
#       endif
        
        // ADVANCE!!
        collection_header += sizeof(pmdl_col_header);
        
    }
}

/* This routine will draw PAR1 PMDLs */
static void pmdl_draw_par1(pmdl_draw_context_t* ctx, pspl_runtime_arc_file_t* pmdl_file) {
    
}

/* This routine will draw PAR2 PMDLs */
static void pmdl_draw_par2(pmdl_draw_context_t* ctx, pspl_runtime_arc_file_t* pmdl_file) {
    
}

/* This is the main draw dispatch routine */
void pmdl_draw(pmdl_draw_context_t* ctx, pspl_runtime_arc_file_t* pmdl_file) {
    pmdl_header* header = pmdl_file->file_data;

    // Master Frustum test
    if (!pmdl_aabb_frustum_test(ctx, header->master_aabb))
        return;
    
    // Select draw routine based on sub-type
    if (header->sub_type_num == '0')
        pmdl_draw_par0(ctx, pmdl_file);
    else if (header->sub_type_num == '1')
        pmdl_draw_par1(ctx, pmdl_file);
    else if (header->sub_type_num == '2')
        pmdl_draw_par2(ctx, pmdl_file);
    
}


