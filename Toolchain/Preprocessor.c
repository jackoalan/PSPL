//
//  Preprocessor.c
//  PSPL
//
//  Created by Jack Andersen on 4/28/13.
//
//

#define PSPL_INTERNAL
#define PSPL_TOOLCHAIN

#include <string.h>
#include <stdarg.h>
#include <stdio.h>

#include <PSPL/PSPLExtension.h>
#include <PSPLInternal.h>

#include "Preprocessor.h"
#include "Buffer.h"


#define PSPL_MAX_PREPROCESSOR_TOKENS 64


#pragma mark Internal Preprocessor State

/* Internal preprocessor state to convey line-by-line
 * preprocessor-expansion info */
static struct _pspl_preprocessor_state {
    
    // Current source being preprocessed
    pspl_toolchain_driver_source_t* source;
    
    // Current indent level
    unsigned int indent_level;
    
    // Current output buffer being built up
    pspl_buffer_t out_buf;
    
} preprocessor_state;


#pragma mark Public Extension API Implementation

/* All of these functions are called within extension preprocessor hook */


/* Private, common `add_line` routine */
static void _add_line(unsigned int indent_level, char* exp_line_text) {
    
    // Tokensise into lines
    char* save_ptr;
    char* line_str = strtok_r(exp_line_text, "\n", &save_ptr);
    if (!line_str)
        line_str = exp_line_text;
    
    // Read in lines
    do {
        
        // Recreate indent level
        unsigned int i;
        for (i=0 ; i<indent_level ; ++i)
            pspl_buffer_addchar(&preprocessor_state.out_buf, '\t');
        
        // Deposit expanded string
        pspl_buffer_addstr(&preprocessor_state.out_buf, line_str);
        pspl_buffer_addchar(&preprocessor_state.out_buf, '\n');
        
        // Add a line to expansion count
        ++preprocessor_state.source->expansion_line_counts[driver_state.line_num];
        
    } while ((line_str = strtok_r(save_ptr, "\n", &save_ptr)));
    
}


/* Emit PSPL line (may be called repeatedly, in desired order)
 * Newlines ('\n') are automatically tokenised into multiple `add_line` calls */
void pspl_preprocessor_add_line(const char* line_text, ...) {
    if (driver_state.pspl_phase != PSPL_PHASE_PREPROCESS_EXTENSION)
        return;
    
    // Expand format string
    char* exp_line_text;
    va_list va;
    va_start(va, line_text);
    vasprintf(&exp_line_text, line_text, va);
    va_end(va);
    
    // Common line add
    _add_line(preprocessor_state.indent_level, exp_line_text);
    
    // Done with format-expanded string
    free(exp_line_text);
}

/* Convenience function to add line with specified indent level (0 is primary) */
void pspl_preprocessor_add_indent_line(unsigned int indent_level, const char* line_text, ...) {
    if (driver_state.pspl_phase != PSPL_PHASE_PREPROCESS_EXTENSION)
        return;
    
    // Expand format string
    char* exp_line_text;
    va_list va;
    va_start(va, line_text);
    vasprintf(&exp_line_text, line_text, va);
    va_end(va);
    
    // Common line add
    _add_line(preprocessor_state.indent_level+indent_level, exp_line_text);
    
    // Done with format-expanded string
    free(exp_line_text);
}

/* Convenience function to emit PSPL primary-heading push
 * (emits 'PSPL_HEADING_PUSH(primary_heading_name)')
 * Saves heading state onto internal toolchain-driver stack */
void pspl_preprocessor_add_heading_push(const char* primary_heading_name) {
    if (driver_state.pspl_phase != PSPL_PHASE_PREPROCESS_EXTENSION)
        return;
    
    // Recreate indent level
    unsigned int i;
    for (i=0 ; i<preprocessor_state.indent_level ; ++i)
        pspl_buffer_addchar(&preprocessor_state.out_buf, '\t');
    
    // Deposit expanded string
    pspl_buffer_addstr(&preprocessor_state.out_buf, "PSPL_HEADING_PUSH(");
    pspl_buffer_addstr(&preprocessor_state.out_buf, primary_heading_name);
    pspl_buffer_addstr(&preprocessor_state.out_buf, ")\n");
    
    // Add a line to expansion count
    ++preprocessor_state.source->expansion_line_counts[driver_state.line_num];
}

/* Convenience function to emit PSPL primary-heading push
 * (emits 'PSPL_HEADING_POP()')
 * restores the heading state before previous PSPL_HEADING_PUSH() */
