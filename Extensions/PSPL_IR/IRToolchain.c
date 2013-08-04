//
//  IRToolchain.c
//  PSPL
//
//  Created by Jack Andersen on 6/9/13.
//
//

#include <stdio.h>
#include <PSPLExtension.h>
#include <PSPL/PSPL_IR.h>

/* Global IR staging state */
pspl_ir_state_t pspl_ir_state;

static void copyright_hook() {
    
    pspl_toolchain_provide_copyright("PSPL-IR (Platform-independent shader representation)",
                                     "Copyright (c) 2013 Jack Andersen <jackoalan@gmail.com>",
                                     "[See licence for \"PSPL\" above]");
    
}

static int init(const pspl_toolchain_context_t* driver_context) {
    
    memset(&pspl_ir_state, 0, sizeof(pspl_ir_state));
    
    pspl_ir_state.vertex.in_matrix_def = 0;
    pspl_ir_state.vertex.tc_count = 0;
    pspl_ir_state.vertex.bone_count = 0;
    pspl_ir_state.vertex.shader_def = 0;
    pspl_buffer_init(&pspl_ir_state.vertex.glsl_pre, 512);
    pspl_buffer_init(&pspl_ir_state.vertex.glsl_post, 512);
    pspl_buffer_init(&pspl_ir_state.vertex.hlsl_pre, 512);
    pspl_buffer_init(&pspl_ir_state.vertex.hlsl_post, 512);
    
    pspl_ir_state.depth.test = PLATFORM;
    pspl_ir_state.depth.write = PLATFORM;
    
    pspl_ir_state.fragment.stage_count = 0;
    pspl_ir_state.fragment.shader_def = 0;
    pspl_buffer_init(&pspl_ir_state.fragment.glsl_pre, 512);
    pspl_buffer_init(&pspl_ir_state.fragment.glsl_post, 512);
    pspl_buffer_init(&pspl_ir_state.fragment.hlsl_pre, 512);
    pspl_buffer_init(&pspl_ir_state.fragment.hlsl_post, 512);

    
    pspl_ir_state.blend.blending = PLATFORM;
    pspl_ir_state.blend.source_factor = SRC_ALPHA;
    pspl_ir_state.blend.dest_factor = ONE_MINUS | SRC_ALPHA;
    
    return 0;
    
}

static void shutdown(const pspl_toolchain_context_t* driver_context) {
    
    pspl_buffer_free(&pspl_ir_state.vertex.glsl_pre);
    pspl_buffer_free(&pspl_ir_state.vertex.glsl_post);
    pspl_buffer_free(&pspl_ir_state.vertex.hlsl_pre);
    pspl_buffer_free(&pspl_ir_state.vertex.hlsl_post);
    
    pspl_buffer_free(&pspl_ir_state.fragment.glsl_pre);
    pspl_buffer_free(&pspl_ir_state.fragment.glsl_post);
    pspl_buffer_free(&pspl_ir_state.fragment.hlsl_pre);
    pspl_buffer_free(&pspl_ir_state.fragment.hlsl_post);
    
}


/* VERTEX tests */
static inline int is_vert(const char* str) {return (*str == 'v' ||
                                                    *str == 'V');}

static inline int is_matrix(const char* str) {return (*str == 'm' ||
                                                      *str == 'M');}

static inline int is_endmatrix(const char* str) {return !strncasecmp(str, "END", 3) &&
                                                        (str[3] == 'm' ||
                                                         str[3] == 'M');}

static inline int is_position(const char* str) {return (*str == 'p' ||
                                                        *str == 'P');}

static inline int is_normal(const char* str) {return (*str == 'n' ||
                                                      *str == 'N');}

static inline int is_texcoord(const char* str) {return (*str == 't' ||
                                                        *str == 'T');}

static inline int is_bonecount(const char* str) {return (*str == 'b' ||
                                                         *str == 'B');}

static inline int is_translation(const char* str) {return !strncasecmp(str, "TRANS", 5);}

static inline int is_rotation(const char* str) {return !strncasecmp(str, "ROT", 3);}

static inline int is_scale(const char* str) {return (*str == 's' ||
                                                     *str == 'S');}


/* DEPTH tests */
static inline int is_test(const char* str) {return (*str == 't' ||
                                                    *str == 'T');}

static inline int is_write(const char* str) {return (*str == 'w' ||
                                                     *str == 'W');}

/* FRAGMENT tests */
static inline int is_fragment(const char* str) {return (*str == 'f' ||
                                                        *str == 'F');}

/* BLEND tests */
static inline int is_inverse(const char* str) {return (*str == 'i' ||
                                                       *str == 'I');}

static inline int is_source(const char* str) {return (*str == 's' ||
                                                      *str == 'S');}

static inline int is_dest(const char* str) {return (*str == 'd' ||
                                                    *str == 'D');}

static inline int is_colour(const char* str) {return (*str == 'c' ||
                                                      *str == 'C');}

static inline int is_alpha(const char* str) {return (*str == 'a' ||
                                                     *str == 'A');}

