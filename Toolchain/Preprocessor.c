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

#include <PSPL/PSPLExtension.h>
#include <PSPLInternal.h>

#include "Preprocessor.h"
#include "Buffer.h"


#define PSPL_MAX_PREPROCESSOR_TOKENS 64


#pragma mark Public Extension API Implementation



#pragma mark Private Core API Implementation

/* Get preprocessor hook from directive name
 * ensure weak convention is followed */
static pspl_toolchain_line_preprocessor_hook get_pp_hook(const char* name) {
    
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
        return strong_ext->toolchain_extension->line_preprocessor_hook;
    if (weak_ext)
        return weak_ext->toolchain_extension->line_preprocessor_hook;
    
    // No matching directive found
    return NULL;
    
}

/* Main entry point for preprocessing a single PSPL source */
void _pspl_run_preprocessor(pspl_toolchain_driver_source_t* source,
                            pspl_toolchain_context_t* ext_driver_ctx,
                            pspl_toolchain_driver_opts_t* driver_opts) {
    
    // Allocate space for preprocessed expansion (and maintain reallocation state)
    pspl_buffer_t out_buf;
    pspl_buffer_init(&out_buf, strlen(source->original_source)*2+1);
    
    
    // Start by counting lines
    unsigned int line_count = 1;
    const char* nl_ptr = source->original_source;
    while ((nl_ptr = strchr(nl_ptr, '\n'))) {
        ++line_count;
        ++nl_ptr;
    }
    
    // Allocate expansion line count array
    source->expansion_line_counts = calloc(line_count, sizeof(unsigned int));
    
    // Examine each line for preprocessor invocation ('[')
    unsigned int line_idx = 0;
    const char* current_line = source->original_source;
    
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
        unsigned int indent_level = added_spaces/PSPL_INDENT_SPACES + added_tabs;
        
        if (*current_line == '[') { // Invoke preprocessor
            
            // Start expansion line count at 0
            source->expansion_line_counts[line_idx] = 0;
            
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
            for (cur_chr=pp_start;cur_chr<pp_end;++cur_chr) {
                
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
                
                // Now determine which preprocessor hook needs to be dispatched
                pspl_toolchain_line_preprocessor_hook hook = get_pp_hook(tok_arr[0]);
                
                if (hook) {
                    
                    // Set callout context accordingly
                    // TODO: DO
                    
                    // Callout to preprocessor hook
                    hook(ext_driver_ctx, tok_c, (const char**)tok_arr);
                    
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
            pspl_buffer_addstrn(&out_buf, current_line, line_len);
            
        }
        
        ++line_idx;
    } while ((current_line = strchr(current_line, '\n')));
    
    
}


