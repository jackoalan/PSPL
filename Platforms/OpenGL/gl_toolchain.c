//
//  gl_toolchain.c
//  PSPL
//
//  Created by Jack Andersen on 5/25/13.
//
//

#include <stdio.h>
#include <PSPLExtension.h>
#include "gl_common.h"

static const char SHADER_HEAD[] =
"#ifdef GL_ES\n"
"#version 100\n"
"#define HIGHPREC highp\n"
"#define MEDPREC mediump\n"
"#define LOWPREC lowp\n"
"#else\n"
"#version 120\n"
"#define HIGHPREC\n"
"#define MEDPREC\n"
"#define LOWPREC\n"
"#endif\n\n";


static void copyright_hook() {
    
    pspl_toolchain_provide_copyright("OpenGL[ES] 2.0 Platform (GLSL Generator and Runtime Loader)",
                                     "Copyright (c) 2013 Jack Andersen <jackoalan@gmail.com>",
                                     "[See licence for \"PSPL\" above]");
    
}

static void generate_vertex(const pspl_toolchain_context_t* driver_context,
                            const pspl_ir_state_t* ir_state,
                            pspl_buffer_t* vert) {
    int j;
    
    // Start with preamble
    pspl_buffer_addstr(vert,
                       "/* PSPL AUTO-GENERATED VERTEX SHADER SOURCE\n"
                       " * generated for `");
    pspl_buffer_addstr(vert, driver_context->pspl_name);
    pspl_buffer_addstr(vert,
                       "` */\n"
                       "\n");
    pspl_buffer_addstr(vert, SHADER_HEAD);
    
    
    
    // Modelview transform uniform
    pspl_buffer_addstr(vert, "uniform mat4 modelview_mat;\n");
    pspl_buffer_addstr(vert, "uniform mat4 modelview_invtrans_mat;\n");
    
    // Projection transform uniform
    pspl_buffer_addstr(vert, "uniform mat4 projection_mat;\n");
    
    // Texcoord generator transform uniforms
    if (ir_state->vertex.tc_count) {
        char uniform[64];
        snprintf(uniform, 64, "uniform mat4 tc_generator_mats[%u];\n", ir_state->vertex.tc_count);
        pspl_buffer_addstr(vert, uniform);
    }
    
    pspl_buffer_addstr(vert, "\n");
    
    
    // Position attribute
    pspl_buffer_addstr(vert, "attribute vec4 pos;\n");
    
    // Normal attribute
    pspl_buffer_addstr(vert, "attribute vec4 norm;\n");
    
    // Texcoord attributes
    for (j=0 ; j<ir_state->total_uv_attr_count ; ++j) {
        char uv[64];
        snprintf(uv, 64, "attribute vec2 uv%u;\n", j);
        pspl_buffer_addstr(vert, uv);
    }
    
    pspl_buffer_addstr(vert, "\n");
    
    
    // Varying normal
    pspl_buffer_addstr(vert, "varying HIGHPREC vec4 normal;\n");
    
    // Varying texcoord linkages
    if (ir_state->vertex.tc_count) {
        char varying_str[64];
        snprintf(varying_str, 64, "varying HIGHPREC vec2 texCoords[%u];\n\n", ir_state->vertex.tc_count);
        pspl_buffer_addstr(vert, varying_str);
    }
    pspl_buffer_addstr(vert, "\n");
    
    
    // Main vertex code
    pspl_buffer_addstr(vert, "void main() {\n");
    
    // Position
    pspl_buffer_addstr(vert, "    gl_Position = projection_mat * modelview_mat * pos;\n");
    
    // Normal
    pspl_buffer_addstr(vert, "    normal = modelview_invtrans_mat * norm;\n");
    
    // Texcoords
    for (j=0 ; j<ir_state->vertex.tc_count ; ++j) {
        char assign[64];
        if (ir_state->vertex.tc_array[j].tc_source == TEXCOORD_UV)
            snprintf(assign, 64, "    texCoords[%u] = tc_generator_mats[%u] * uv%u;\n", j, j,
                     ir_state->vertex.tc_array[j].uv_idx);
        else if (ir_state->vertex.tc_array[j].tc_source == TEXCOORD_POS)
            snprintf(assign, 64, "    texCoords[%u] = tc_generator_mats[%u] * pos;\n", j, j);
        else if (ir_state->vertex.tc_array[j].tc_source == TEXCOORD_NORM)
            snprintf(assign, 64, "    texCoords[%u] = tc_generator_mats[%u] * normal;\n", j, j);
        pspl_buffer_addstr(vert, assign);
    }
    
    pspl_buffer_addstr(vert, "}\n\n");
    
}