/* Feature enable/disable test */
static enum pspl_feature feature_value(const char* str) {
    if (*str == 'e' || *str == 'E' || !strcasecmp(str, "ON") || !strcasecmp(str, "YES"))
        return ENABLED;
    if (*str == 'd' || *str == 'D' || !strcasecmp(str, "OFF") || !strcasecmp(str, "NO"))
        return DISABLED;
    if (*str == 'p' || *str == 'P')
        return PLATFORM;
    
    if (strtol(str, NULL, 10))
        return ENABLED;
    else
        return DISABLED;
}


/* Lookup sidechain index from name */
static unsigned lookup_sidechain(const char* name) {
    int i;
    
    for (i=0 ; i<pspl_ir_state.fragment.stage_count ; ++i) {
        if (pspl_ir_state.fragment.stage_array[i].stage_output == OUT_SIDECHAIN)
            if (!strcasecmp(pspl_ir_state.fragment.stage_array[i].side_out_name, name))
                return i;
    }
    
    pspl_error(-1, "Unable to find sidechain value", "`%s` not previously defined", name);
    return 0;
    
}

/* Handle incoming command call */
static void command_call(const pspl_toolchain_context_t* driver_context,
                         const pspl_toolchain_heading_context_t* current_heading,
                         const char* command_name,
                         unsigned int command_argc,
                         const char** command_argv) {
    
    
    if (is_vert(current_heading->heading_name)) {
        
        // Bone count
        if (is_bonecount(command_name)) {
            if (!command_argc)
                pspl_error(-1, "Invalid Command Usage", "`BONE_COUNT` must have at least *one* argument");
            else
                pspl_ir_state.vertex.bone_count = abs((int)strtol(command_argv[0], NULL, 10));
            return;
        } else if (is_texcoord(command_name)) {
            
            if (command_argc >= 2) {
                
                char* nametest = (char*)command_argv[0];
                long name_idx = strtol(command_argv[0], &nametest, 10);
                if ((unsigned)name_idx >= 10)
                    pspl_error(-1, "Invalid TEX_COORD usage", "texcoord generator index must be in range [0,9]");
                
                if (nametest == command_argv[0])
                    pspl_error(-1, "Invalid TEX_COORD usage", "first argument must be texcoord generator index");
                
                char* numtest = (char*)command_argv[1];
                long idx = strtol(command_argv[1], &numtest, 10);
                
                if (is_position(command_argv[1])) {
                    
                    pspl_ir_state.vertex.tc_array[pspl_ir_state.vertex.tc_count].name_idx = (unsigned)name_idx;
                    pspl_ir_state.vertex.tc_array[pspl_ir_state.vertex.tc_count].tc_source = TEXCOORD_POS;
                    ++pspl_ir_state.vertex.tc_count;
                    
                } else if (is_normal(command_argv[1])) {
                    
                    pspl_ir_state.vertex.tc_array[pspl_ir_state.vertex.tc_count].name_idx = (unsigned)name_idx;
                    pspl_ir_state.vertex.tc_array[pspl_ir_state.vertex.tc_count].tc_source = TEXCOORD_NORM;
                    ++pspl_ir_state.vertex.tc_count;
                    
                } else if (numtest != command_argv[1]) {
                    
                    pspl_ir_state.vertex.tc_array[pspl_ir_state.vertex.tc_count].name_idx = (unsigned)name_idx;
                    pspl_ir_state.vertex.tc_array[pspl_ir_state.vertex.tc_count].tc_source = TEXCOORD_UV;
                    pspl_ir_state.vertex.tc_array[pspl_ir_state.vertex.tc_count].uv_idx = (unsigned)idx;
                    ++pspl_ir_state.vertex.tc_count;
                    
                } else
                    pspl_error(-1, "Invalid TEX_COORD usage", "there must be *three* arguments with the UV generator index and a numeric index [0-7] indicating which UV layer should be used as coordinate source. Alternatively, the keyword `POSITION` or `NORMAL` may be used for dynamic coordinate source");
                
            } else
                pspl_error(-1, "Invalid TEX_COORD usage", "there must be *two* arguments: TEX_COORD(<name>, <uv source>)");
            
        } else if (!strcasecmp(current_heading->heading_name, "SHADER")) {
            
            // Begin a vertex shader
            if (command_argc < 2)
                pspl_error(-1, "Invalid SHADER command usage", "there must be *two* arguments ([GLSL, HLSL], [PRE, POST])");
            
            if (!strcasecmp(command_argv[0], "GLSL")) {
                if (!strcasecmp(command_argv[1], "PRE"))
                    pspl_ir_state.vertex.shader_def = DEF_GLSL_VERT_PRE;
                else if (!strcasecmp(command_argv[1], "POST"))
                    pspl_ir_state.vertex.shader_def = DEF_GLSL_VERT_POST;
                else
                    pspl_error(-1, "Invalid SHADER token", "'%s' is not PRE or POST", command_argv[1]);

            } else if (!strcasecmp(command_argv[0], "HLSL")) {
                if (!strcasecmp(command_argv[1], "PRE"))
                    pspl_ir_state.vertex.shader_def = DEF_HLSL_VERT_PRE;
                else if (!strcasecmp(command_argv[1], "POST"))
                    pspl_ir_state.vertex.shader_def = DEF_HLSL_VERT_POST;
                else
                    pspl_error(-1, "Invalid SHADER token", "'%s' is not PRE or POST", command_argv[1]);
                
            } else
                pspl_error(-1, "Invalid SHADER token", "'%s' is not GLSL or HLSL", command_argv[0]);
            
            pspl_toolchain_line_read_only();
            
        } else
            pspl_error(-1, "Invalid VERTEX heading usage",
                       "under VERTEX heading, a command must be specified (BONE_COUNT or TEX_COORD)");
        
    } else if (!strcasecmp(current_heading->heading_name, "DEPTH")) {
        
        if (is_test(command_name)) {
            if (!command_argc)
                pspl_error(-1, "Invalid command usage",
                           "the depth TEST command must have *one* boolean argument or 'PLATFORM'");
            pspl_ir_state.depth.test = feature_value(command_argv[0]);
        } else if (is_write(command_name)) {
            if (!command_argc)
                pspl_error(-1, "Invalid command usage",
                           "the depth WRITE command must have *one* boolean argument or 'PLATFORM'");
            pspl_ir_state.depth.write = feature_value(command_argv[0]);
        } else
            pspl_error(-1, "Unrecognised DEPTH command",
                       "`%s` command not recognised by DEPTH heading", command_name);
        
    } else if (is_fragment(current_heading->heading_name)) {
        
        
        if (!strcasecmp(command_name, "SAMPLE")) {
            // Texture sample definition
            if (command_argc < 2)
                pspl_error(-1, "Invalid command use",
                           "`SAMPLE` must specify 2 arguments in `SAMPLE(<MAPIDX> <UVIDX>)` format");
            
            
            unsigned stage_idx = pspl_ir_state.fragment.stage_count;
            
            if (!stage_idx) {
                // If first stage, insert initial SET stage
                
                pspl_ir_state.fragment.stage_array[0].stage_output = OUT_MAIN;
                pspl_ir_state.fragment.stage_array[0].stage_op = OP_SET;
                
                pspl_ir_state.fragment.stage_array[0].sources[0] = IN_TEXTURE;
                
                // This is used to ensure an operator line is specified
                pspl_ir_state.fragment.def_stage_op = OP_SET;
                pspl_ir_state.fragment.def_stage_op_idx = 0;
                ++pspl_ir_state.fragment.stage_count;
                
            } else {
                // Otherwise determine stage accordingly
                
                if (pspl_ir_state.fragment.def_stage_op == OP_SET)
                    pspl_error(-1, "Invalid FRAGMENT definition",
                               "there needs to be an operator line separating fragment stages");
                
                else if (pspl_ir_state.fragment.def_stage_op == OP_MUL ||
                         pspl_ir_state.fragment.def_stage_op == OP_ADD ||
                         pspl_ir_state.fragment.def_stage_op == OP_SUB) {
                    
                    
                    pspl_ir_state.fragment.stage_array[stage_idx].sources[1] = IN_TEXTURE;
                    
                    // This is used to ensure an operator line is specified
                    pspl_ir_state.fragment.def_stage_op = OP_SET;
                    pspl_ir_state.fragment.def_stage_op_idx = 0;
                    ++pspl_ir_state.fragment.stage_count;
                    
                }
                
                else if (pspl_ir_state.fragment.def_stage_op == OP_BLEND) {
                    
                    if (pspl_ir_state.fragment.def_stage_op_idx == 1) {
                        
                        // This is the blend coefficient operand
                        pspl_ir_state.fragment.stage_array[stage_idx].sources[1] = IN_TEXTURE;
                        
                    } else if (pspl_ir_state.fragment.def_stage_op_idx == 2) {
                        
                        if (pspl_ir_state.fragment.stage_array[stage_idx].using_texture) {
                            
                            // We need to spill onto next stage (as plain multiplication)
                            // if two textures specified in blend
                            pspl_ir_state.fragment.stage_array[stage_idx].stage_output = OUT_MAIN;
                            ++pspl_ir_state.fragment.stage_count;
                            ++stage_idx;
                            pspl_ir_state.fragment.stage_array[stage_idx].stage_op = OP_MUL;
                            pspl_ir_state.fragment.stage_array[stage_idx].sources[0] = IN_MAIN;
                            pspl_ir_state.fragment.stage_array[stage_idx].sources[1] = IN_TEXTURE;
                            
                        } else {
                            
                            // Otherwise consolidate texture here
                            pspl_ir_state.fragment.stage_array[stage_idx].sources[2] = IN_TEXTURE;
                            
                        }
                        
                        // This is used to ensure an operator line is specified
                        pspl_ir_state.fragment.def_stage_op = OP_SET;
                        pspl_ir_state.fragment.def_stage_op_idx = 0;
                        ++pspl_ir_state.fragment.stage_count;
                        
                    }
                    
                    
                }
                
                
            }
            
            // Now populate texture map info
            char* nametest = (char*)command_argv[0];
            int map = (int)strtol(command_argv[0], &nametest, 10);
            if (nametest == command_argv[0])
                pspl_error(-1, "Invalid SAMPLE value", "map index must be positive");
            pspl_ir_state.fragment.stage_array[stage_idx].stage_texmap.texmap_idx = map;
            
            nametest = (char*)command_argv[1];
            int tcidx = (int)strtol(command_argv[1], &nametest, 10);
            if (nametest == command_argv[1])
                pspl_error(-1, "Invalid SAMPLE value", "texgen index must be positive");
            pspl_ir_state.fragment.stage_array[stage_idx].stage_texmap.texcoord_idx = tcidx;
            pspl_ir_state.fragment.stage_array[stage_idx].using_texture = 1;
            
            
        } else if (!strcasecmp(command_name, "RGBA")) {
            // RGBA constant definition
            if (command_argc < 4)
                pspl_error(-1, "Invalid command use",
                           "`RGBA` must specify 4 arguments in `RGBA(<RED> <GREEN> <BLUE> <ALPHA>)` format");
            
            
            unsigned stage_idx = pspl_ir_state.fragment.stage_count;
            
            if (!stage_idx) {
                // If first stage, insert initial SET stage
                
                pspl_ir_state.fragment.stage_array[0].stage_output = OUT_MAIN;
                pspl_ir_state.fragment.stage_array[0].stage_op = OP_SET;
                
                pspl_ir_state.fragment.stage_array[0].sources[0] = IN_COLOUR;
                
                // This is used to ensure an operator line is specified
                pspl_ir_state.fragment.def_stage_op = OP_SET;
                pspl_ir_state.fragment.def_stage_op_idx = 0;
                ++pspl_ir_state.fragment.stage_count;
                
            } else {
                // Otherwise determine stage accordingly
                
                if (pspl_ir_state.fragment.def_stage_op == OP_SET)
                    pspl_error(-1, "Invalid FRAGMENT definition",
                               "there needs to be an operator line separating fragment stages");
                
                else if (pspl_ir_state.fragment.def_stage_op == OP_MUL ||
                         pspl_ir_state.fragment.def_stage_op == OP_ADD ||
                         pspl_ir_state.fragment.def_stage_op == OP_SUB) {
                    
                    
                    pspl_ir_state.fragment.stage_array[stage_idx].sources[1] = IN_COLOUR;
                    
                    // This is used to ensure an operator line is specified
                    pspl_ir_state.fragment.def_stage_op = OP_SET;
                    pspl_ir_state.fragment.def_stage_op_idx = 0;
                    ++pspl_ir_state.fragment.stage_count;
                    
                }
                
                else if (pspl_ir_state.fragment.def_stage_op == OP_BLEND) {
                    
                    if (pspl_ir_state.fragment.def_stage_op_idx == 1) {
                        
                        // This is the blend coefficient operand
                        pspl_ir_state.fragment.stage_array[stage_idx].sources[1] = IN_COLOUR;
                        
                    } else if (pspl_ir_state.fragment.def_stage_op_idx == 2) {
                        
                        if (pspl_ir_state.fragment.stage_array[stage_idx].using_colour) {
                            
                            // We need to spill onto next stage (as plain multiplication)
                            // if two textures specified in blend
                            pspl_ir_state.fragment.stage_array[stage_idx].stage_output = OUT_MAIN;
                            ++pspl_ir_state.fragment.stage_count;
                            ++stage_idx;
                            pspl_ir_state.fragment.stage_array[stage_idx].stage_op = OP_MUL;
                            pspl_ir_state.fragment.stage_array[stage_idx].sources[0] = IN_MAIN;
                            pspl_ir_state.fragment.stage_array[stage_idx].sources[1] = IN_COLOUR;
                            
                        } else {
                            
                            // Otherwise consolidate colour here
                            pspl_ir_state.fragment.stage_array[stage_idx].sources[2] = IN_COLOUR;
                            
                        }
                        
                        // This is used to ensure an operator line is specified
                        pspl_ir_state.fragment.def_stage_op = OP_SET;
                        pspl_ir_state.fragment.def_stage_op_idx = 0;
                        ++pspl_ir_state.fragment.stage_count;
                        
                    }
                    
                    
                }
                
                
            }
            
            
            // Now populate colour info
            double r,g,b,a;
            
            r = strtof(command_argv[0], NULL);
            if (r < 0.0 || r > 1.0)
                pspl_error(-1, "Invalid colour value", "Red value in `RGBA` must be a normalised [0,1] float value");
            g = strtof(command_argv[1], NULL);
            if (r < 0.0 || r > 1.0)
                pspl_error(-1, "Invalid colour value", "Green value in `RGBA` must be a normalised [0,1] float value");
            b = strtof(command_argv[2], NULL);
            if (r < 0.0 || r > 1.0)
                pspl_error(-1, "Invalid colour value", "Blue value in `RGBA` must be a normalised [0,1] float value");
            a = strtof(command_argv[3], NULL);
            if (r < 0.0 || r > 1.0)
                pspl_error(-1, "Invalid colour value", "Alpha value in `RGBA` must be a normalised [0,1] float value");
            
            pspl_ir_state.fragment.stage_array[stage_idx].stage_colour.r = r;
            pspl_ir_state.fragment.stage_array[stage_idx].stage_colour.g = g;
            pspl_ir_state.fragment.stage_array[stage_idx].stage_colour.b = b;
            pspl_ir_state.fragment.stage_array[stage_idx].stage_colour.a = a;
            pspl_ir_state.fragment.stage_array[stage_idx].using_colour = 1;
            
            
        } else if (!strcasecmp(command_name, "SIDE")) {
            // Use previously-set named sidechain as the operand source
            if (!command_argc)
                pspl_error(-1, "Invalid command usage",
                           "FRAGMENT SIDE command needs *one* operand");
            
            
            unsigned stage_idx = pspl_ir_state.fragment.stage_count;
            
            if (!stage_idx)
                pspl_error(-1, "Invalid SIDE usage",
                           "impossible to use sidechain in first stage");
            
            else {
                // Otherwise determine stage accordingly
                
                if (pspl_ir_state.fragment.def_stage_op == OP_SET)
                    pspl_error(-1, "Invalid FRAGMENT definition",
                               "there needs to be an operator line separating fragment stages");
                
                else if (pspl_ir_state.fragment.def_stage_op == OP_MUL ||
                         pspl_ir_state.fragment.def_stage_op == OP_ADD ||
                         pspl_ir_state.fragment.def_stage_op == OP_SUB) {
                    
                    pspl_ir_state.fragment.stage_array[stage_idx].sources[1] = IN_SIDECHAIN;
                    pspl_ir_state.fragment.stage_array[stage_idx].side_in_indices[1] = lookup_sidechain(command_argv[0]);
                    
                    // This is used to ensure an operator line is specified
                    pspl_ir_state.fragment.def_stage_op = OP_SET;
                    pspl_ir_state.fragment.def_stage_op_idx = 0;
                    ++pspl_ir_state.fragment.stage_count;
                    
                }
                
                else if (pspl_ir_state.fragment.def_stage_op == OP_BLEND) {
                    
                    if (pspl_ir_state.fragment.def_stage_op_idx == 1) {
                        
                        // This is the blend coefficient operand
                        pspl_ir_state.fragment.stage_array[stage_idx].sources[1] = IN_SIDECHAIN;
                        pspl_ir_state.fragment.stage_array[stage_idx].side_in_indices[1] = lookup_sidechain(command_argv[0]);
                        
                    } else if (pspl_ir_state.fragment.def_stage_op_idx == 2) {
                        
                        pspl_ir_state.fragment.stage_array[stage_idx].sources[2] = IN_SIDECHAIN;
                        pspl_ir_state.fragment.stage_array[stage_idx].side_in_indices[2] = lookup_sidechain(command_argv[0]);
                        
                        
                        // This is used to ensure an operator line is specified
                        pspl_ir_state.fragment.def_stage_op = OP_SET;
                        pspl_ir_state.fragment.def_stage_op_idx = 0;
                        ++pspl_ir_state.fragment.stage_count;
                        
                    }
                    
                    
                }
                
                
            }
            
        } else if (!strcasecmp(command_name, "SHADER")) {
            
            // Begin a fragment shader
            if (command_argc < 2)
                pspl_error(-1, "Invalid SHADER command usage", "there must be *two* arguments ([GLSL, HLSL], [PRE, POST])");
            
            if (!strcasecmp(command_argv[0], "GLSL")) {
                if (!strcasecmp(command_argv[1], "PRE"))
                    pspl_ir_state.fragment.shader_def = DEF_GLSL_FRAG_PRE;
                else if (!strcasecmp(command_argv[1], "POST"))
                    pspl_ir_state.fragment.shader_def = DEF_GLSL_FRAG_POST;
                else
                    pspl_error(-1, "Invalid SHADER token", "'%s' is not PRE or POST", command_argv[1]);
                
            } else if (!strcasecmp(command_argv[0], "HLSL")) {
                if (!strcasecmp(command_argv[1], "PRE"))
                    pspl_ir_state.fragment.shader_def = DEF_HLSL_FRAG_PRE;
                else if (!strcasecmp(command_argv[1], "POST"))
                    pspl_ir_state.fragment.shader_def = DEF_HLSL_FRAG_POST;
                else
                    pspl_error(-1, "Invalid SHADER token", "'%s' is not PRE or POST", command_argv[1]);
                
            } else
                pspl_error(-1, "Invalid SHADER token", "'%s' is not GLSL or HLSL", command_argv[0]);
            
            pspl_toolchain_line_read_only();
            
        } else
            pspl_error(-1, "Unrecognised FRAGMENT command",
                       "`%s` command not recognised by FRAGMENT heading", command_name);
        
        
        
    } else if (!strcasecmp(current_heading->heading_name, "BLEND")) {
        
        if (command_argc >= 2) {
            
            if (is_source(command_name)) {
                
                pspl_ir_state.blend.blending = 1;
                pspl_ir_state.blend.source_factor = 0;
                
                if (command_argc >= 3 && is_inverse(command_argv[0])) {
                    pspl_ir_state.blend.source_factor = ONE_MINUS;
                    ++command_argv;
                }
                
                if (is_source(command_argv[0])) {
                    
                    if (is_colour(command_argv[1])) {
                        pspl_ir_state.blend.source_factor |= SRC_COLOUR;
                        return;
                    } else if (is_alpha(command_argv[1])) {
                        pspl_ir_state.blend.source_factor |= SRC_ALPHA;
                        return;
                    }
                    
                } else if (is_dest(command_argv[0])) {
                    
                    if (is_colour(command_argv[1])) {
                        pspl_ir_state.blend.source_factor |= DST_COLOUR;
                        return;
                    } else if (is_alpha(command_argv[1])) {
                        pspl_ir_state.blend.source_factor |= DST_ALPHA;
                        return;
                    }
                    
                }
                
                
            } else if (is_dest(command_name)) {
                
                pspl_ir_state.blend.blending = 1;
                pspl_ir_state.blend.dest_factor = 0;
                
                if (command_argc >= 3 && is_inverse(command_argv[0])) {
                    pspl_ir_state.blend.dest_factor = ONE_MINUS;
                    ++command_argv;
                }
                
                if (is_source(command_argv[0])) {
                    
                    if (is_colour(command_argv[1])) {
                        pspl_ir_state.blend.dest_factor |= SRC_COLOUR;
                        return;
                    } else if (is_alpha(command_argv[1])) {
                        pspl_ir_state.blend.dest_factor |= SRC_ALPHA;
                        return;
                    }
                    
                } else if (is_dest(command_argv[0])) {
                    
                    if (is_colour(command_argv[1])) {
                        pspl_ir_state.blend.dest_factor |= DST_COLOUR;
                        return;
                    } else if (is_alpha(command_argv[1])) {
                        pspl_ir_state.blend.dest_factor |= DST_ALPHA;
                        return;
                    }
                    
                }
                
            } else
                pspl_error(-1, "Unrecognised BLEND command",
                           "`%s` command not recognised by BLEND heading", command_name);
            
        }
        
        pspl_error(-1, "Invalid command use",
                   "`%s` command must follow `%s([INVERSE] <SOURCE,DESTINATION> <COLOUR,ALPHA>)` format",
                   command_name, command_name);
        
    }
    
    
}