void pspl_preprocessor_add_heading_pop() {
    if (driver_state.pspl_phase != PSPL_PHASE_PREPROCESS_EXTENSION)
        return;
    
    // Recreate indent level
    unsigned int i;
    for (i=0 ; i<preprocessor_state.indent_level ; ++i)
        pspl_buffer_addchar(&preprocessor_state.out_buf, '\t');
    
    // Deposit expanded string
    pspl_buffer_addstr(&preprocessor_state.out_buf, "PSPL_HEADING_POP()\n");
    
    // Add a line to expansion count
    ++preprocessor_state.source->expansion_line_counts[driver_state.line_num];
}

/* Convenience function to emit command call with arguments.
 * All variadic arguments *must* be C-strings */
void _pspl_preprocessor_add_command_call(const char* command_name, ...) {
    if (driver_state.pspl_phase != PSPL_PHASE_PREPROCESS_EXTENSION)
        return;
    
    // Construct command call
    const char* arg;
    va_list va;
    va_start(va, command_name);
    pspl_buffer_addstr(&preprocessor_state.out_buf, command_name);
    pspl_buffer_addchar(&preprocessor_state.out_buf, '(');
    uint8_t put_space = 1;
    while ((arg = va_arg(va, const char*))) {
        if (put_space) {
            pspl_buffer_addchar(&preprocessor_state.out_buf, ' ');
            put_space = 0;
        }
        pspl_buffer_addstr(&preprocessor_state.out_buf, arg);
    }
    pspl_buffer_addstr(&preprocessor_state.out_buf, ")\n");
    va_end(va);
    
    // Add a line to expansion count
    ++preprocessor_state.source->expansion_line_counts[driver_state.line_num];
}


#pragma mark Private Core API Implementation

/* Get preprocessor hook from directive name
 * ensure weak convention is followed */
static const pspl_extension_t* get_pp_hook_ext(const char* name) {
    
    // Track candidate weak and strong hooks
    const pspl_extension_t* weak_ext = NULL;
    const pspl_extension_t* strong_ext = NULL;
    
    // Enumerate all extensions for hook
    pspl_extension_t* ext;
    unsigned int i = 0;
    while ((ext = pspl_available_extensions[i++])) {
        const pspl_toolchain_extension_t* tool_ext;
        if ((tool_ext = ext->toolchain_extension)) {
            
            const char* ext_directive;
            unsigned int j = 0;
            
            // Weak directive enumeration
            if (tool_ext->weak_claimed_global_preprocessor_directives)
                while ((ext_directive = tool_ext->weak_claimed_global_preprocessor_directives[j++]))
                    if (!strcasecmp(name, ext_directive) && tool_ext->line_preprocessor_hook) {
                        if (weak_ext)
                            pspl_warn("Multiple weakly-claimed preprocessor directive",
                                      "Directive named `%s` claimed by both `%s` extension and `%s` extension; latter will overload former",
                                       name, strong_ext->extension_name, ext->extension_name);
                        weak_ext = ext;
                    }
            
            // Strong directive enumeration
            j = 0;
            if (tool_ext->claimed_global_preprocessor_directives)
                while ((ext_directive = tool_ext->claimed_global_preprocessor_directives[j++]))
                    if (!strcasecmp(name, ext_directive) && tool_ext->line_preprocessor_hook) {
                        if (!strong_ext)
                            strong_ext = ext;
                        else
                            pspl_error(-1, "Multiple strongly-claimed preprocessor directive",
                                       "Directive named `%s` claimed by both `%s` extension and `%s` extension; this is disallowed by PSPL",
                                       name, strong_ext->extension_name, ext->extension_name);
                    }
            
        }
    }
    
    // Return strong if it exists; otherwise return weak
    if (strong_ext)
        return strong_ext;
    if (weak_ext)
        return weak_ext;
    
    // No matching directive found
    return NULL;
    
}

