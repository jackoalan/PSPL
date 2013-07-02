//
//  gx_toolchain.c
//  PSPL
//
//  Created by Jack Andersen on 5/22/13.
//
//

#include <PSPLExtension.h>
#include "GXStreamGen/gx_offline.h"

static void copyright_hook() {
    
    pspl_toolchain_provide_copyright("GX Platform (Offline GX Stream Generator and Runtime Loader)",
                                     "Copyright (c) 2013 Jack Andersen <jackoalan@gmail.com>",
                                     "[See licence for \"PSPL\" above]");
    
}

static void instruction_hook(const pspl_toolchain_context_t* driver_context,
                             const pspl_extension_t* sending_extension,
                             const char* operation,
                             const void* data) {
    int i,j;
    
    if (!strcmp(operation, "PSPL-IR")) {
        const pspl_ir_state_t* ir_state = data;
        
        
        // Begin GX transaction
        pspl_gx_offline_begin_transaction();
        
        
        // Vertex state
        for (j=0 ; j<ir_state->vertex.tc_count ; ++j) {
            if (j >= 8)
                pspl_error(-1, "GX TexCoord Generator Overflow",
                           "GX supports a maximum of 8 TexCoord Generators; this PSPL has %u generators",
                           ir_state->vertex.tc_count);
            if (ir_state->vertex.tc_array[j].tc_source == TEXCOORD_UV)
                GX_SetTexCoordGen(GX_TEXCOORD0 + j, GX_TG_MTX3x4,
                                  GX_TG_TEX0 + ir_state->vertex.tc_array[j].uv_idx,
                                  GX_TEXMTX0 + ir_state->vertex.tc_array[j].uv_idx);
            else if (ir_state->vertex.tc_array[j].tc_source == TEXCOORD_POS)
                GX_SetTexCoordGen(GX_TEXCOORD0 + j, GX_TG_MTX3x4,
                                  GX_TG_POS,
                                  GX_TEXMTX0 + ir_state->vertex.tc_array[j].uv_idx);
            else if (ir_state->vertex.tc_array[j].tc_source == TEXCOORD_NORM)
                GX_SetTexCoordGen(GX_TEXCOORD0 + j, GX_TG_MTX3x4,
                                  GX_TG_NRM,
                                  GX_TEXMTX0 + ir_state->vertex.tc_array[j].uv_idx);
        }
        
        
        // Depth mode
        GX_SetZMode((ir_state->depth.test == ENABLED || ir_state->depth.test == PLATFORM), GX_LESS,
                    (ir_state->depth.write == ENABLED || ir_state->depth.write == PLATFORM));
        
        
        // Fragment state
        unsigned tev_count = 0;
        unsigned colour_index = 0;
        for (i=0 ; i<ir_state->fragment.stage_count ; ++i) {
            if (tev_count >= 16)
                pspl_error(-1, "GX TEV Stage Overflow",
                           "GX supports a maximum of 16 TEV (multi-texturing) stages; this PSPL has %u stages",
                           ir_state->fragment.stage_count);
            const pspl_ir_fragment_stage_t* stage = &ir_state->fragment.stage_array[i];
            u8 tev_stage = GX_TEVSTAGE0 + tev_count;
            
            u8 tex_coord = GX_TEXCOORDNULL;
            u32 tex_map = GX_TEXMAP_NULL;
            if (stage->using_texture) {
                tex_coord = GX_TEXCOORD0 + stage->stage_texmap.resolved_name_idx;
                tex_map = GX_TEXMAP0 + stage->stage_texmap.texmap_idx;
            }
            GX_SetTevOrder(tev_stage, tex_coord, tex_map, GX_COLORNULL);
            
            if (stage->using_colour) {
                if (colour_index >= GX_KCOLOR_MAX)
                    pspl_error(-1, "GX TEV Constant Colour Overflow",
                               "GX supports a maximum of 4 constant colours per TEV stage");
                GXColor color = {
                    .r = stage->stage_colour.r * 0xff,
                    .g = stage->stage_colour.g * 0xff,
                    .b = stage->stage_colour.b * 0xff,
                    .a = stage->stage_colour.a * 0xff
                };
                GX_SetTevKColor(GX_KCOLOR0 + colour_index, color);
                GX_SetTevKColorSel(tev_stage, GX_TEV_KCSEL_K0 + colour_index);
                GX_SetTevKAlphaSel(tev_stage, GX_TEV_KASEL_K0_A + colour_index);
                ++colour_index;
            }
            
            u8 stagesourcesc[3];
            u8 stagesourcesa[3];
            for (j=0 ; j<3 ; ++j) {
                stagesourcesc[j] = GX_CC_ZERO;
                stagesourcesa[j] = GX_CA_ZERO;
                if (stage->sources[j] == IN_ZERO) {
                    stagesourcesc[j] = GX_CC_ZERO;
                } else if (stage->sources[j] == IN_ONE) {
                    stagesourcesc[j] = GX_CC_ONE;
                } else if (stage->sources[j] == IN_TEXTURE) {
                    stagesourcesc[j] = GX_CC_TEXC;
                    stagesourcesa[j] = GX_CA_TEXA;
                } else if (stage->sources[j] == IN_COLOUR) {
                    stagesourcesc[j] = GX_CC_KONST;
                    stagesourcesa[j] = GX_CA_KONST;
                } else if (stage->sources[j] == IN_MAIN) {
                    stagesourcesc[j] = GX_CC_CPREV;
                    stagesourcesa[j] = GX_CA_APREV;
                } else if (stage->sources[j] == IN_SIDECHAIN) {
                    stagesourcesc[j] = GX_CC_C0 + stage->side_in_indices[j];
                    stagesourcesa[j] = GX_CA_A0 + stage->side_in_indices[j];
                }
            }
            
            if (stage->stage_op == OP_SET) {
                
                GX_SetTevColorOp(tev_stage, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
                GX_SetTevColorIn(tev_stage, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, stagesourcesc[0]);
                GX_SetTevAlphaOp(tev_stage, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
                GX_SetTevAlphaIn(tev_stage, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, stagesourcesa[0]);
                
            } else if (stage->stage_op == OP_MUL) {
                
                GX_SetTevColorOp(tev_stage, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
                GX_SetTevColorIn(tev_stage, GX_CC_ZERO, stagesourcesc[1], stagesourcesc[0], GX_CC_ZERO);
                GX_SetTevAlphaOp(tev_stage, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
                GX_SetTevAlphaIn(tev_stage, GX_CA_ZERO, stagesourcesa[1], stagesourcesa[0], GX_CA_ZERO);
                
            } else if (stage->stage_op == OP_ADD) {
                
                GX_SetTevColorOp(tev_stage, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
                GX_SetTevColorIn(tev_stage, stagesourcesc[1], GX_CC_ZERO, GX_CC_ZERO, stagesourcesc[0]);
                GX_SetTevAlphaOp(tev_stage, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
                GX_SetTevAlphaIn(tev_stage, stagesourcesa[1], GX_CA_ZERO, GX_CA_ZERO, stagesourcesa[0]);
                
            } else if (stage->stage_op == OP_SUB) {
                
                GX_SetTevColorOp(tev_stage, GX_TEV_SUB, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
                GX_SetTevColorIn(tev_stage, stagesourcesc[1], GX_CC_ZERO, GX_CC_ZERO, stagesourcesc[0]);
                GX_SetTevAlphaOp(tev_stage, GX_TEV_SUB, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
                GX_SetTevAlphaIn(tev_stage, stagesourcesa[1], GX_CA_ZERO, GX_CA_ZERO, stagesourcesa[0]);
                
            } else if (stage->stage_op == OP_BLEND) {
                
                GX_SetTevColorOp(tev_stage, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
                GX_SetTevColorIn(tev_stage, stagesourcesc[0], stagesourcesc[2], stagesourcesc[1], GX_CC_ZERO);
                GX_SetTevAlphaOp(tev_stage, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
                GX_SetTevAlphaIn(tev_stage, stagesourcesa[0], stagesourcesa[2], stagesourcesa[1], GX_CA_ZERO);
                
            }
            
            ++tev_count;
        }
        GX_SetNumTevStages(tev_count);
        
        
        // Blending mode
        if (ir_state->blend.blending == ENABLED) {
            
            u8 src_fac = GX_BL_SRCALPHA;
            unsigned data_source = ir_state->blend.source_factor & 0x3;
            unsigned inverse = ir_state->blend.source_factor & 0x4;
            if (data_source == SRC_COLOUR)
                src_fac = (inverse)?GX_BL_INVSRCCLR:GX_BL_SRCCLR;
            else if (data_source == DST_COLOUR)
                src_fac = (inverse)?GX_BL_INVDSTCLR:GX_BL_DSTCLR;
            else if (data_source == SRC_ALPHA)
                src_fac = (inverse)?GX_BL_INVSRCALPHA:GX_BL_SRCALPHA;
            else if (data_source == DST_ALPHA)
                src_fac = (inverse)?GX_BL_INVDSTALPHA:GX_BL_DSTALPHA;
            
            u8 dest_fac = GX_BL_INVSRCALPHA;
            data_source = ir_state->blend.dest_factor & 0x3;
            inverse = ir_state->blend.dest_factor & 0x4;
            if (data_source == SRC_COLOUR)
                dest_fac = (inverse)?GX_BL_INVSRCCLR:GX_BL_SRCCLR;
            else if (data_source == DST_COLOUR)
                dest_fac = (inverse)?GX_BL_INVDSTCLR:GX_BL_DSTCLR;
            else if (data_source == SRC_ALPHA)
                dest_fac = (inverse)?GX_BL_INVSRCALPHA:GX_BL_SRCALPHA;
            else if (data_source == DST_ALPHA)
                dest_fac = (inverse)?GX_BL_INVDSTALPHA:GX_BL_DSTALPHA;
            
            GX_SetBlendMode(GX_BM_BLEND, src_fac, dest_fac, 0);
            
        } else
            GX_SetBlendMode(GX_BM_NONE, GX_BL_ZERO, GX_BL_ZERO, 0);
        
        
        // End GX Transaction
        GX_Flush();
        void* gx_buf = NULL;
        size_t gx_size = pspl_gx_offline_end_transaction(&gx_buf);
        pspl_embed_platform_integer_keyed_object(0, NULL, gx_buf, gx_size);
        free(gx_buf);
        
    }
}

/* Toolchain platform definition */
pspl_toolchain_platform_t GX_toolplat = {
    .copyright_hook = copyright_hook,
    .instruction_hook = instruction_hook
};
