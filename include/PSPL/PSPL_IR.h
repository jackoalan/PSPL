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

/* Maximum UV chains */
#define MAX_CALC_UVS 16

#pragma mark CalcChain Interface

#if PSPL_TOOLCHAIN
#include <PSPL/PSPLBuffer.h>

enum CHAIN_LINK_USE {
    LINK_USE_STATIC,
    LINK_USE_DYNAMIC
};

/* Calculation chain-link type */
typedef struct {
    
    // How the link value is obtained
    enum CHAIN_LINK_USE use;
    
    // Link matrix value (Static)
    pspl_matrix34_t matrix_value;
    
    // Link name-binding (Dynamic)
    char link_dynamic_bind_name[IR_NAME_LEN];
    
} pspl_calc_link_t;

/* Calculation chain type */
typedef struct {
    pspl_malloc_context_t mem_ctx;
} pspl_calc_chain_t;

/* Routines provided by `PSPL-IR` toolchain extension */
void pspl_calc_chain_init(pspl_calc_chain_t* chain);
void pspl_calc_chain_destroy(pspl_calc_chain_t* chain);
void pspl_calc_chain_add_static_transform(pspl_calc_chain_t* chain,
                                          const pspl_matrix34_t matrix);
void pspl_calc_chain_add_dynamic_transform(pspl_calc_chain_t* chain,
                                           const char* bind_name);
void pspl_calc_chain_add_static_scale(pspl_calc_chain_t* chain,
                                      const pspl_vector3_t scale_vector);
void pspl_calc_chain_add_dynamic_scale(pspl_calc_chain_t* chain,
                                       const char* bind_name);
void pspl_calc_chain_add_static_rotation(pspl_calc_chain_t* chain,
                                         const pspl_rotation_t* rotation);
void pspl_calc_chain_add_dynamic_rotation(pspl_calc_chain_t* chain,
                                          const char* bind_name);
void pspl_calc_chain_add_static_translation(pspl_calc_chain_t* chain,
                                            const pspl_vector3_t trans_vector);
void pspl_calc_chain_add_dynamic_translation(pspl_calc_chain_t* chain,
                                             const char* bind_name);
void pspl_calc_chain_write_position(pspl_calc_chain_t* chain);
void pspl_calc_chain_write_normal(pspl_calc_chain_t* chain);
void pspl_calc_chain_write_uv(pspl_calc_chain_t* chain, unsigned uv_idx);


#elif PSPL_RUNTIME
#include <PSPLExtension.h>

/* Routines provided by `PSPL-IR` runtime extension */
void pspl_calc_chain_bind(const pspl_runtime_psplc_t* object);
void pspl_calc_chain_set_dynamic_transform(const char* bind_name,
                                           const pspl_matrix34_t matrix);
void pspl_calc_chain_set_dynamic_scale(const char* bind_name,
                                       const pspl_vector3_t scale_vector);
void pspl_calc_chain_set_dynamic_rotation(const char* bind_name,
                                          const pspl_rotation_t* rotation);
void pspl_calc_chain_set_dynamic_translation(const char* bind_name,
                                             const pspl_vector3_t trans_vector);
void pspl_calc_chain_flush();

#endif

#pragma mark IR Interface

#ifdef PSPL_TOOLCHAIN

#define MAX_TEX_COORDS 10
#define MAX_FRAG_STAGES 16

/* Texture map config */
typedef struct {
    
    // Index of texture map (automatically mapped by toolchain)
    unsigned texmap_idx;
    
    // Index of texcoord gen (binds fragment stage to vertex stage)
    unsigned texcoord_idx;
    
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
    enum stage_op {
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
        IN_LIGHTING, // Computed lighting channel value // NOT USED QUITE YET!!!
        IN_COLOUR,   // Specified constant colour value (from parent stage structure)
        IN_MAIN,     // Previous-stage-output main value
        IN_SIDECHAIN // Previous-stage-output sidechain value
    } sources[3];
    
    // Sidechain input indices (when one or more sources set to 'IN_SIDE')
    unsigned side_in_indices[3];
    
    // Texture map index
    uint8_t using_texture;
    pspl_ir_texture_t stage_texmap;
    
    // Indirect texture map index
    uint8_t using_indirect;
    pspl_ir_texture_t stage_indtexmap;
    
    // Lighting channel index // NOT USED QUITE YET!!!
    unsigned stage_lightchan_idx;
    
    // Constant colour (for stage source 'IN_COLOUR')
    uint8_t using_colour;
    pspl_colour_t stage_colour;
    
    
} pspl_ir_fragment_stage_t;

enum shader_def_type {
    DEF_NONE = 0,
    
    DEF_GLSL_VERT_PRE,
    DEF_GLSL_VERT_POST,
    DEF_GLSL_FRAG_PRE,
    DEF_GLSL_FRAG_POST,
    
    DEF_HLSL_VERT_PRE,
    DEF_HLSL_VERT_POST,
    DEF_HLSL_FRAG_PRE,
    DEF_HLSL_FRAG_POST
    
};

/* IR translation state */
typedef struct {
    
    // Total count of UV vertex attributes
    unsigned total_uv_attr_count;
    
    // Total count of texture maps
    unsigned total_texmap_count;
    
    // Vertex state
    struct {
        
        // Are we populating a matrix?
        int in_matrix_def;
        unsigned matrix_row_count;
        pspl_matrix34_t matrix;
        
        // Position transform chain
        //pspl_calc_chain_t pos_chain;
        
        // Texcoord gens
        unsigned tc_count;
        struct {
            unsigned name_idx;
            enum {
                TEXCOORD_UV,
                TEXCOORD_POS,
                TEXCOORD_NORM
            } tc_source;
            unsigned uv_idx;
            //pspl_calc_chain_t tc_chain;
        } tc_array[MAX_TEX_COORDS];
        
        // Bone count
        unsigned bone_count;
        
        
        // In shader definition?
        int shader_def;
        
        // GLSL inclusion
        pspl_buffer_t glsl_pre;
        pspl_buffer_t glsl_post;
        
        // HLSL inclusion
        pspl_buffer_t hlsl_pre;
        pspl_buffer_t hlsl_post;
        
    } vertex;
    
    // Depth state
    struct {
        
        enum pspl_feature test;
        enum pspl_feature write;
        
    } depth;
    
    // Fragment state
    struct {
        
        // Are we enumerating stage operands?
        enum stage_op def_stage_op;
        unsigned def_stage_op_idx;
        
        // Stage Array
        unsigned stage_count;
        pspl_ir_fragment_stage_t stage_array[MAX_FRAG_STAGES];
        
        
        // In shader definition?
        int shader_def;
        
        // GLSL inclusion
        pspl_buffer_t glsl_pre;
        pspl_buffer_t glsl_post;
        
        // HLSL inclusion
        pspl_buffer_t hlsl_pre;
        pspl_buffer_t hlsl_post;
        
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
