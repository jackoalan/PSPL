//
//  gx_runtime.c
//  PSPL
//
//  Created by Jack Andersen on 5/25/13.
//
//

#include <ogc/cache.h>
#include <ogc/gx.h>
#include <PSPLExtension.h>
#include "gx_common.h"

static void load_object(pspl_runtime_psplc_t* object) {
    
    pspl_data_object_t datao;
    pspl_runtime_get_embedded_data_object_from_integer(object, GX_SHADER_IR, &datao);

    object->native_shader.ir_stream_buf = datao.object_data;
    object->native_shader.ir_stream_len = (u32)datao.object_len;
    
    pspl_gx_ir_t* ir_state = datao.object_data;
    object->native_shader.texgen_count = ir_state->vertex.tc_count;
    object->native_shader.using_texcoord_normal = ir_state->vertex.using_texcoord_normal;
    
}

static void bind_object(pspl_runtime_psplc_t* object) {
    int i,j;
    
    void* ir_cur = object->native_shader.ir_stream_buf;
    pspl_gx_ir_t* ir_state = ir_cur;
    
    pspl_gx_ir_tc_gen_t* tc_array = ir_cur + ir_state->vertex.tc_off;
    pspl_gx_ir_fragment_stage_t* stage_array = ir_cur + ir_state->fragment.stage_off;
    
    // Vertex state
    for (j=0 ; j<ir_state->vertex.tc_count ; ++j) {
        if (j >= 8)
            pspl_error(-1, "GX TexCoord Generator Overflow",
                       "GX supports a maximum of 8 TexCoord Generators; this PSPL has %u generators",
                       ir_state->vertex.tc_count);
        if (tc_array[j].tc_source == PSPL_GX_TEXCOORD_UV)
            GX_SetTexCoordGen(GX_TEXCOORD0 + j, GX_TG_MTX3x4,
                              GX_TG_TEX0 + tc_array[j].uv_idx,
                              GX_TEXMTX0 + (j*3));
        else if (tc_array[j].tc_source == PSPL_GX_TEXCOORD_POS)
            GX_SetTexCoordGen(GX_TEXCOORD0 + j, GX_TG_MTX3x4,
                              GX_TG_POS,
                              GX_TEXMTX0 + (j*3));
        else if (tc_array[j].tc_source == PSPL_GX_TEXCOORD_NORM) {
            GX_SetTexCoordGen2(GX_TEXCOORD0 + j, GX_TG_MTX3x4,
                               GX_TG_NRM,
                               GX_TEXMTX9,
                               GX_TRUE,
                               GX_DTTMTX0 + (j*3));
        }
    }
    GX_SetNumTexGens(ir_state->vertex.tc_count);
    
    
    // Depth mode
    GX_SetZMode((ir_state->depth.test == PSPL_GX_ENABLED || ir_state->depth.test == PSPL_GX_PLATFORM), GX_LESS,
                (ir_state->depth.write == PSPL_GX_ENABLED || ir_state->depth.write == PSPL_GX_PLATFORM));
    
    
    // Fragment state
    unsigned tev_count = 0;
    unsigned colour_index = 0;
    for (i=0 ; i<ir_state->fragment.stage_count ; ++i) {
        if (tev_count >= 16)
            pspl_error(-1, "GX TEV Stage Overflow",
                       "GX supports a maximum of 16 TEV (multi-texturing) stages; this PSPL has %u stages",
                       ir_state->fragment.stage_count);
        const pspl_gx_ir_fragment_stage_t* stage = &stage_array[i];
        u8 tev_stage = GX_TEVSTAGE0 + tev_count;
        
        u8 tex_coord = GX_TEXCOORDNULL;
        u32 tex_map = GX_TEXMAP_NULL;
        if (stage->using_texture) {
            tex_coord = GX_TEXCOORD0 + stage->stage_texmap.texcoord_idx;
            tex_map = GX_TEXMAP0 + stage->stage_texmap.texmap_idx;
        }
        GX_SetTevOrder(tev_stage, tex_coord, tex_map, GX_COLORNULL);
        
        if (stage->using_colour) {
            if (colour_index >= GX_KCOLOR_MAX)
                pspl_error(-1, "GX TEV Constant Colour Overflow",
                           "GX supports a maximum of 4 constant colours per TEV stage");
            GX_SetTevKColor(GX_KCOLOR0 + colour_index, stage->stage_colour);
            GX_SetTevKColorSel(tev_stage, GX_TEV_KCSEL_K0 + colour_index);
            GX_SetTevKAlphaSel(tev_stage, GX_TEV_KASEL_K0_A + colour_index);
            ++colour_index;
        }
        
        u8 stagesourcesc[3];
        u8 stagesourcesa[3];
        for (j=0 ; j<3 ; ++j) {
            stagesourcesc[j] = GX_CC_ZERO;
            stagesourcesa[j] = GX_CA_ZERO;
            if (stage->sources[j] == PSPL_GX_IN_ZERO) {
                stagesourcesc[j] = GX_CC_ZERO;
            } else if (stage->sources[j] == PSPL_GX_IN_ONE) {
                stagesourcesc[j] = GX_CC_ONE;
            } else if (stage->sources[j] == PSPL_GX_IN_TEXTURE) {
                stagesourcesc[j] = GX_CC_TEXC;
                stagesourcesa[j] = GX_CA_TEXA;
            } else if (stage->sources[j] == PSPL_GX_IN_COLOUR) {
                stagesourcesc[j] = GX_CC_KONST;
                stagesourcesa[j] = GX_CA_KONST;
            } else if (stage->sources[j] == PSPL_GX_IN_MAIN) {
                stagesourcesc[j] = GX_CC_CPREV;
                stagesourcesa[j] = GX_CA_APREV;
            } else if (stage->sources[j] == PSPL_GX_IN_SIDECHAIN) {
                stagesourcesc[j] = GX_CC_C0 + stage->side_in_indices[j]*2;
                stagesourcesa[j] = GX_CA_A0 + stage->side_in_indices[j];
            }
        }
        
        if (stage->stage_op == PSPL_GX_OP_SET) {
            
            GX_SetTevColorOp(tev_stage, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
            GX_SetTevColorIn(tev_stage, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, stagesourcesc[0]);
            //GX_SetTevAlphaOp(tev_stage, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
            //GX_SetTevAlphaIn(tev_stage, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, stagesourcesa[0]);
            
        } else if (stage->stage_op == PSPL_GX_OP_MUL) {
            
            GX_SetTevColorOp(tev_stage, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
            GX_SetTevColorIn(tev_stage, GX_CC_ZERO, stagesourcesc[1], stagesourcesc[0], GX_CC_ZERO);
            //GX_SetTevAlphaOp(tev_stage, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
            //GX_SetTevAlphaIn(tev_stage, GX_CA_ZERO, stagesourcesa[1], stagesourcesa[0], GX_CA_ZERO);
            
        } else if (stage->stage_op == PSPL_GX_OP_ADD) {
            
            GX_SetTevColorOp(tev_stage, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
            GX_SetTevColorIn(tev_stage, stagesourcesc[1], GX_CC_ZERO, GX_CC_ZERO, stagesourcesc[0]);
            //GX_SetTevAlphaOp(tev_stage, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
            //GX_SetTevAlphaIn(tev_stage, stagesourcesa[1], GX_CA_ZERO, GX_CA_ZERO, stagesourcesa[0]);
            
        } else if (stage->stage_op == PSPL_GX_OP_SUB) {
            
            GX_SetTevColorOp(tev_stage, GX_TEV_SUB, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
            GX_SetTevColorIn(tev_stage, stagesourcesc[1], GX_CC_ZERO, GX_CC_ZERO, stagesourcesc[0]);
            //GX_SetTevAlphaOp(tev_stage, GX_TEV_SUB, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
            //GX_SetTevAlphaIn(tev_stage, stagesourcesa[1], GX_CA_ZERO, GX_CA_ZERO, stagesourcesa[0]);
            
        } else if (stage->stage_op == PSPL_GX_OP_BLEND) {
            
            GX_SetTevColorOp(tev_stage, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
            GX_SetTevColorIn(tev_stage, stagesourcesc[0], stagesourcesc[2], stagesourcesc[1], GX_CC_ZERO);
            //GX_SetTevAlphaOp(tev_stage, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
            //GX_SetTevAlphaIn(tev_stage, stagesourcesa[0], stagesourcesa[2], stagesourcesa[1], GX_CA_ZERO);
            
        }
        
        ++tev_count;
    }
    GX_SetNumTevStages(tev_count);
    
    
    // Blending mode
    if (ir_state->blend.blending == PSPL_GX_ENABLED) {
        
        u8 src_fac = GX_BL_SRCALPHA;
        unsigned data_source = ir_state->blend.source_factor & 0x3;
        unsigned inverse = ir_state->blend.source_factor & 0x4;
        if (data_source == PSPL_GX_SRC_COLOUR)
            src_fac = (inverse)?GX_BL_INVSRCCLR:GX_BL_SRCCLR;
        else if (data_source == PSPL_GX_DST_COLOUR)
            src_fac = (inverse)?GX_BL_INVDSTCLR:GX_BL_DSTCLR;
        else if (data_source == PSPL_GX_SRC_ALPHA)
            src_fac = (inverse)?GX_BL_INVSRCALPHA:GX_BL_SRCALPHA;
        else if (data_source == PSPL_GX_DST_ALPHA)
            src_fac = (inverse)?GX_BL_INVDSTALPHA:GX_BL_DSTALPHA;
        
        u8 dest_fac = GX_BL_INVSRCALPHA;
        data_source = ir_state->blend.dest_factor & 0x3;
        inverse = ir_state->blend.dest_factor & 0x4;
        if (data_source == PSPL_GX_SRC_COLOUR)
            dest_fac = (inverse)?GX_BL_INVSRCCLR:GX_BL_SRCCLR;
        else if (data_source == PSPL_GX_DST_COLOUR)
            dest_fac = (inverse)?GX_BL_INVDSTCLR:GX_BL_DSTCLR;
        else if (data_source == PSPL_GX_SRC_ALPHA)
            dest_fac = (inverse)?GX_BL_INVSRCALPHA:GX_BL_SRCALPHA;
        else if (data_source == PSPL_GX_DST_ALPHA)
            dest_fac = (inverse)?GX_BL_INVDSTALPHA:GX_BL_DSTALPHA;
        
        GX_SetBlendMode(GX_BM_BLEND, src_fac, dest_fac, 0);
        
    } else
        GX_SetBlendMode(GX_BM_NONE, GX_BL_ZERO, GX_BL_ZERO, 0);
    
}


pspl_runtime_platform_t GX_runplat = {
    .load_object_hook = load_object,
    .bind_object_hook = bind_object
};
