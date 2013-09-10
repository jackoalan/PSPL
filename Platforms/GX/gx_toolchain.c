//
//  gx_toolchain.c
//  PSPL
//
//  Created by Jack Andersen on 5/22/13.
//
//

#include <PSPLExtension.h>
#include "GXStreamGen/gx_offline.h"
#include "gx_common.h"

static void copyright_hook() {
    
    pspl_toolchain_provide_copyright("GX PSPL Platform (Offline GX Stream Generator and Runtime Loader)",
                                     "Copyright (c) 2013 Jack Andersen <jackoalan@gmail.com>",
                                     "[See licence for \"PSPL\" above]");
    
    pspl_toolchain_provide_copyright("libogc (GX API)",
                                     "Copyright (C) 2004 - 2009\n"
                                     "    Michael Wiedenbauer (shagkur)\n"
                                     "    Dave Murphy (WinterMute)",
                                     
                                     "This software is provided 'as-is', without any express or implied\n"
                                     "warranty.  In no event will the authors be held liable for any\n"
                                     "damages arising from the use of this software.\n"
                                     "\n"
                                     "Permission is granted to anyone to use this software for any\n"
                                     "purpose, including commercial applications, and to alter it and\n"
                                     "redistribute it freely, subject to the following restrictions:\n"
                                     "\n"
                                     "1. The origin of this software must not be misrepresented; you\n"
                                     "   must not claim that you wrote the original software. If you use\n"
                                     "   this software in a product, an acknowledgment in the product\n"
                                     "   documentation would be appreciated but is not required.\n"
                                     "2. Altered source versions must be plainly marked as such, and\n"
                                     "   must not be misrepresented as being the original software.\n"
                                     "3. This notice may not be removed or altered from any source\n"
                                     "   distribution.\n"
                                     "");
    
}

#if __LITTLE_ENDIAN__
#define MAKE_BIG_U16(val) swap_uint16(val)
#elif __BIG_ENDIAN__
#define MAKE_BIG_U16(val) (val)
#endif

