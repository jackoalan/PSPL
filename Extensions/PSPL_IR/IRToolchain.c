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
    
    pspl_ir_state.vertex.in_matrix_def = 0;
    pspl_calc_chain_init(&pspl_ir_state.vertex.pos_chain);
    pspl_ir_state.vertex.generate_normal = 0;
    int i;
    //for (i=0 ; i<MAX_TEX_COORDS ; ++i)
        //pspl_calc_chain_init(&pspl_ir_state.vertex.tc_chain[i]);
    pspl_ir_state.vertex.tc_count = 0;
    
    pspl_ir_state.depth.test = PLATFORM;
    pspl_ir_state.depth.write = PLATFORM;
    
    pspl_ir_state.fragment.stage_count = 0;
        
    pspl_ir_state.blend.blending = PLATFORM;
    pspl_ir_state.blend.source_factor = SRC_ALPHA;
    pspl_ir_state.blend.dest_factor = ONE_MINUS | SRC_ALPHA;
    
    return 0;
    
}

static void shutdown(const pspl_toolchain_context_t* driver_context) {
    
    pspl_calc_chain_destroy(&pspl_ir_state.vertex.pos_chain);
    int i;
    //for (i=0 ; i<MAX_TEX_COORDS ; ++i)
        //pspl_calc_chain_destroy(&pspl_ir_state.vertex.tc_chain[i]);
    
}


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


/* Handle incoming command call */

static void command_call(const pspl_toolchain_context_t* driver_context,
                         const pspl_toolchain_heading_context_t* current_heading,
                         const char* command_name,
                         unsigned int command_argc,
                         const char** command_argv) {
    
    if (current_heading->heading_level == 1) {
        
        if (!strcasecmp(current_heading->heading_name, "VERTEX")) {
            
            
            
        } else if (!strcasecmp(current_heading->heading_name, "DEPTH")) {
            
        } else if (!strcasecmp(current_heading->heading_name, "FRAGMENT")) {
            
            
            if (!strcasecmp(command_name, "SAMPLE")) {
                // Texture sample definition
                if (command_argc < 2)
                    pspl_error(-1, "Invalid command use",
                               "`SAMPLE` must specify 2 arguments in `SAMPLE(<MAPIDX> <UV>)` format");
                
                if (!pspl_ir_state.fragment.stage_count) {
                    // If first stage, insert initial SET stage
                    
                    
                } else {
                    // Otherwise determine stage accordingly
                    
                    int map;
                    
                    map = atoi(command_argv[0]);
                    if (map < 0)
                        pspl_error(-1, "Invalid SAMPLE value", "map index must be positive");
                    
                    
                    
                }
                
            } else if (!strcasecmp(command_name, "RGBA")) {
                // RGBA constant definition
                if (command_argc < 4)
                    pspl_error(-1, "Invalid command use",
                               "`RGBA` must specify 4 arguments in `RGBA(<RED> <GREEN> <BLUE> <ALPHA>)` format");
                
                unsigned stage_idx = 0;
                
                if (!pspl_ir_state.fragment.stage_count) {
                    // If first stage, insert initial SET stage
                    
                    stage_idx = 0;
                    pspl_ir_state.fragment.stage_array[0].stage_output = OUT_MAIN;
                    pspl_ir_state.fragment.stage_array[0].stage_op = OP_SET;
                    
                } else {
                    // Otherwise determine stage accordingly
                    
                    
                }
                
                pspl_ir_state.fragment.stage_array[stage_idx].sources[0] = IN_COLOUR;
                
                double r,g,b,a;
                
                r = atof(command_argv[0]);
                if (r < 0.0 || r > 1.0)
                    pspl_error(-1, "Invalid colour value", "Red value in `RGBA` must be a normalised [0,1] float value");
                g = atof(command_argv[1]);
                if (r < 0.0 || r > 1.0)
                    pspl_error(-1, "Invalid colour value", "Green value in `RGBA` must be a normalised [0,1] float value");
                b = atof(command_argv[2]);
                if (r < 0.0 || r > 1.0)
                    pspl_error(-1, "Invalid colour value", "Blue value in `RGBA` must be a normalised [0,1] float value");
                a = atof(command_argv[3]);
                if (r < 0.0 || r > 1.0)
                    pspl_error(-1, "Invalid colour value", "Alpha value in `RGBA` must be a normalised [0,1] float value");
                
                pspl_ir_state.fragment.stage_array[stage_idx].stage_colour.r = r;
                pspl_ir_state.fragment.stage_array[stage_idx].stage_colour.g = g;
                pspl_ir_state.fragment.stage_array[stage_idx].stage_colour.b = b;
                pspl_ir_state.fragment.stage_array[stage_idx].stage_colour.a = a;

                
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


static const char* claimed_headings[] = {
    "VERTEX",
    "DEPTH",
    "FRAGMENT",
    "BLEND",
    NULL};

pspl_toolchain_extension_t PSPL_IR_toolext = {
    .copyright_hook = copyright_hook,
    .init_hook = init,
    .command_call_hook = command_call,
    .claimed_heading_names = claimed_headings
};