static void generate_fragment(const pspl_toolchain_context_t* driver_context,
                              const pspl_ir_state_t* ir_state,
                              pspl_buffer_t* frag) {
    int j;
    
    // Start with preamble
    pspl_buffer_addstr(frag,
                       "/* PSPL AUTO-GENERATED FRAGMENT SHADER SOURCE\n"
                       " * generated for `");
    pspl_buffer_addstr(frag, driver_context->pspl_name);
    pspl_buffer_addstr(frag,
                       "` */\n"
                       "\n");
    pspl_buffer_addstr(frag, SHADER_HEAD);
    
    
    
    // Varying normal
    pspl_buffer_addstr(frag, "varying HIGHPREC vec4 normal;\n");
    
    // Varying texcoord linkages
    char varying_str[64];
    snprintf(varying_str, 64, "varying HIGHPREC vec2 texCoords[%u];\n\n", ir_state->vertex.tc_count);
    pspl_buffer_addstr(frag, varying_str);
    
    
    
    // Texture map uniforms
    char uniform_str[64];
    snprintf(uniform_str, 64, "uniform LOWPREC sampler2D texs[%u];\n\n", ir_state->total_texmap_count);
    pspl_buffer_addstr(frag, uniform_str);
    
    
    
    // Main fragment code
    pspl_buffer_addstr(frag, "void main() {\n");
    
    // Stage output variables
    pspl_buffer_addstr(frag, "    LOWPREC vec4 stage_main;\n");
    for (j=0 ; j<ir_state->fragment.stage_count ; ++j) {
        if (ir_state->fragment.stage_array[j].stage_output == OUT_SIDECHAIN) {
            char sidedef[64];
            snprintf(sidedef, 64, "    LOWPREC vec4 stage_side_%d;\n", j);
            pspl_buffer_addstr(frag, sidedef);
        }
    }
    pspl_buffer_addstr(frag, "\n");
        
    
    // Iterate each fragment stage
    for (j=0 ; j<ir_state->fragment.stage_count ; ++j) {
        const pspl_ir_fragment_stage_t* stage = &ir_state->fragment.stage_array[j];
        
        char comment[64];
        snprintf(comment, 64, "    // Stage %u\n", j);
        pspl_buffer_addstr(frag, comment);
        
        char stagedecl[64];
        if (stage->stage_output == OUT_MAIN)
            snprintf(stagedecl, 64, "stage_main");
        else if (stage->stage_output == OUT_SIDECHAIN)
            snprintf(stagedecl, 64, "stage_side_%d", j);
        
        char stagesources[3][64];
        int s;
        for (s=0 ; s<3 ; ++s) {
            if (stage->sources[s] == IN_ZERO)
                snprintf(stagesources[s], 64, "vec4(0.0,0.0,0.0,1.0)");
            else if (stage->sources[s] == IN_ONE)
                snprintf(stagesources[s], 64, "vec4(1.0,1.0,1.0,1.0)");
            else if (stage->sources[s] == IN_TEXTURE)
                snprintf(stagesources[s], 64, "texture2D(texs[%u], texCoords[%u])",
                         stage->stage_texmap.texmap_idx, stage->stage_texmap.resolved_name_idx);
            else if (stage->sources[s] == IN_LIGHTING)
                snprintf(stagesources[s], 64, "vec4(1.0,1.0,1.0,1.0)");
            else if (stage->sources[s] == IN_COLOUR)
                snprintf(stagesources[s], 64, "vec4(%f,%f,%f,%f)", stage->stage_colour.r,
                         stage->stage_colour.g, stage->stage_colour.b, stage->stage_colour.a);
            else if (stage->sources[s] == IN_MAIN)
                snprintf(stagesources[s], 64, "stage_main");
            else if (stage->sources[s] == IN_SIDECHAIN)
                snprintf(stagesources[s], 64, "stage_side_%d", stage->side_in_indices[s]);
        }
        
        char stagedef[256];
        if (stage->stage_op == OP_SET)
            snprintf(stagedef, 256, "    %s = %s;\n", stagedecl, stagesources[0]);
        else if (stage->stage_op == OP_MUL)
            snprintf(stagedef, 256, "    %s = %s * %s;\n", stagedecl,
                     stagesources[0], stagesources[1]);
        else if (stage->stage_op == OP_ADD)
            snprintf(stagedef, 256, "    %s = %s + %s;\n", stagedecl,
                     stagesources[0], stagesources[1]);
        else if (stage->stage_op == OP_SUB)
            snprintf(stagedef, 256, "    %s = %s - %s;\n", stagedecl,
                     stagesources[0], stagesources[1]);
        else if (stage->stage_op == OP_BLEND)
            snprintf(stagedef, 256, "    %s = mix(%s,%s,%s);\n", stagedecl,
                     stagesources[0], stagesources[2], stagesources[1]);
        
        pspl_buffer_addstr(frag, stagedef);
        
        pspl_buffer_addstr(frag, "\n");
    }
    
    // Colour
    pspl_buffer_addstr(frag, "    gl_FragColor = stage_main;\n");
    
    
    pspl_buffer_addstr(frag, "}\n\n");
        
}

