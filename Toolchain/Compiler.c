//
//  Compiler.c
//  PSPL
//
//  Created by Jack Andersen on 4/28/13.
//
//

#define PSPL_INTERNAL
#define PSPL_TOOLCHAIN

#include <string.h>

#include <PSPL/PSPLExtension.h>
#include <PSPLInternal.h>

#include "Compiler.h"

/* Generate platform-native bullet enumerations */
const unsigned int PSPL_BULLET_MASK = (~(((~0)<<2)>>2));
const unsigned int PSPL_BULLET_STAR = (((~0)>>1)&PSPL_BULLET_MASK);
const unsigned int PSPL_BULLET_DASH = (((~0)>>1)&PSPL_BULLET_MASK);


#define PSPL_MAX_COMMAND_ARGS 64


#pragma mark Internal Compiler State

/* Internal compiler state to convey information between
 * extensions and core PSPL */
static struct _pspl_compiler_state {
    
    // Current source being preprocessed
    pspl_toolchain_driver_source_t* source;
    
    // Current indent level
    unsigned int indent_level;
    
    // Current heading context info
    pspl_toolchain_heading_context_t heading_context;
    
    // Resolved extension for heading (NULL if 'GLOBAL' heading)
    pspl_extension_t* heading_extension;
    
} compiler_state;


#pragma mark Public Extension API Implementation

/* Request the immediate initialisation of another extension
 * (only valid within init hook) */
int pspl_toolchain_init_other_extension(pspl_extension_t* extension) {
    
}

/* Add referenced source file to Reference Gathering list */
void pspl_gather_referenced_file(const char* file_path) {
    
}

/* Add data object (keyed with a null-terminated string stored as 32-bit truncated SHA1 hash) */
int __pspl_psplc_embed_hash_keyed_object(pspl_runtime_platform_t* platforms,
                                         const char* key,
                                         const void* little_object,
                                         const void* big_object,
                                         size_t object_size) {
    
}

/* Add data object (keyed with a non-hashed 32-bit unsigned numeric value)
 * Integer keying uses a separate namespace from hashed keying */
int __pspl_psplc_embed_integer_keyed_object(pspl_runtime_platform_t* platforms,
                                            uint32_t key,
                                            const void* little_object,
                                            const void* big_object,
                                            size_t object_size) {
    
}

/* Add file for PSPL-packaging */
int pspl_package_add_file(pspl_runtime_platform_t* platforms,
                          const char* path,
                          int move,
                          uint32_t* hash_out) {
    
    
}


#pragma mark Private Core API Implementation

/* Main entry point for compiling a single PSPL source */
void _pspl_run_compiler(pspl_toolchain_driver_source_t* source,
                        pspl_toolchain_context_t* ext_driver_ctx,
                        pspl_toolchain_driver_opts_t* driver_opts) {
    
    // Set error-handling phase
    driver_state.pspl_phase = PSPL_PHASE_COMPILE;
    
    // Propogate source reference
    compiler_state.source = source;
    
    
    // Determine purpose of each line and handle appropriately
    driver_state.line_num = 0;
    const char* cur_line = compiler_state.source->original_source;
    const char* cur_chr = NULL;
    
    // Command invocation state
    uint8_t in_com = 0;
    char* tok_read_in;
    char* tok_read_ptr;
    char* tok_arr[PSPL_MAX_COMMAND_ARGS];
    unsigned int tok_c;
    uint8_t just_read_tok;
    uint8_t in_quote;
    uint8_t in_comment;
    
    // Command start and end pointers
    const char* com_start;
    const char* com_end;
    
    do { // Per line
        if (*cur_line == '\n')
            ++cur_line;
        
        // Chomp leading whitespace (and track indent level)
        unsigned int added_spaces = 0;
        unsigned int added_tabs = 0;
        while (*cur_line == ' ' || *cur_line == '\t') {
            if (*cur_line == ' ')
                ++added_spaces;
            else if(*cur_line == '\t')
                ++added_tabs;
            ++cur_line;
        }
        compiler_state.indent_level = added_spaces/PSPL_INDENT_SPACES + added_tabs;
        
        // TODO: DO
        
    } while ((cur_line = strchr(cur_line, '\n')) && ++driver_state.line_num);
    
    
}


