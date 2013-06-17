//
//  PSPL_IR.h
//  PSPL
//
//  Created by Jack Andersen on 6/16/13.
//
//

#ifndef PSPL_PSPL_IR_h
#define PSPL_PSPL_IR_h

#include <PSPL/PSPLCommon.h>

#define IR_NAME_LEN 256

#pragma mark CalcChain Interface

#if PSPL_TOOLCHAIN

enum CHAIN_LINK_TYPE {
    LINK_TYPE_IDENTITY,
    LINK_TYPE_MATRIX34,
    LINK_TYPE_VECTOR4,
    LINK_TYPE_VECTOR3,
};

enum CHAIN_LINK_USE {
    LINK_USE_NONE,
    LINK_USE_STATIC,
    LINK_USE_DYNAMIC,
    LINK_USE_GPU
};

union CHAIN_LINK_VALUE {
    
    // Static values
    pspl_matrix34_t matrix;
    pspl_vector3_t vector3;
    
    // Dynamic name-binding
    char link_dynamic_bind_name[IR_NAME_LEN];
    
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
    enum CHAIN_LINK_USE perspective_use;
    pspl_perspective_t perspective;
    pspl_matrix34_t perspective_matrix;
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

#pragma mark IR Interface

#ifdef PSPL_TOOLCHAIN

#define MAX_TEX_COORDS 10
#define MAX_FRAG_STAGES 16

/* Feature enable state enum */
enum pspl_feature {
    PLATFORM = 0,
    DISABLED = 1,
    ENABLED  = 2
};

/* Blend factor enum */
enum pspl_blend_factor {
    SRC_COLOUR = 0,
    DST_COLOUR = 1,
    SRC_ALPHA  = 2,
    DST_ALPHA  = 3,
    ONE_MINUS  = 4
};

/* Texture map config */
typedef struct {
    
    // Index of texture map (automatically mapped by toolchain)
    unsigned texmap_idx;
    
    // Name of texcoord gen (binds fragment stage to vertex stage)
    char name[IR_NAME_LEN];
    unsigned resolved_name_idx;
    
} pspl_ir_texture_t;

/* Multi-texturing stage */
typedef struct {
    
    // Stage output
    enum {
        OUT_MAIN,
        OUT_SIDECHAIN
    } stage_output;
    
    // Sidechain output name (when output 'OUT_SIDECHAIN')
    char side_out_name[IR_NAME_LEN];
    
    // Stage Operation
    enum {
        OP_SET,    // 1-source, always used in first stage
        OP_MUL,    // 2-sources; (a*b)
        OP_ADD,    // 2-sources; (a+b)
        OP_SUB,    // 2-sources; (a-b)
        OP_BLEND,  // 3-sources; ((1.0 - c)*a + c*b)
    } stage_op;
    
    // Stage Operation Sources [a,b,c]
    enum {
        IN_ZERO,     // Constant '0' value
        IN_ONE,      // Constant '1' value
        IN_TEXTURE,  // Sampled texel value
        IN_LIGHTING, // Computed lighting channel value
        IN_COLOUR,   // Specified constant colour value (from parent stage structure)
        IN_MAIN,     // Previous-stage-output main value
        IN_SIDE      // Previous-stage-output sidechain value
    } sources[3];
    
    // Sidechain input names (when one or more sources set to 'IN_SIDE')
    char side_in_names[3][IR_NAME_LEN];
    
    // Texture map index
    uint8_t using_texture;
    pspl_ir_texture_t stage_texmap;
    
    // Indirect texture map index
    uint8_t using_indirect;
    pspl_ir_texture_t stage_indtexmap;
    
    // Lighting channel index
    unsigned stage_lightchan_idx;
    
    // Constant colour (for stage source 'IN_COLOUR')
    pspl_colour_t stage_colour;
    
    
} pspl_ir_fragment_stage_t;

/* IR translation state */
typedef struct {
    
    // Total count of UV vertex attributes
    unsigned total_uv_attr_count;
    
    // Vertex state
    struct {
        
        // Are we populating a matrix?
        int in_matrix_def;
        pspl_matrix34_t matrix;
        
        // Position transform chain
        pspl_calc_chain_t pos_chain;
        
        // Normal transform
        // (if the normal is referenced in fragment stage,
        // this is set, resulting in automatic inverse-transpose of
        // position matrix chain)
        uint8_t generate_normal;
        
        // Texcoord gens
        unsigned tc_count;
        struct {
            char name[IR_NAME_LEN];
            unsigned resolved_name_idx;
            enum {
                TEXCOORD_UV,
                TEXCOORD_POS,
                TEXCOORD_NORM
            } tc_source;
            unsigned uv_idx;
            pspl_calc_chain_t tc_chain;
        } tc_array[MAX_TEX_COORDS];
        
    } vertex;
    
    // Depth state
    struct {
        
        enum pspl_feature test;
        enum pspl_feature write;
        
    } depth;
    
    // Fragment state
    struct {
        
        // Stage Array
        unsigned stage_count;
        pspl_ir_fragment_stage_t stage_array[MAX_FRAG_STAGES];
        
    } fragment;
    
    // Blend State
    struct {
        
        // Blending enabled?
        enum pspl_feature blending;
        
        // Blend factors
        enum pspl_blend_factor source_factor;
        enum pspl_blend_factor dest_factor;
        
    } blend;
    
} pspl_ir_state_t;

#endif
#endif
