//
//  d3d11_toolchain.c
//  PSPL
//
//  Created by Jack Andersen on 5/25/13.
//
//

#include <PSPLExtension.h>
#include "d3d11_common.h"

static void copyright_hook11() {
    
    pspl_toolchain_provide_copyright("Direct3D 11 PSPL Platform (HLSL Shader Model 5 Generator and Runtime Loader)",
                                     "Copyright (c) 2013 Jack Andersen <jackoalan@gmail.com>",
                                     "[See licence for \"PSPL\" above]");
    
}

static void generate_vertex(const pspl_toolchain_context_t* driver_context,
                            const pspl_ir_state_t* ir_state,
                            pspl_buffer_t* vert) {
    int j;
    
    // Start with preamble
    pspl_buffer_addstr(vert,
                       "/* PSPL AUTO-GENERATED HLSL (SM 5) VERTEX SHADER SOURCE\n"
                       " * generated for `");
    pspl_buffer_addstr(vert, driver_context->pspl_name);
    pspl_buffer_addstr(vert,
                       "` */\n"
                       "\n");
    
    // CONSTANT BUFFER (vertex uniforms)
    pspl_buffer_addstr(vert, "cbuffer VertexUniforms {\n");
    
    // Modelview transform uniform
    char temp[256];
    snprintf(temp, 256, "    float4x4 modelview_mat;\n");
    pspl_buffer_addstr(vert, temp);
    snprintf(temp, 256, "    float4x4 modelview_invtrans_mat;\n");
    pspl_buffer_addstr(vert, temp);
    
    // Bone transforms and base coordinates
    if (ir_state->vertex.bone_count) {
        snprintf(temp, 256, "    float4x4 bone_mat[%u];\n", ir_state->vertex.bone_count);
        pspl_buffer_addstr(vert, temp);
        snprintf(temp, 256, "    float4 bone_base[%u];\n", ir_state->vertex.bone_count);
        pspl_buffer_addstr(vert, temp);
    }
    
    // Projection transform uniform
    pspl_buffer_addstr(vert, "    float4x4 projection_mat;\n");
    
    // Texcoord generator transform uniforms
    if (ir_state->vertex.tc_count) {
        char uniform[64];
        snprintf(uniform, 64, "    float4x4 tc_generator_mats[%u];\n", ir_state->vertex.tc_count);
        pspl_buffer_addstr(vert, uniform);
    }
    
    pspl_buffer_addstr(vert, "};\n\n");
    
    
    // VERTEX BUFFER (vertex attributes)
    pspl_buffer_addstr(vert, "struct VertexAttributes {\n");
    
    // Position attribute
    pspl_buffer_addstr(vert, "    float3 pos : POSITION;\n");
    
    // Normal attribute
    pspl_buffer_addstr(vert, "    float3 norm : NORMAL;\n");
    
    // Texcoord attributes
    for (j=0 ; j<ir_state->total_uv_attr_count ; ++j) {
        char uv[64];
        snprintf(uv, 64, "    float2 uv%u : TEXCOORD%u;\n", j, j);
        pspl_buffer_addstr(vert, uv);
    }
    
    // Bone weight attributes
    unsigned bones_round4 = ROUND_UP_4(ir_state->vertex.bone_count) / 4;
    for (j=0 ; j<bones_round4 ; ++j) {
        char bone[64];
        snprintf(bone, 64, "    float4 bone_weights%u : BLENDWEIGHT%u;\n", j, j);
        pspl_buffer_addstr(vert, bone);
    }
    
    pspl_buffer_addstr(vert, "};\n\n");
    
    
    
    // PIXEL BUFFER (vertex-pixel varying values)
    pspl_buffer_addstr(vert, "struct ToPixel {\n");
    
    // Varying position
    pspl_buffer_addstr(vert, "    float4 position : SV_POSITION;\n");
    
    // Varying normal
    pspl_buffer_addstr(vert, "    float4 normal : NORMAL;\n");
    
    // Varying texcoord linkages
    if (ir_state->vertex.tc_count) {
        char varying_str[128];
        snprintf(varying_str, 128, "    float2 tex_coords[%u] : TEXCOORD;\n", ir_state->vertex.tc_count);
        pspl_buffer_addstr(vert, varying_str);
    }
    pspl_buffer_addstr(vert, "};\n\n");
    
    
    // Main vertex code
    pspl_buffer_addstr(vert, "ToPixel main(in VertexAttributes VERT) {\n");
    pspl_buffer_addstr(vert, "    ToPixel OUT;\n\n");
    
    // Pre-vertex code
    pspl_buffer_addstr(vert, ir_state->vertex.hlsl_pre.buf);
    pspl_buffer_addchar(vert, '\n');
    
    // Position and normal (if no bones)
    if (!ir_state->vertex.bone_count) {
        pspl_buffer_addstr(vert, "    // Non-rigged position and normal\n");
        pspl_buffer_addstr(vert, "    OUT.position = VERT.pos * modelview_mat * projection_mat;\n");
        pspl_buffer_addstr(vert, "    OUT.normal = VERT.norm * modelview_invtrans_mat;\n");
    } else { // Bones
        pspl_buffer_addstr(vert, "    // Rigged position and normal\n");
        pspl_buffer_addstr(vert, "    OUT.position = {0.0,0.0,0.0,0.0};\n");
        pspl_buffer_addstr(vert, "    OUT.normal = {0.0,0.0,0.0,0.0};\n");
        unsigned bone_idx = 0;
        for (j=0 ; j<ir_state->vertex.bone_count ; ++j) {
            char bone[256];
            snprintf(bone, 256, "    OUT.position += (((VERT.pos - bone_base[%u]) * bone_mat[%u]) + bone_base[%u]) * bone_weights%u[%u];\n", j, j, j, bone_idx, j%4);
            pspl_buffer_addstr(vert, bone);
            snprintf(bone, 256, "    OUT.normal += (VERT.norm * bone_mat[%u]) * VERT.bone_weights%u[%u];\n", j, bone_idx, j%4);
            pspl_buffer_addstr(vert, bone);
            if (j && !(j%4)) {++bone_idx;}
        }
        pspl_buffer_addstr(vert, "    OUT.position = OUT.position * modelview_mat * projection_mat;\n");
        pspl_buffer_addstr(vert, "    OUT.normal = normalize(OUT.normal) * modelview_invtrans_mat;\n");
    }
    
    pspl_buffer_addstr(vert, "\n");
    
    // Texcoords
    for (j=0 ; j<ir_state->vertex.tc_count ; ++j) {
        char assign[128];
        if (ir_state->vertex.tc_array[j].tc_source == TEXCOORD_UV)
            snprintf(assign, 128, "    OUT.tex_coords[%u] = ({VERT.uv%u,0,0} * tc_generator_mats[%u]).xy;\n", j, j,
                     ir_state->vertex.tc_array[j].uv_idx);
        else if (ir_state->vertex.tc_array[j].tc_source == TEXCOORD_POS)
            snprintf(assign, 128, "    OUT.tex_coords[%u] = (VERT.pos * tc_generator_mats[%u]).xy;\n", j, j);
        else if (ir_state->vertex.tc_array[j].tc_source == TEXCOORD_NORM)
            snprintf(assign, 128, "    OUT.tex_coords[%u] = (OUT.normal * tc_generator_mats[%u]).xy;\n", j, j);
        pspl_buffer_addstr(vert, assign);
    }
    
    // Post-vertex code
    pspl_buffer_addchar(vert, '\n');
    pspl_buffer_addstr(vert, ir_state->vertex.hlsl_post.buf);
    
    pspl_buffer_addstr(vert, "\n");
    
    pspl_buffer_addstr(vert, "    return OUT;\n");
    pspl_buffer_addstr(vert, "}\n\n");
    
}

