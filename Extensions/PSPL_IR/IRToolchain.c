//
//  IRToolchain.c
//  PSPL
//
//  Created by Jack Andersen on 6/9/13.
//
//

#include <stdio.h>
#include <PSPLExtension.h>
#include "CalcChain.h"

#define MAX_TEX_COORDS 10
#define MAX_FRAG_STAGES 16

static void copyright_hook() {
    
    pspl_toolchain_provide_copyright("PSPL-IR (Platform-independent shader representation)",
                                     "Copyright (c) 2013 Jack Andersen <jackoalan@gmail.com>",
                                     "[See licence for \"PSPL\" above]");
    
}

/* Feature enable state enum */
enum pspl_feature_enable {
    PLATFORM = 0,
    DISABLED = 1,
    ENABLED  = 2
};

/* Blend factor enum */
enum pspl_blend_factor {
    SRC_COLOUR = 0,
    DST_COLOUR = 1,
    SRC_ALPHA  = 2,
    DST_ALPHA  = 3,
    ONE_MINUS  = 4
};


/* IR translation state */
static struct {
    
    // Vertex state
    struct {
        
        // Are we populating a matrix?
        int in_matrix_def;
        pspl_matrix34_t matrix;
        
        // Position transform chain
        pspl_calc_chain_t pos_chain;
        
        // Normal transform chain
        pspl_calc_chain_t norm_chain;
        
        // Texcoord chains
        int tc_count;
        pspl_calc_chain_t tc_chain[MAX_TEX_COORDS];
        
    } vertex;
    
    // Depth state
    struct {
        
        enum pspl_feature_enable test;
        enum pspl_feature_enable write;
        
    } depth;
    
    // Fragment state
    struct {
        
        // Are we populating a matrix?
        int in_matrix_def;
        pspl_matrix34_t matrix;
        
        // Stage Array
        
        
    } fragment;
    
    // Blend State
    struct {
        
        // Blending enabled?
        enum pspl_feature_enable blending;
        
        // Blend factors
        enum pspl_blend_factor source_factor;
        enum pspl_blend_factor dest_factor;
        
    } blend;
    
} ir_state;


static int init(const pspl_toolchain_context_t* driver_context) {
    
    ir_state.vertex.in_matrix_def = 0;
    pspl_calc_chain_init(&ir_state.vertex.pos_chain);
    pspl_calc_chain_init(&ir_state.vertex.norm_chain);
    int i;
    for (i=0 ; i<MAX_TEX_COORDS ; ++i)
        pspl_calc_chain_init(&ir_state.vertex.tc_chain[i]);
    ir_state.vertex.tc_count = 0;
    
    ir_state.depth.test = PLATFORM;
    ir_state.depth.write = PLATFORM;
    
    ir_state.fragment.in_matrix_def = 0;
    
    ir_state.blend.blending = PLATFORM;
    ir_state.blend.source_factor = SRC_ALPHA;
    ir_state.blend.dest_factor = ONE_MINUS | SRC_ALPHA;
    
    return 0;
    
}

static void shutdown(const pspl_toolchain_context_t* driver_context) {
    
    
    
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
            
        } else if (!strcasecmp(current_heading->heading_name, "BLEND")) {
            
            if (command_argc >= 2) {
                
                if (is_source(command_name)) {
                    
                    ir_state.blend.blending = 1;
                    ir_state.blend.source_factor = 0;
                    
                    if (command_argc >= 3 && is_inverse(command_argv[0])) {
                        ir_state.blend.source_factor = ONE_MINUS;
                        ++command_argv;
                    }
                    
                    if (is_source(command_argv[0])) {
                        
                        if (is_colour(command_argv[1])) {
                            ir_state.blend.source_factor |= SRC_COLOUR;
                            return;
                        } else if (is_alpha(command_argv[1])) {
                            ir_state.blend.source_factor |= SRC_ALPHA;
                            return;
                        }
                        
                    } else if (is_dest(command_argv[0])) {
                        
                        if (is_colour(command_argv[1])) {
                            ir_state.blend.source_factor |= DST_COLOUR;
                            return;
                        } else if (is_alpha(command_argv[1])) {
                            ir_state.blend.source_factor |= DST_ALPHA;
                            return;
                        }
                        
                    }
                    
                    
                } else if (is_dest(command_name)) {
                    
                    ir_state.blend.blending = 1;
                    ir_state.blend.dest_factor = 0;
                    
                    if (command_argc >= 3 && is_inverse(command_argv[0])) {
                        ir_state.blend.dest_factor = ONE_MINUS;
                        ++command_argv;
                    }
                    
                    if (is_source(command_argv[0])) {
                        
                        if (is_colour(command_argv[1])) {
                            ir_state.blend.dest_factor |= SRC_COLOUR;
                            return;
                        } else if (is_alpha(command_argv[1])) {
                            ir_state.blend.dest_factor |= SRC_ALPHA;
                            return;
                        }
                        
                    } else if (is_dest(command_argv[0])) {
                        
                        if (is_colour(command_argv[1])) {
                            ir_state.blend.dest_factor |= DST_COLOUR;
                            return;
                        } else if (is_alpha(command_argv[1])) {
                            ir_state.blend.dest_factor |= DST_ALPHA;
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