static void instruction_hook(const pspl_toolchain_context_t* driver_context,
                             const pspl_extension_t* sending_extension,
                             const char* operation,
                             const void* data) {
    if (!strcmp(operation, "PSPL-IR")) {
        const pspl_ir_state_t* ir_state = data;
        
        // Config structure
        gl_config_t config = {
            .uv_attr_count = ir_state->total_uv_attr_count,
            .texmap_count = ir_state->total_texmap_count,
            .depth_write = ir_state->depth.write,
            .depth_test = ir_state->depth.test,
            .blending = ir_state->blend.blending,
            .source_factor = ir_state->blend.source_factor,
            .dest_factor = ir_state->blend.dest_factor
        };
        pspl_embed_platform_integer_keyed_object(GL_CONFIG_STRUCT, &config, &config, sizeof(gl_config_t));
        
        // Vertex shader
        pspl_buffer_t vert_buf;
        pspl_buffer_init(&vert_buf, 2048);
        generate_vertex(driver_context, ir_state, &vert_buf);
        pspl_embed_platform_integer_keyed_object(GL_VERTEX_SOURCE, vert_buf.buf, vert_buf.buf,
                                                 vert_buf.buf_cur - vert_buf.buf + 1);
        pspl_buffer_free(&vert_buf);
        
        // Fragment shader
        pspl_buffer_t frag_buf;
        pspl_buffer_init(&frag_buf, 2048);
        generate_fragment(driver_context, ir_state, &frag_buf);
        pspl_embed_platform_integer_keyed_object(GL_FRAGMENT_SOURCE, frag_buf.buf, frag_buf.buf,
                                                 frag_buf.buf_cur - frag_buf.buf + 1);
        pspl_buffer_free(&frag_buf);
    }
}


/* Toolchain platform definition */
pspl_toolchain_platform_t GL2_toolplat = {
    .copyright_hook = copyright_hook,
    .instruction_hook = instruction_hook
};
