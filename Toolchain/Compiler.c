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

/* Name of GLOBAL context */
static const char* PSPL_GLOBAL_NAME = "GLOBAL";


#define PSPL_HEADING_CTX_STACK_SIZE 8
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
    pspl_toolchain_heading_context_t* heading_context;
    
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
    
    // Heading context stack
    pspl_toolchain_heading_context_t heading_ctx_stack[PSPL_HEADING_CTX_STACK_SIZE];
    
    // Global heading context
    compiler_state.heading_extension = NULL;
    heading_ctx_stack[0].heading_name = PSPL_GLOBAL_NAME;
    heading_ctx_stack[0].heading_argc = 0;
    heading_ctx_stack[0].heading_level = 0;
    heading_ctx_stack[0].heading_trace = NULL;
    compiler_state.heading_context = &heading_ctx_stack[0];
    
    do { // Per line
        if (driver_state.line_num && *cur_line == '\n')
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
        
        
        // End of this line
        const char* end_of_line = strchr(cur_line, '\n');
        if (!end_of_line)
            end_of_line = strchr(cur_line, '\0');

        // Heading definition? and level?
        uint8_t is_heading = 0;
        unsigned int level = 0;
        
        // Determine if we are entering a new heading with '#'s
        if (*cur_line == '#') {
            ++cur_line;
            is_heading = 1;
            while (*cur_line == '#') {
                ++cur_line;
                ++level;
            }
            while (*cur_line == ' ' || *cur_line == '\t')
                ++cur_line;
        }
        
        // Determine if we are entering a new heading with '='s or '-'s
        else if (*end_of_line == '\n') {
            const char* next_line = end_of_line + 1;
            while (*next_line == ' ' || *next_line == '\t')
                ++next_line;
            if (*next_line == '=') {
                is_heading = 1;
                level = 0;
            } else if (*next_line == '-') {
                is_heading = 1;
                level = 1;
            }
        }
        
        if (is_heading) { // This is a heading spec
        
            // Validate heading level (and push one level if level greater)
            if (level > compiler_state.heading_context->heading_level)
                level = ++compiler_state.heading_context->heading_level;
            else { // Deallocate same-and-higher-level contexts and bring stack down
                unsigned int down_level = compiler_state.heading_context->heading_level;
                while (level <= down_level) {
                    pspl_toolchain_heading_context_t* de_ctx = &heading_ctx_stack[down_level];
                    if (de_ctx->heading_name != PSPL_GLOBAL_NAME)
                        free((void*)de_ctx->heading_name);
                    if (de_ctx->heading_argc)
                        free((void*)de_ctx->heading_argv[0]);
                    --down_level;
                }
                compiler_state.heading_context->heading_level = level;
            }
            if (level >= PSPL_HEADING_CTX_STACK_SIZE)
                pspl_error(-1, "Unsupported heading level",
                           "specified heading level would cause stack overflow; maximum level count: %u",
                           PSPL_HEADING_CTX_STACK_SIZE);
            
            // Target heading context
            pspl_toolchain_heading_context_t* target_ctx = &heading_ctx_stack[level];
            target_ctx->heading_argc = 0;
            target_ctx->heading_level = level;
            target_ctx->heading_trace = (level)?&heading_ctx_stack[level-1]:NULL;
            
            // Parse heading characters and (possibly) argument array
            const char* name_start = cur_line;
            const char* name_end = end_of_line;
            --cur_line;
            while (++cur_line < end_of_line) { // Scan for arguments (end name there)
                
                if (*cur_line == '(' && name_end == end_of_line) { // Check for arguments
                    
                    // Validate argument set
                    name_end = cur_line;
                    const char* end_args = strchr(cur_line, ')');
                    if (end_args >= end_of_line)
                        pspl_error(-1, "Unclosed heading arguments",
                                   "please add closing ')' on same line");
                    ++cur_line;
                    while (*cur_line == ' ' || *cur_line == '\t')
                        ++cur_line;
                    if (end_args > cur_line) {
                        
                        // Make argument buffer
                        char* arg_buf = malloc(end_args-cur_line+1);
                        strncpy(arg_buf, cur_line, end_args-cur_line);
                        arg_buf[end_args-cur_line] = '\0';
                        char* space = strchr(cur_line, ' ');
                        if (space < end_args) {
                            char* save_ptr;
                            char* token = strtok_r(arg_buf, " ", &save_ptr);
                            do {
                                if (*token == ' ')
                                    continue;
                                if (target_ctx->heading_argc >= PSPL_MAX_HEADING_ARGS)
                                    pspl_error(-1, "Maximum heading arguments exceeded",
                                               "up to %u heading arguments may be specified",
                                               PSPL_MAX_HEADING_ARGS);
                                target_ctx->heading_argv[target_ctx->heading_argc] = token;
                                ++target_ctx->heading_argc;
                            } while ((token = strtok_r(save_ptr, " ", &save_ptr)));
                        } else {
                            target_ctx->heading_argc = 1;
                            target_ctx->heading_argv[0] = arg_buf;
                        }
                        
                    }
                    break; // Done with heading
                    
                }
                
            }
            
            // Chomp whitespace off end of name
            while (name_end > name_start &&
                   (*name_end == ' ' || *name_end == '\n' || *name_end == '\0' || *name_end == '('))
                --name_end;
            if (name_end == name_start)
                pspl_error(-1, "No-name heading",
                           "headings must have a name in PSPL");
            
            // Add name to context
            char* name = malloc(name_end-name_start+1);
            strncpy(name, name_start, name_end-name_start);
            name[name_end-name_start] = '\0';
            target_ctx->heading_name = name;
            
        }
        
    } while ((cur_line = strchr(cur_line, '\n')) && ++driver_state.line_num);
    
    
}


