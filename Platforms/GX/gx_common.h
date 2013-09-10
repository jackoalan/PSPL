//
//  gx_common.h
//  PSPL
//
//  Created by Jack Andersen on 8/14/13.
//
//

#ifndef PSPL_gx_common_h
#define PSPL_gx_common_h

#include <stdint.h>

enum gx_object {
    GX_SHADER_IR     = 1
};


/* Feature enable state enum */
enum pspl_gx_feature {
    PSPL_GX_PLATFORM = 0,
    PSPL_GX_DISABLED = 1,
    PSPL_GX_ENABLED  = 2
};

/* Blend factor enum */
enum pspl_gx_blend_factor {
    PSPL_GX_SRC_COLOUR = 0,
    PSPL_GX_DST_COLOUR = 1,
    PSPL_GX_SRC_ALPHA  = 2,
    PSPL_GX_DST_ALPHA  = 3,
    PSPL_GX_ONE_MINUS  = 4
};

/* TC Generator Source */
enum pspl_gx_tc_source {
    PSPL_GX_TEXCOORD_UV = 0,
    PSPL_GX_TEXCOORD_POS = 1,
    PSPL_GX_TEXCOORD_NORM = 2
};

/* TEV Op */
enum pspl_gx_tev_op {
    PSPL_GX_OP_SET = 0,    // 1-source, always used in first stage
    PSPL_GX_OP_MUL = 1,    // 2-sources; (a*b)
    PSPL_GX_OP_ADD = 2,    // 2-sources; (a+b)
    PSPL_GX_OP_SUB = 3,    // 2-sources; (a-b)
    PSPL_GX_OP_BLEND = 4,  // 3-sources; ((1.0 - c)*a + c*b)
};

/* TEV operand source */
enum pspl_gx_tev_op_source {
    PSPL_GX_IN_ZERO = 0,     // Constant '0' value
    PSPL_GX_IN_ONE = 1,      // Constant '1' value
    PSPL_GX_IN_TEXTURE = 2,  // Sampled texel value
    PSPL_GX_IN_LIGHTING = 3, // Computed lighting channel value // NOT USED QUITE YET!!!
    PSPL_GX_IN_COLOUR = 4,   // Specified constant colour value (from parent stage structure)
    PSPL_GX_IN_MAIN = 5,     // Previous-stage-output main value
    PSPL_GX_IN_SIDECHAIN = 6 // Previous-stage-output sidechain value
};

/* Texture map config */
typedef struct {
    
    // Index of texture map (automatically mapped by toolchain)
    uint16_t texmap_idx;
    
    // Index of texcoord gen (binds fragment stage to vertex stage)
    uint16_t texcoord_idx;
    
} pspl_gx_ir_texture_t;

/* Multi-texturing stage */
typedef struct {
    
    // Stage output
    uint16_t stage_output;
    
    // Stage Operation
    uint16_t stage_op;
    
    // Stage Operation Sources [a,b,c]
    uint16_t sources[3];
    
    // Sidechain input indices (when one or more sources set to 'IN_SIDE')
    uint16_t side_in_indices[3];
    
    // Texture map index
    uint16_t using_texture;
    pspl_gx_ir_texture_t stage_texmap;
    
    // Indirect texture map index
    uint16_t using_indirect;
    pspl_gx_ir_texture_t stage_indtexmap;
    
    // Lighting channel index // NOT USED QUITE YET!!!
    uint16_t stage_lightchan_idx;
    
    // Constant colour (for stage source 'IN_COLOUR')
    uint16_t using_colour;
    GXColor stage_colour;
    
    
} pspl_gx_ir_fragment_stage_t;

typedef struct {
    uint16_t tc_source;
    uint16_t uv_idx;
} pspl_gx_ir_tc_gen_t;

/* IR translation state */
typedef struct {
    
    // Total count of UV vertex attributes
    uint16_t total_uv_attr_count;
    
    // Total count of texture maps
    uint16_t total_texmap_count;
    
    // Vertex state
    struct {
        
        // Texcoord gen array
        uint16_t tc_off;
        uint16_t tc_count, using_texcoord_normal;
        //pspl_gx_ir_tc_gen_t tc_array[MAX_TEX_COORDS];
        
        // Bone count
        uint16_t bone_count;
        
    } vertex;
    
    // Depth state
    struct {
        
        uint16_t test;
        uint16_t write;
        
    } depth;
    
    // Fragment state
    struct {
        
        // Are we enumerating stage operands?
        uint16_t def_stage_op;
        uint16_t def_stage_op_idx;
        
        // Stage Array
        uint16_t stage_count;
        uint16_t stage_off;
        //pspl_gx_ir_fragment_stage_t stage_array[MAX_FRAG_STAGES];
        
        
    } fragment;
    
    // Blend State
    struct {
        
        // Blending enabled?
        uint16_t blending;
        
        // Blend factors
        uint16_t source_factor;
        uint16_t dest_factor;
        
    } blend;
    
} pspl_gx_ir_t;

#endif
