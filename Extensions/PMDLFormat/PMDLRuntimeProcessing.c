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



/* PAR Enum */
enum pmdl_par {
    PMDL_PAR0 = 0,
    PMDL_PAR1 = 1,
    PMDL_PAR2 = 2
};

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
#elif PMDL_GX
    typedef struct {
        uint32_t dl_offset, dl_length;
    } pmdl_gx_mesh;
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
            glVertexPointer(3, GL_FLOAT, buf_stride, 0);
                
                glEnableVertexAttribArray(1); // Normal
                glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, buf_stride, (GLvoid*)12);
            glNormalPointer(GL_FLOAT, buf_stride, (GLvoid*)12);
                
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
int pmdl_init(const pspl_runtime_arc_file_t* pmdl_file) {
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
#   elif PMDL_GX
        DCStoreRange(pmdl_file->file_data, pmdl_file->file_len);
        return 0;
#   else
        return 0;
#   endif
    
    

}

/* This routine will unload data from GPU */
void pmdl_destroy(const pspl_runtime_arc_file_t* pmdl_file) {
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

void print_matrix(pspl_matrix34_t mtx) {
    printf("%f | %f | %f | %f\n"
           "%f | %f | %f | %f\n"
           "%f | %f | %f | %f\n",
           mtx[0][0], mtx[0][1], mtx[0][2], mtx[0][3],
           mtx[1][0], mtx[1][1], mtx[1][2], mtx[1][3],
           mtx[2][0], mtx[2][1], mtx[2][2], mtx[2][3]);
}

/* Invalidate context transformation cache (if values updated) */
void pmdl_update_context(pmdl_draw_context_t* ctx, enum pmdl_invalidate_bits inv_bits) {
    int i;
    
    if (!inv_bits)
        return;
    
    if (inv_bits & PMDL_INVALIDATE_VIEW) {
        pmdl_matrix_lookat(ctx->cached_view_mtx, ctx->camera_view);
//#       ifdef GEKKO
//        DCStoreRange(ctx->cached_view_mtx, sizeof(pspl_matrix34_t));
//#       endif
    }
    
    if (inv_bits & (PMDL_INVALIDATE_MODEL | PMDL_INVALIDATE_VIEW)) {
        /*
        printf("Model:\n");
        print_matrix(ctx->model_mtx);
        
        printf("\nView:\n");
        print_matrix(ctx->cached_view_mtx);
         */
        
        pmdl_matrix34_mul(ctx->model_mtx, ctx->cached_view_mtx, ctx->cached_modelview_mtx);
        for (i=0 ; i<3 ; ++i)
            ctx->cached_modelview_mtx[3][i] = 0;
        ctx->cached_modelview_mtx[3][3] = 1;
        
        /*
        printf("ModelView:\n");
        print_matrix(ctx->cached_modelview_mtx);
        sleep(5);
        */
        

//#       ifdef GEKKO
//        DCStoreRange(ctx->cached_modelview_mtx, sizeof(pspl_matrix34_t));
//#       endif
        

        pmdl_matrix34_invxpose(ctx->cached_modelview_mtx, ctx->cached_modelview_invxpose_mtx);
        for (i=0 ; i<3 ; ++i)
            ctx->cached_modelview_invxpose_mtx[3][i] = 0;
        ctx->cached_modelview_invxpose_mtx[3][3] = 1;
        
    }
    
    if (inv_bits & PMDL_INVALIDATE_PROJECTION) {
        if (ctx->projection_type == PMDL_PERSPECTIVE) {
            pmdl_matrix_perspective(ctx->cached_projection_mtx, ctx->projection.perspective);
            ctx->f_tanv = tanf(DegToRad(ctx->projection.perspective.fov));
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
            
            if (-point[2] > ctx->projection.orthographic.far) {
                straddle |= TOO_FAR;
                continue;
            }
            if (-point[2] < ctx->projection.orthographic.near) {
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
        else if (ctx->projection_type == PMDL_PERSPECTIVE) {
            
            // See if vector exceeds far plane or comes too near
            if (-point[2] > ctx->projection.perspective.far) {
                straddle |= TOO_FAR;
                continue;
            }
            if (-point[2] < ctx->projection.perspective.near) {
                straddle |= TOO_NEAR;
                continue;
            }
            
            // Use radar-test to see if point's Y coordinate is within frustum
            float y_threshold = ctx->f_tanv * (-point[2]);
            if (point[1] > y_threshold) {
                straddle |= TOO_UP;
                continue;
            }
            if (point[1] < -y_threshold) {
                straddle |= TOO_DOWN;
                continue;
            }
            
            // Same for X coordinate
            float x_threshold = ctx->f_tanh * (-point[2]);
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
static inline void null_shader(pmdl_draw_context_t* ctx) {
    glUseProgram(0);
    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixf((GLfloat*)ctx->cached_modelview_mtx);
    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf((GLfloat*)ctx->cached_projection_mtx);
}
#elif PSPL_RUNTIME_PLATFORM_GX
static inline void null_shader(pmdl_draw_context_t* ctx) {
    
}
#elif PSPL_RUNTIME_PLATFORM_D3D11
static inline void null_shader() {
    
}
#endif

static void print_bytes(void* ptr, size_t size) {
    
    int i;
    
    for (i=0 ; i<size ; ++i) {
        
        if (!(i%32))
            
            printf("\n");
        
        printf("%02X", *(uint8_t*)(ptr+i));
        
    }
    
    printf("\n\n");
    
}

/* This routine will draw PAR0 PMDLs */
static void pmdl_draw_par0(pmdl_draw_context_t* ctx, const pspl_runtime_arc_file_t* pmdl_file) {
    pmdl_header* header = pmdl_file->file_data;

    int i,j;

    
    // Shader hash array
    pspl_hash* shader_hashes = pmdl_file->file_data + header->shader_table_offset + sizeof(uint32_t);
    
#   if PMDL_GX
    // Load in GX transformation context here
    GX_LoadPosMtxImm(ctx->cached_modelview_mtx, GX_PNMTX0);
    GX_LoadNrmMtxImm(ctx->cached_modelview_invxpose_mtx, GX_PNMTX0);
    GX_LoadTexMtxImm(ctx->cached_modelview_invxpose_mtx, GX_TEXMTX9, GX_MTX3x4);
    
    unsigned gx_loaded_texcoord_mats = 0;
    
    GX_LoadProjectionMtx(ctx->cached_projection_mtx,
                         (ctx->projection_type == PMDL_PERSPECTIVE)?
                         GX_PERSPECTIVE:GX_ORTHOGRAPHIC);
    
#   endif
    
    
    void* collection_buf = pmdl_file->file_data + header->collection_offset;
    pmdl_col_header* collection_header = collection_buf;
#   if PMDL_GENERAL
    int k,p;
#   endif
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
                } else // TODO: ADD CACHING SPACE TO PMDL FOR SHADER!!!!
                    shader_obj = pspl_runtime_get_psplc_from_hash(pmdl_file->parent, &shader_hashes[mesh_head->shader_index], 0);
                
                if (shader_obj) {
                    pspl_runtime_bind_psplc(shader_obj);
                    
#                   if PSPL_RUNTIME_PLATFORM_GL2
                        glUniformMatrix4fv(shader_obj->native_shader.mv_mtx_uni, 1, GL_FALSE, (GLfloat*)ctx->cached_modelview_mtx);
                        glUniformMatrix4fv(shader_obj->native_shader.mv_invxpose_uni, 1, GL_FALSE, (GLfloat*)ctx->cached_modelview_invxpose_mtx);
                        glUniformMatrix4fv(shader_obj->native_shader.proj_mtx_uni, 1, GL_FALSE, (GLfloat*)ctx->cached_projection_mtx);
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
            int k;
        
            // Set GX Attribute Table
            GX_ClearVtxDesc();
            
            //GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
            //GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
        
            
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
                } else // TODO: ADD CACHING SPACE TO PMDL FOR SHADER!!!!
                    shader_obj = pspl_runtime_get_psplc_from_hash(pmdl_file->parent, &shader_hashes[mesh_head->shader_index], 0);
                
                if (shader_obj) {
                    
                    // Load remaining texture coordinate matrices here
                    if (shader_obj->native_shader.config->texgen_count > gx_loaded_texcoord_mats)
                        for (k=gx_loaded_texcoord_mats ; k<shader_obj->native_shader.config->texgen_count ; ++k)
                            GX_LoadTexMtxImm(ctx->texcoord_mtx[k], GX_DTTMTX0 + (k*3), GX_TG_MTX2x4);
                    
                    // Execute shader Display List
                    //pspl_runtime_bind_psplc(shader_obj);

                }
                
                // Draw mesh
                //printf("About to run:\n");
                //sleep(2);
                //print_bytes(buf_anchor + gx_mesh->dl_offset, gx_mesh->dl_length);
                //sleep(10);
                
                GX_SetNumTexGens(1);
                GX_SetTexCoordGen2(GX_TEXCOORD0, GX_TG_MTX3x4, GX_TG_NRM, GX_TEXMTX9, GX_TRUE, GX_DTTMTX0);
                
                GX_SetNumTevStages(1);
                GXColor tev_color = {255, 255, 255, 255};
                GX_SetTevKColor(GX_KCOLOR0, tev_color);
                GX_SetTevKColorSel(GX_TEVSTAGE0, GX_TEV_KCSEL_K0);
                GX_SetTevKAlphaSel(GX_TEVSTAGE0, GX_TEV_KASEL_1);
                GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXMAP0, GX_TEXCOORD0, GX_COLORNULL);
                GX_SetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
                GX_SetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
                GX_SetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, GX_CC_TEXC);
                GX_SetTevAlphaIn(GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_KONST);
                
                
                /*
                GX_Begin(GX_QUADS, 0, 4);
                GX_Position3f32(-1, 2, 0);
                GX_Position3f32(-1, -2, 0);
                GX_Position3f32(1, -2, 0);
                GX_Position3f32(1, 2, 0);
                GX_End();
                 
                 */
                //printf("Running %p : %d\n", buf_anchor + gx_mesh->dl_offset, gx_mesh->dl_length);
                //sleep(5);
                GX_CallDispList(buf_anchor + gx_mesh->dl_offset, gx_mesh->dl_length);
                
                index_buf += sizeof(pmdl_gx_mesh);
                
            }
        
#       endif
        
        // ADVANCE!!
        collection_header += sizeof(pmdl_col_header);
        
    }
}

/* This routine will draw PAR1 PMDLs */
static void pmdl_draw_par1(pmdl_draw_context_t* ctx, const pspl_runtime_arc_file_t* pmdl_file) {
    
}

/* This routine will draw PAR2 PMDLs */
static void pmdl_draw_par2(pmdl_draw_context_t* ctx, const pspl_runtime_arc_file_t* pmdl_file) {
    
}

/* This is the main draw dispatch routine */
void pmdl_draw(pmdl_draw_context_t* ctx, const pspl_runtime_arc_file_t* pmdl_file) {
    pmdl_header* header = pmdl_file->file_data;

    // Master Frustum test
    if (!pmdl_aabb_frustum_test(ctx, header->master_aabb)) {
        //printf("Failed master\n");
        return;
    }
    //printf("Passed master\n");

    
    // Select draw routine based on sub-type
    if (header->sub_type_num == '0')
        pmdl_draw_par0(ctx, pmdl_file);
    else if (header->sub_type_num == '1')
        pmdl_draw_par1(ctx, pmdl_file);
    else if (header->sub_type_num == '2')
        pmdl_draw_par2(ctx, pmdl_file);
        
}


