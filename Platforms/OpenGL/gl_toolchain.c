//
//  gl_toolchain.c
//  PSPL
//
//  Created by Jack Andersen on 5/25/13.
//
//

#include <stdio.h>
#include <PSPLExtension.h>

static const char SHADER_HEAD[] =
"#ifdef GLES\n"
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

static int init_hook(const pspl_toolchain_context_t* driver_context) {
    return 0;
}

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
    if (ir_state->vertex.generate_normal)
        pspl_buffer_addstr(vert, "uniform mat4 modelview_invtrans_mat;\n");
    
    // Projection transform uniform
    pspl_buffer_addstr(vert, "uniform mat4 projection_mat;\n");
    
    // Texcoord generator transform uniforms
    {
        char uniform[64];
        snprintf(uniform, 64, "uniform mat4 tc_generator_mats[%u]", ir_state->vertex.tc_count);
        pspl_buffer_addstr(vert, uniform);
    }
    
    pspl_buffer_addstr(vert, "\n");
    
    
    // Position attribute
    pspl_buffer_addstr(vert, "attribute vec4 pos;\n");
    
    // Normal attribute
    if (ir_state->vertex.generate_normal)
        pspl_buffer_addstr(vert, "attribute vec4 norm;\n");
    
    // Texcoord attributes
    for (j=0 ; j<ir_state->total_uv_attr_count ; ++j) {
        char uv[64];
        snprintf(uv, 64, "attribute vec2 uv%u;\n", j);
        pspl_buffer_addstr(vert, uv);
    }
    
    pspl_buffer_addstr(vert, "\n");
    
    
    // Varying normal
    if (ir_state->vertex.generate_normal)
        pspl_buffer_addstr(vert, "varying HIGHPREC vec4 normal;\n");
    
    // Varying texcoord linkages
    char varying_str[64];
    snprintf(varying_str, 64, "varying HIGHPREC vec2 texCoords[%u];\n\n", ir_state->vertex.tc_count);
    pspl_buffer_addstr(vert, varying_str);
    
    
    
    // Main vertex code
    pspl_buffer_addstr(vert, "void main() {\n");
    
    // Position
    pspl_buffer_addstr(vert, "    gl_Position = projection_mat * modelview_mat * pos;\n");
    
    // Normal
    if (ir_state->vertex.generate_normal)
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
    if (ir_state->vertex.generate_normal)
        pspl_buffer_addstr(frag, "varying HIGHPREC vec4 normal;\n");
    
    // Varying texcoord linkages
    char varying_str[64];
    snprintf(varying_str, 64, "varying HIGHPREC vec2 texCoords[%u];\n\n", ir_state->vertex.tc_count);
    pspl_buffer_addstr(frag, varying_str);
    
    
    
    // Main fragment code
    pspl_buffer_addstr(frag, "void main() {\n");
    
    // Colour
    pspl_buffer_addstr(frag, "    gl_FragColor = rgba(1.0,1.0,1.0,1.0);\n");
    
    
    pspl_buffer_addstr(frag, "}\n\n");
    
}

static void generate_hook(const pspl_toolchain_context_t* driver_context,
                          const pspl_ir_state_t* ir_state) {
    
    pspl_buffer_t vert_buf;
    pspl_buffer_init(&vert_buf, 2048);
    generate_vertex(driver_context, ir_state, &vert_buf);
    
    pspl_buffer_t frag_buf;
    pspl_buffer_init(&frag_buf, 2048);
    generate_fragment(driver_context, ir_state, &frag_buf);
    
}

/* Toolchain platform definition */
pspl_toolchain_platform_t GL2_toolplat = {
    .init_hook = init_hook,
    .copyright_hook = copyright_hook,
    .generate_hook = generate_hook
};