/* Set output sidechain */
static void set_out_sidechain(const char* line_text) {
    
    ++line_text;
    while (*line_text == ' ' || *line_text == '\t')
        ++line_text;
    
    char* dest_str = pspl_ir_state.fragment.stage_array[pspl_ir_state.fragment.stage_count].side_out_name;
    strlcpy(dest_str, line_text, IR_NAME_LEN);
    
    while (*dest_str != ' ' && *dest_str != '\t')
        ++dest_str;
    *dest_str = '\0';
    
    pspl_ir_state.fragment.stage_array[pspl_ir_state.fragment.stage_count].stage_output = OUT_SIDECHAIN;
    
}

/* Handle incoming line-read */
static void line_read(const pspl_toolchain_context_t* driver_context,
                      const pspl_toolchain_heading_context_t* current_heading,
                      const char* line_text) {
    int i;
    
    // Check for shader end
    int shader_end = 0;
    const char* token = line_text;
    while (*token == ' ' || *token == '\t')
        ++token;
    char testbuf[10];
    strncpy(testbuf, token, 10);
    testbuf[9] = '\0';
    if (!strcasecmp(testbuf, "ENDSHADER"))
        shader_end = 1;
    
    // If defining shader code, pass through to shader buffer
    pspl_buffer_t* target_buf = NULL;
    if (pspl_ir_state.vertex.shader_def) {
        if (shader_end) {
            pspl_ir_state.vertex.shader_def = DEF_NONE;
            pspl_toolchain_line_read_auto();
            return;
        }
        
        if (pspl_ir_state.vertex.shader_def == DEF_GLSL_VERT_PRE)
            target_buf = &pspl_ir_state.vertex.glsl_pre;
        else if (pspl_ir_state.vertex.shader_def == DEF_GLSL_VERT_POST)
            target_buf = &pspl_ir_state.vertex.glsl_post;
        else if (pspl_ir_state.vertex.shader_def == DEF_HLSL_VERT_PRE)
            target_buf = &pspl_ir_state.vertex.hlsl_pre;
        else if (pspl_ir_state.vertex.shader_def == DEF_HLSL_VERT_POST)
            target_buf = &pspl_ir_state.vertex.hlsl_post;
        
    } else if (pspl_ir_state.fragment.shader_def) {
        if (shader_end) {
            pspl_ir_state.fragment.shader_def = DEF_NONE;
            pspl_toolchain_line_read_auto();
            return;
        }
        
        if (pspl_ir_state.fragment.shader_def == DEF_GLSL_FRAG_PRE)
            target_buf = &pspl_ir_state.fragment.glsl_pre;
        else if (pspl_ir_state.fragment.shader_def == DEF_GLSL_FRAG_POST)
            target_buf = &pspl_ir_state.fragment.glsl_post;
        else if (pspl_ir_state.fragment.shader_def == DEF_HLSL_FRAG_PRE)
            target_buf = &pspl_ir_state.fragment.hlsl_pre;
        else if (pspl_ir_state.fragment.shader_def == DEF_HLSL_FRAG_POST)
            target_buf = &pspl_ir_state.fragment.hlsl_post;
        
    }
    
    if (target_buf) {
        pspl_buffer_addstr(target_buf, "    ");
        pspl_buffer_addstr(target_buf, line_text);
        pspl_buffer_addchar(target_buf, '\n');
        
        return;
    }
    
    // Matrix or operator definition
    if (current_heading->heading_level == 1) {
        
        if (is_vert(current_heading->heading_trace->heading_name)) {
            
            if (pspl_ir_state.vertex.in_matrix_def) {
                
                // Parse matrix row
                while (*line_text == ' ' || *line_text == '\t')
                    ++line_text;
                
                if (*line_text == '-') {
                    while (*line_text == ' ' || *line_text == '\t' || *line_text == '-')
                        ++line_text;
                    if (*line_text == '\0' || *line_text == '\n')
                        return;
                    else
                        pspl_error(-1, "Invalid matrix border line",
                                   "matrix border line must consist of all '-'s or whitespace");
                }
                
                if (pspl_ir_state.vertex.matrix_row_count >= 3)
                    pspl_error(-1, "Too many matrix rows defined", "only 3x4 matrices accepted");
                
                for (i=0 ; i<4 ; ++i) {
                    while (*line_text == ' ' || *line_text == '\t' || *line_text == '|')
                        ++line_text;
                    if (*line_text == '\0' || *line_text == '\n')
                        pspl_error(-1, "Insufficient matrix columns defined",
                                   "there needs to be 4 columns in each matrix row");
                    pspl_ir_state.vertex.matrix[pspl_ir_state.vertex.matrix_row_count][i] =
                    strtof(line_text, (char**)&line_text);
                }
                
                ++pspl_ir_state.vertex.matrix_row_count;
                return;
                
            }
            
        }
        
    } else if (current_heading->heading_level == 0) {
        
        if (is_fragment(current_heading->heading_name)) {
            
            // Parse operator row
            while (*line_text == ' ' || *line_text == '\t')
                ++line_text;
            
            if (pspl_ir_state.fragment.def_stage_op_idx == 0) {
                
                // Addition (or subtraction)
                if (*line_text == '+') {
                    ++line_text;
                    if (*line_text == '-') {
                        
                        pspl_ir_state.fragment.def_stage_op = OP_SUB;
                        pspl_ir_state.fragment.stage_array[pspl_ir_state.fragment.stage_count].stage_op = OP_SUB;
                        pspl_ir_state.fragment.stage_array[pspl_ir_state.fragment.stage_count].sources[0] = IN_MAIN;
                        ++pspl_ir_state.fragment.def_stage_op_idx;

                        while (*line_text == ' ' || *line_text == '\t' || *line_text == '-')
                            ++line_text;
                        
                        if (*line_text == '\0' || *line_text == '\n') {
                            
                            return;
                            
                        } else if (*line_text == '>') {
                            
                            set_out_sidechain(line_text);
                            return;
                            
                        } else
                            pspl_error(-1, "Invalid operation line",
                                       "subtraction operation line must consist of '+' followed by '-'s or whitespace");
                        
                    } else {
                        
                        pspl_ir_state.fragment.def_stage_op = OP_ADD;
                        pspl_ir_state.fragment.stage_array[pspl_ir_state.fragment.stage_count].stage_op = OP_ADD;
                        pspl_ir_state.fragment.stage_array[pspl_ir_state.fragment.stage_count].sources[0] = IN_MAIN;
                        ++pspl_ir_state.fragment.def_stage_op_idx;

                        while (*line_text == ' ' || *line_text == '\t' || *line_text == '+')
                            ++line_text;
                        if (*line_text == '\0' || *line_text == '\n') {
                            
                            return;
                            
                        } else if (*line_text == '>') {
                            
                            set_out_sidechain(line_text);
                            return;
                            
                        } else
                            pspl_error(-1, "Invalid operation line",
                                       "addition operation line must consist of '+'s or whitespace");
                        
                    }
                }
                
                // Multiplication
                else if (*line_text == '*') {
                    
                    pspl_ir_state.fragment.def_stage_op = OP_MUL;
                    pspl_ir_state.fragment.stage_array[pspl_ir_state.fragment.stage_count].stage_op = OP_MUL;
                    pspl_ir_state.fragment.stage_array[pspl_ir_state.fragment.stage_count].sources[0] = IN_MAIN;
                    ++pspl_ir_state.fragment.def_stage_op_idx;
                    
                    while (*line_text == ' ' || *line_text == '\t' || *line_text == '*')
                        ++line_text;
                    if (*line_text == '\0' || *line_text == '\n') {
                        
                        return;
                        
                    } else if (*line_text == '>') {
                        
                        set_out_sidechain(line_text);
                        return;
                        
                    } else
                        pspl_error(-1, "Invalid operation line",
                                   "multiplication operation line must consist of '*'s or whitespace");
                }
                
                // Blend
                else if (*line_text == '%') {
                    
                    pspl_ir_state.fragment.def_stage_op = OP_BLEND;
                    pspl_ir_state.fragment.stage_array[pspl_ir_state.fragment.stage_count].stage_op = OP_BLEND;
                    pspl_ir_state.fragment.stage_array[pspl_ir_state.fragment.stage_count].sources[0] = IN_MAIN;
                    ++pspl_ir_state.fragment.def_stage_op_idx;
                    
                    while (*line_text == ' ' || *line_text == '\t' || *line_text == '%')
                        ++line_text;
                    if (*line_text == '\0' || *line_text == '\n') {
                        
                        return;
                        
                    } else if (*line_text == '>') {
                        
                        set_out_sidechain(line_text);
                        return;
                        
                    } else
                        pspl_error(-1, "Invalid operation line",
                                   "blend operation line must consist of '%c's or whitespace", '%');
                }
                
            } else if (pspl_ir_state.fragment.def_stage_op_idx == 1) {
                
                // Must be blend (it's the only 3-operand operation)
                if (*line_text != '%')
                    pspl_error(-1, "Invalid BLEND operation usage",
                               "there must be two instances of '%c' operation lines with blend operation", '%');
                
            }
                        
        }
        
    }
    
    // Whitespace lines are OK
    while (*line_text == ' ' || *line_text == '\t')
        ++line_text;
    if (*line_text == '\0' || *line_text == '\n')
        return;
    
    // Anything else is not
    pspl_error(-1, "Unrecognised line format", "PSPL-IR doesn't recognise `%s`", line_text);
    
}