static void generate_pixel(const pspl_toolchain_context_t* driver_context,
                           const pspl_ir_state_t* ir_state,
                           pspl_buffer_t* pix) {
    int j;
    
    // Start with preamble
    pspl_buffer_addstr(pix,
                       "/* PSPL AUTO-GENERATED HLSL (SM 5) PIXEL SHADER SOURCE\n"
                       " * generated for `");
    pspl_buffer_addstr(pix, driver_context->pspl_name);
    pspl_buffer_addstr(pix,
                       "` */\n"
                       "\n");
    
    
    
    // PIXEL BUFFER (vertex-pixel varying values)
    pspl_buffer_addstr(pix, "struct ToPixel {\n");
    
    // Varying position
    pspl_buffer_addstr(pix, "    float4 position : SV_POSITION;\n");
    
    // Varying normal
    pspl_buffer_addstr(pix, "    float4 normal : NORMAL;\n");
    
    // Varying texcoord linkages
    if (ir_state->vertex.tc_count) {
        char varying_str[64];
        snprintf(varying_str, 64, "    float2 tex_coords[%u] : TEXCOORD;\n", ir_state->vertex.tc_count);
        pspl_buffer_addstr(pix, varying_str);
    }
    pspl_buffer_addstr(pix, "};\n\n");
    
    
    
    // Texture map uniforms
    pspl_buffer_addstr(pix, "SamplerState CommonSampler;\n");
    if (ir_state->total_texmap_count > MAX_TEX_MAPS)
        pspl_error(-1, "Too Many TEXMAPS", "PSPL Direct3D implementation supports up to %u texmaps; `%s` defines %u",
                   MAX_TEX_MAPS, driver_context->pspl_name, ir_state->total_texmap_count);
    if (ir_state->total_texmap_count) {
        char uniform_str[64];
        snprintf(uniform_str, 64, "Texture2D tex_map[%u];\n\n", ir_state->total_texmap_count);
        pspl_buffer_addstr(pix, uniform_str);
    }
    
    
    
    // Main fragment code
    pspl_buffer_addstr(pix, "float4 main(in ToPixel IN) : COLOR {\n");
    pspl_buffer_addstr(pix, "    float4 OUT;\n\n");
    
    // Pre-fragment code
    pspl_buffer_addstr(pix, ir_state->fragment.hlsl_pre.buf);
    pspl_buffer_addchar(pix, '\n');
    
    // Stage output variables
    for (j=0 ; j<ir_state->fragment.stage_count ; ++j) {
        if (ir_state->fragment.stage_array[j].stage_output == OUT_SIDECHAIN) {
            char sidedef[64];
            snprintf(sidedef, 64, "    float4 stage_side_%d;\n", j);
            pspl_buffer_addstr(pix, sidedef);
        }
    }
    pspl_buffer_addstr(pix, "\n");
    
    
    // Iterate each fragment stage
    for (j=0 ; j<ir_state->fragment.stage_count ; ++j) {
        const pspl_ir_fragment_stage_t* stage = &ir_state->fragment.stage_array[j];
        
        char comment[64];
        snprintf(comment, 64, "    // Stage %u\n", j);
        pspl_buffer_addstr(pix, comment);
        
        char stagedecl[64];
        if (stage->stage_output == OUT_MAIN)
            snprintf(stagedecl, 64, "OUT");
        else if (stage->stage_output == OUT_SIDECHAIN)
            snprintf(stagedecl, 64, "stage_side_%d", j);
        
        char stagesources[3][64];
        int s;
        for (s=0 ; s<3 ; ++s) {
            if (stage->sources[s] == IN_ZERO)
                snprintf(stagesources[s], 64, "{0.0,0.0,0.0,1.0}");
            else if (stage->sources[s] == IN_ONE)
                snprintf(stagesources[s], 64, "{1.0,1.0,1.0,1.0}");
            else if (stage->sources[s] == IN_TEXTURE)
                snprintf(stagesources[s], 64, "tex_map[%u].Sample(CommonSampler, IN.tex_coords[%u])",
                         stage->stage_texmap.texmap_idx, stage->stage_texmap.texcoord_idx);
            else if (stage->sources[s] == IN_LIGHTING)
                snprintf(stagesources[s], 64, "{1.0,1.0,1.0,1.0}");
            else if (stage->sources[s] == IN_COLOUR)
                snprintf(stagesources[s], 64, "{%f,%f,%f,%f}", stage->stage_colour.r,
                         stage->stage_colour.g, stage->stage_colour.b, stage->stage_colour.a);
            else if (stage->sources[s] == IN_MAIN)
                snprintf(stagesources[s], 64, "OUT");
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
            snprintf(stagedef, 256, "    %s = lerp(%s,%s,%s);\n", stagedecl,
                     stagesources[0], stagesources[2], stagesources[1]);
        
        pspl_buffer_addstr(pix, stagedef);
        
        pspl_buffer_addstr(pix, "\n");
    }
    
    // Post-fragment code
    pspl_buffer_addchar(pix, '\n');
    pspl_buffer_addstr(pix, ir_state->fragment.hlsl_post.buf);
    pspl_buffer_addchar(pix, '\n');
    
    pspl_buffer_addstr(pix, "    return OUT;\n");
    pspl_buffer_addstr(pix, "}\n\n");
    
}

static void instruction_hook(const pspl_toolchain_context_t* driver_context,
                             const pspl_extension_t* sending_extension,
                             const char* operation,
                             const void* data) {
    if (!strcmp(operation, "PSPL-IR")) {
        const pspl_ir_state_t* ir_state = data;
        
        // Config structure
        d3d11_config_t config = {
            .uv_attr_count = ir_state->total_uv_attr_count,
            .texmap_count = ir_state->total_texmap_count,
            .texgen_count = ir_state->vertex.tc_count,
            .depth_write = ir_state->depth.write,
            .depth_test = ir_state->depth.test,
            .blending = ir_state->blend.blending,
            .source_factor = ir_state->blend.source_factor,
            .dest_factor = ir_state->blend.dest_factor
        };
        pspl_embed_platform_integer_keyed_object(D3D11_CONFIG_STRUCT, &config, &config, sizeof(d3d11_config_t));
        
        // Vertex shader
        pspl_buffer_t vert_buf;
        pspl_buffer_init(&vert_buf, 2048);
        generate_vertex(driver_context, ir_state, &vert_buf);
        pspl_embed_platform_integer_keyed_object(D3D11_VERTEX_SOURCE, vert_buf.buf, vert_buf.buf,
                                                 vert_buf.buf_cur - vert_buf.buf + 1);
        pspl_buffer_free(&vert_buf);
        
        // Pixel shader
        pspl_buffer_t pix_buf;
        pspl_buffer_init(&pix_buf, 2048);
        generate_pixel(driver_context, ir_state, &pix_buf);
        pspl_embed_platform_integer_keyed_object(D3D11_PIXEL_SOURCE, pix_buf.buf, pix_buf.buf,
                                                 pix_buf.buf_cur - pix_buf.buf + 1);
        pspl_buffer_free(&pix_buf);
    }
}

/* Toolchain platform definition */
pspl_toolchain_platform_t D3D11_toolplat = {
    .copyright_hook = copyright_hook11,
    .instruction_hook = instruction_hook
};