static void instruction_hook(const pspl_toolchain_context_t* driver_context,
                             const pspl_extension_t* sending_extension,
                             const char* operation,
                             const void* data) {
    int i,j;
    
    if (!strcmp(operation, "PSPL-IR")) {
        const pspl_ir_state_t* ir_state = data;
        
        // Allocate IR buffer
        size_t ir_size = sizeof(pspl_gx_ir_t) + (sizeof(pspl_gx_ir_tc_gen_t)*ir_state->vertex.tc_count) +
                         (sizeof(pspl_gx_ir_fragment_stage_t)*ir_state->fragment.stage_count);
        void* ir_buf = malloc(ir_size);
        
        pspl_gx_ir_t* target_ir_state = ir_buf;
        
        target_ir_state->total_uv_attr_count = MAKE_BIG_U16(ir_state->total_uv_attr_count);
        target_ir_state->total_texmap_count = MAKE_BIG_U16(ir_state->total_texmap_count);
        
        target_ir_state->vertex.tc_count = MAKE_BIG_U16(ir_state->vertex.tc_count);
        target_ir_state->vertex.using_texcoord_normal = MAKE_BIG_U16(0);
        target_ir_state->vertex.tc_off = MAKE_BIG_U16(sizeof(pspl_gx_ir_t));
        target_ir_state->vertex.bone_count = MAKE_BIG_U16(ir_state->vertex.bone_count);
        
        target_ir_state->depth.test = MAKE_BIG_U16(ir_state->depth.test);
        target_ir_state->depth.write = MAKE_BIG_U16(ir_state->depth.write);
        
        target_ir_state->fragment.def_stage_op = MAKE_BIG_U16(ir_state->fragment.def_stage_op);
        target_ir_state->fragment.def_stage_op_idx = MAKE_BIG_U16(ir_state->fragment.def_stage_op_idx);
        target_ir_state->fragment.stage_count = MAKE_BIG_U16(ir_state->fragment.stage_count);
        target_ir_state->fragment.stage_off = MAKE_BIG_U16(sizeof(pspl_gx_ir_t) + (sizeof(pspl_gx_ir_tc_gen_t) * ir_state->vertex.tc_count));
        
        target_ir_state->blend.blending = MAKE_BIG_U16(ir_state->blend.blending);
        target_ir_state->blend.source_factor = MAKE_BIG_U16(ir_state->blend.source_factor);
        target_ir_state->blend.dest_factor = MAKE_BIG_U16(ir_state->blend.dest_factor);
        
        pspl_gx_ir_tc_gen_t* target_tc_gen_arr = ir_buf + sizeof(pspl_gx_ir_t);
        for (i=0 ; i<ir_state->vertex.tc_count ; ++i) {
            pspl_gx_ir_tc_gen_t* target_tc_gen = &target_tc_gen_arr[i];
            target_tc_gen->tc_source = MAKE_BIG_U16(ir_state->vertex.tc_array[i].tc_source);
            if (ir_state->vertex.tc_array[i].tc_source == TEXCOORD_NORM)
                target_ir_state->vertex.using_texcoord_normal = MAKE_BIG_U16(1);
            target_tc_gen->uv_idx = MAKE_BIG_U16(ir_state->vertex.tc_array[i].uv_idx);
        }
        
        pspl_gx_ir_fragment_stage_t* target_stage_arr = ir_buf + sizeof(pspl_gx_ir_t) + (sizeof(pspl_gx_ir_tc_gen_t) * ir_state->vertex.tc_count);
        for (i=0 ; i<ir_state->fragment.stage_count ; ++i) {
            pspl_gx_ir_fragment_stage_t* target_stage = &target_stage_arr[i];
            const pspl_ir_fragment_stage_t* source_stage = &ir_state->fragment.stage_array[i];
            target_stage->stage_output = MAKE_BIG_U16(source_stage->stage_output);
            target_stage->stage_op = MAKE_BIG_U16(source_stage->stage_op);
            for (j=0 ; j<3 ; ++j) {
                target_stage->sources[j] = MAKE_BIG_U16(source_stage->sources[j]);
            }
            for (j=0 ; j<3 ; ++j) {
                target_stage->side_in_indices[j] = MAKE_BIG_U16(source_stage->side_in_indices[j]);
            }
            target_stage->using_texture = MAKE_BIG_U16(source_stage->using_texture);
            target_stage->stage_texmap.texcoord_idx = MAKE_BIG_U16(source_stage->stage_texmap.texcoord_idx);
            target_stage->stage_texmap.texmap_idx = MAKE_BIG_U16(source_stage->stage_texmap.texmap_idx);
            target_stage->using_indirect = MAKE_BIG_U16(source_stage->using_indirect);
            target_stage->stage_indtexmap.texcoord_idx = MAKE_BIG_U16(source_stage->stage_indtexmap.texcoord_idx);
            target_stage->stage_indtexmap.texmap_idx = MAKE_BIG_U16(source_stage->stage_indtexmap.texmap_idx);
            target_stage->stage_lightchan_idx = MAKE_BIG_U16(source_stage->stage_lightchan_idx);
            target_stage->using_colour = MAKE_BIG_U16(source_stage->using_colour);
            target_stage->stage_colour.a = source_stage->stage_colour.a * 0xff;
            target_stage->stage_colour.r = source_stage->stage_colour.r * 0xff;
            target_stage->stage_colour.g = source_stage->stage_colour.g * 0xff;
            target_stage->stage_colour.b = source_stage->stage_colour.b * 0xff;
        }
        
        
        pspl_embed_platform_integer_keyed_object(GX_SHADER_IR, NULL, ir_buf, ir_size);
        free(ir_buf);
        
    }
}

/* Toolchain platform definition */
pspl_toolchain_platform_t GX_toolplat = {
    .copyright_hook = copyright_hook,
    .instruction_hook = instruction_hook
};