/* Main entry point for preprocessing a single PSPL source */
void _pspl_run_preprocessor(pspl_toolchain_driver_source_t* source,
                            pspl_toolchain_context_t* ext_driver_ctx,
                            pspl_toolchain_driver_opts_t* driver_opts) {
    
    // Set error-handling phase
    driver_state.pspl_phase = PSPL_PHASE_PREPROCESS;
    
    // Propogate source reference
    preprocessor_state.source = source;
    
    // Allocate space for preprocessed expansion (and maintain reallocation state)
    pspl_buffer_init(&preprocessor_state.out_buf,
                     strlen(preprocessor_state.source->original_source)*2+1);
    
    
    // Start by counting lines
    unsigned int line_count = 1;
    const char* nl_ptr = preprocessor_state.source->original_source;
    while ((nl_ptr = strchr(nl_ptr, '\n'))) {
        ++line_count;
        ++nl_ptr;
    }
    
    // Allocate expansion line count array
    preprocessor_state.source->expansion_line_counts = calloc(line_count, sizeof(unsigned int));
    
    // Examine each line for preprocessor invocation ('[')
    driver_state.line_num = 0;
    const char* current_line = preprocessor_state.source->original_source;
    
    do {
        // Chomp leading whitespace (and track indent level)
        unsigned int added_spaces = 0;
        unsigned int added_tabs = 0;
        while (*current_line == ' ' || *current_line == '\t') {
            if (*current_line == ' ')
                ++added_spaces;
            else if(*current_line == '\t')
                ++added_tabs;
            ++current_line;
        }
        preprocessor_state.indent_level = added_spaces/PSPL_INDENT_SPACES + added_tabs;
        
        if (*current_line == '[') { // Invoke preprocessor
            
            // Start expansion line count at 0
            preprocessor_state.source->expansion_line_counts[driver_state.line_num] = 0;
            
            // Start and end pointers
            const char* pp_start = &current_line[1];
            const char* pp_end;
            
            // Find preprocessor invocation end (']') (ensure it exists)
            const char* cur_chr = pp_start;
            for (;;) {
                if (*cur_chr == ']') {
                    pp_end = cur_chr;
                    break;
                } else if (*cur_chr == '\n' || *cur_chr == '\0')
                    pspl_error(-2, "Unclosed preprocessor directive",
                               "please add closing ']'");
                ++cur_chr;
            }
            
            // Read in invocation up to end; saving token pointers using a buffer-array
            char* tok_read_in = malloc(pp_end-pp_start);
            char* tok_read_ptr = tok_read_in;
            char* tok_arr[PSPL_MAX_PREPROCESSOR_TOKENS];
            tok_arr[0] = tok_read_ptr;
            unsigned int tok_c = 0;
            uint8_t just_read_tok = 0;
            for (cur_chr=pp_start ; cur_chr<pp_end ; ++cur_chr) {
                
                // Ignore whitespace (or treat as token delimiter)
                if (*cur_chr == ' ' || *cur_chr == '\t') {
                    if (just_read_tok) {
                        just_read_tok = 0;
                        *tok_read_ptr = '\0';
                        ++tok_read_ptr;
                        tok_arr[tok_c] = tok_read_ptr;
                        if (tok_c >= PSPL_MAX_PREPROCESSOR_TOKENS)
                            pspl_error(-2, "Maximum preprocessor tokens exceeded",
                                       "Up to %u tokens supported", PSPL_MAX_PREPROCESSOR_TOKENS);
                    }
                    continue;
                }
                
                // Copy character into read ptr otherwise
                if (!just_read_tok) {
                    just_read_tok = 1;
                    ++tok_c;
                }
                *tok_read_ptr = *cur_chr;
                ++tok_read_ptr;
                
            }
            
            // Ensure the directive was not empty
            if (tok_c) {
                
                // Now determine which extension's preprocessor hook needs to be dispatched
                const pspl_extension_t* hook_ext = get_pp_hook_ext(tok_arr[0]);
                
                if (hook_ext) {
                    
                    // Set callout context accordingly
                    driver_state.pspl_phase = PSPL_PHASE_PREPROCESS_EXTENSION;
                    driver_state.proc_extension = hook_ext;
                    
                    // Callout to preprocessor hook
                    hook_ext->toolchain_extension->
                    line_preprocessor_hook(ext_driver_ctx, tok_c, (const char**)tok_arr);
                    
                    // Unset callout context
                    driver_state.pspl_phase = PSPL_PHASE_PREPROCESS;
                    
                } else
                    pspl_warn("Unrecognised preprocessor directive",
                              "directive `%s` not handled by any installed extensions; skipping",
                              tok_arr[0]);
                
            }
            
            // Free token buffer array
            free(tok_read_in);
            
        } else {
            
            // Copy line into preprocessed buffer *without* expansion
            char* end_of_line = strchr(current_line, '\n');
            size_t line_len = (end_of_line)?(end_of_line-current_line+1):strlen(current_line);
            pspl_buffer_addstrn(&preprocessor_state.out_buf, current_line, line_len);
            preprocessor_state.source->expansion_line_counts[driver_state.line_num] = 1;
            
        }
        
        ++driver_state.line_num;
    } while ((current_line = strchr(current_line, '\n')));
    
    
    // Load expanded buffer into source object
    preprocessor_state.source->preprocessed_source = preprocessor_state.out_buf.buf;
    
}