/* Called when done */
static void platform_instruct(const pspl_toolchain_context_t* driver_context) {
    int j;
    
    // Calculate total UV attr count
    pspl_ir_state.total_uv_attr_count = 0;
    for (j=0 ; j<pspl_ir_state.vertex.tc_count ; ++j) {
        if (pspl_ir_state.vertex.tc_array[j].tc_source == TEXCOORD_UV)
            if (pspl_ir_state.total_uv_attr_count <= pspl_ir_state.vertex.tc_array[j].uv_idx)
                pspl_ir_state.total_uv_attr_count = pspl_ir_state.vertex.tc_array[j].uv_idx + 1;
    }
    
    // Calculate total texmap count
    pspl_ir_state.total_texmap_count = 0;
    for (j=0 ; j<pspl_ir_state.fragment.stage_count ; ++j) {
        if (pspl_ir_state.fragment.stage_array[j].using_texture)
            if (pspl_ir_state.total_texmap_count <= pspl_ir_state.fragment.stage_array[j].stage_texmap.texmap_idx)
                pspl_ir_state.total_texmap_count = pspl_ir_state.fragment.stage_array[j].stage_texmap.texmap_idx + 1;
        if (pspl_ir_state.fragment.stage_array[j].using_indirect)
            if (pspl_ir_state.total_texmap_count <= pspl_ir_state.fragment.stage_array[j].stage_indtexmap.texmap_idx)
                pspl_ir_state.total_texmap_count = pspl_ir_state.fragment.stage_array[j].stage_indtexmap.texmap_idx + 1;
    }
    
    // Write chains into PSPLC
    //pspl_calc_chain_write_position(&pspl_ir_state.vertex.pos_chain);
    //for (i=0 ; i<pspl_ir_state.vertex.tc_count ; ++i)
        //pspl_calc_chain_write_uv(&pspl_ir_state.vertex.tc_array[i].tc_chain, i);
    
    pspl_toolchain_send_platform_instruction("PSPL-IR", &pspl_ir_state);
}


static const char* claimed_headings[] = {
    "VERTEX",
    "DEPTH",
    "FRAGMENT",
    "BLEND",
    NULL};

pspl_toolchain_extension_t PSPL_IR_toolext = {
    .copyright_hook = copyright_hook,
    .init_hook = init,
    .platform_instruct_hook = platform_instruct,
    .command_call_hook = command_call,
    .line_read_hook = line_read,
    .claimed_heading_names = claimed_headings
};
