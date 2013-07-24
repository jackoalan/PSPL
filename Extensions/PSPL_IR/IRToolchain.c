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
    int i;
    
    memset(&pspl_ir_state, 0, sizeof(pspl_ir_state));
    
    pspl_ir_state.vertex.in_matrix_def = 0;
    pspl_ir_state.vertex.tc_count = 0;
    pspl_calc_chain_init(&pspl_ir_state.vertex.pos_chain);
    for (i=0 ; i<MAX_TEX_COORDS ; ++i) {
        pspl_ir_state.vertex.tc_array[i].resolved_name_idx = -1;
        pspl_calc_chain_init(&pspl_ir_state.vertex.tc_array[i].tc_chain);
    }
    
    pspl_ir_state.depth.test = PLATFORM;
    pspl_ir_state.depth.write = PLATFORM;
    
    pspl_ir_state.fragment.stage_count = 0;
        
    pspl_ir_state.blend.blending = PLATFORM;
    pspl_ir_state.blend.source_factor = SRC_ALPHA;
    pspl_ir_state.blend.dest_factor = ONE_MINUS | SRC_ALPHA;
    
    return 0;
    
}

static void shutdown(const pspl_toolchain_context_t* driver_context) {
    int i;
    
    pspl_calc_chain_destroy(&pspl_ir_state.vertex.pos_chain);
    for (i=0 ; i<MAX_TEX_COORDS ; ++i)
        pspl_calc_chain_destroy(&pspl_ir_state.vertex.tc_array[i].tc_chain);
    
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

static inline int is_texcoord(const char* str) {return (*str == 't' ||
                                                        *str == 'T');}

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
    int i;
    
    if (pspl_ir_state.vertex.in_matrix_def) {
        if (strcasecmp(command_name, "ENDMATRIX"))
            pspl_error(-1, "Premature MATRIX end",
                       "a 3x4 matrix must be closed with `ENDMATRIX()`");
    }
    
    
    if (current_heading->heading_level == 1) {
        
        if (is_vert(current_heading->heading_trace->heading_name)) {
            
            pspl_calc_chain_t* cur_chain = NULL;
            
            if (is_position(current_heading->heading_name)) {
                cur_chain = &pspl_ir_state.vertex.pos_chain;
            } else if (is_texcoord(current_heading->heading_name)) {
                if (current_heading->heading_argc < 1)
                    pspl_error(-1, "Invalid TEXCOORD sub-heading usage",
                               "there needs to be *one* augument specifying texcoord generator name");
                strlcpy(pspl_ir_state.vertex.tc_array[pspl_ir_state.vertex.tc_count].name,
                        current_heading->heading_argv[0], IR_NAME_LEN);
                cur_chain = &pspl_ir_state.vertex.tc_array[pspl_ir_state.vertex.tc_count].tc_chain;
                ++pspl_ir_state.vertex.tc_count;
            } else
                pspl_error(-1, "Unrecognised VERTEX sub-heading",
                           "`%s` sub-heading not recognised by VERTEX heading", current_heading->heading_name);
            
            if (is_matrix(command_name)) {
                
                if (!command_argc) {
                    pspl_ir_state.vertex.in_matrix_def = 1;
                    pspl_ir_state.vertex.matrix_row_count = 0;
                } else
                    pspl_calc_chain_add_dynamic_transform(cur_chain, command_argv[0]);
                
            } else if (is_endmatrix(command_name)) {
                
                pspl_ir_state.vertex.in_matrix_def = 0;
                for (i=pspl_ir_state.vertex.matrix_row_count-1 ; i<3 ; ++i) {
                    pspl_ir_state.vertex.matrix[i][0] = 0;
                    pspl_ir_state.vertex.matrix[i][1] = 0;
                    pspl_ir_state.vertex.matrix[i][2] = 0;
                    pspl_ir_state.vertex.matrix[i][3] = 0;
                }
                pspl_calc_chain_add_static_transform(cur_chain, pspl_ir_state.vertex.matrix);
                
            } else if (is_translation(command_name)) {
                
                if (command_argc == 1) {
                    pspl_calc_chain_add_dynamic_translation(cur_chain, command_argv[0]);
                } else if (command_argc == 3) {
                    pspl_vector3_t trans_vec = {
                        strtof(command_argv[0], NULL),
                        strtof(command_argv[1], NULL),
                        strtof(command_argv[2], NULL)
                    };
                    pspl_calc_chain_add_static_translation(cur_chain, trans_vec);
                } else
                    pspl_error(-1, "Invalid TRANSLATE usage",
                               "either *one* (dynamic) or *three* (static) arguments accepted");
                
            } else if (is_rotation(command_name)) {
                
                if (command_argc == 1) {
                    pspl_calc_chain_add_dynamic_rotation(cur_chain, command_argv[0]);
                } else if (command_argc == 4) {
                    pspl_rotation_t rot = {
                        strtof(command_argv[0], NULL),
                        strtof(command_argv[1], NULL),
                        strtof(command_argv[2], NULL),
                        strtof(command_argv[3], NULL)
                    };
                    pspl_calc_chain_add_static_rotation(cur_chain, &rot);
                } else
                    pspl_error(-1, "Invalid ROTATE usage",
                               "either *one* (dynamic) or *four* (static) arguments accepted");
                
            } else if (is_scale(command_name)) {
                
                if (command_argc == 1) {
                    pspl_calc_chain_add_dynamic_scale(cur_chain, command_argv[0]);
                } else if (command_argc == 3) {
                    pspl_vector3_t scale_vec = {
                        strtof(command_argv[0], NULL),
                        strtof(command_argv[1], NULL),
                        strtof(command_argv[2], NULL)
                    };
                    pspl_calc_chain_add_static_scale(cur_chain, scale_vec);
                } else
                    pspl_error(-1, "Invalid SCALE usage",
                               "either *one* (dynamic) or *three* (static) arguments accepted");
                
            } else
                pspl_error(-1, "Unrecognised VERTEX command",
                           "`%s` command not recognised by VERTEX heading", command_name);
            
        }
        
    } else if (current_heading->heading_level == 0) {
        
        if (is_vert(current_heading->heading_name)) {
            
            pspl_error(-1, "Invalid VERTEX heading usage",
                       "under VERTEX heading, an applied usage sub-heading must be specified (POSITION, TEXCOORD)");
            
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
                               "`SAMPLE` must specify 2 arguments in `SAMPLE(<MAPIDX> <UV>)` format");
                
                
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
                int map = (int)strtol(command_argv[0], NULL, 10);
                if (map < 0)
                    pspl_error(-1, "Invalid SAMPLE value", "map index must be positive");
                pspl_ir_state.fragment.stage_array[stage_idx].stage_texmap.texmap_idx = map;
                strlcpy(pspl_ir_state.fragment.stage_array[stage_idx].stage_texmap.name, command_argv[1], 256);
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
    int i,j;
    
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
    
    // Spill names into indices
    unsigned cur_idx = 0;
    for (i=0 ; i<pspl_ir_state.fragment.stage_count ; ++i) {
        pspl_ir_fragment_stage_t* stage = &pspl_ir_state.fragment.stage_array[i];
        if (stage->using_texture) {
            uint8_t found = 0;
            for (j=0 ; j<pspl_ir_state.vertex.tc_count ; ++j) {
                if (!strcasecmp(stage->stage_texmap.name, pspl_ir_state.vertex.tc_array[j].name)) {
                    if (pspl_ir_state.vertex.tc_array[j].resolved_name_idx < 0) {
                        stage->stage_texmap.resolved_name_idx = cur_idx;
                        pspl_ir_state.vertex.tc_array[j].resolved_name_idx = cur_idx;
                        ++cur_idx;
                    } else
                        stage->stage_texmap.resolved_name_idx = pspl_ir_state.vertex.tc_array[j].resolved_name_idx;
                    found = 1;
                    break;
                }
            }
            if (!found)
                pspl_error(-1, "Texcoord name binding error",
                           "unable to find paired UV generator '%s' in vertex stage",
                           stage->stage_texmap.name);
        }
    }
    
    // Write chains into PSPLC
    pspl_calc_chain_write_position(&pspl_ir_state.vertex.pos_chain);
    for (i=0 ; i<pspl_ir_state.vertex.tc_count ; ++i)
        pspl_calc_chain_write_uv(&pspl_ir_state.vertex.tc_array[i].tc_chain, i);
    
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
