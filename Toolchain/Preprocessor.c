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

#define PSPL_MAX_PREPROCESSOR_TOKENS 64


#pragma mark Public Extension API Implementation



#pragma mark Private Implementation

/* Get preprocessor hook from directive name
 * ensure weak convention is followed */
static pspl_toolchain_line_preprocessor_hook get_pp_hook(const char* name) {
    // TODO: DO
}

/* Main entry point for preprocessing a single PSPL source */
void _pspl_run_preprocessor(pspl_toolchain_driver_source_t* source,
                            pspl_toolchain_context_t* ext_driver_ctx,
                            pspl_toolchain_driver_opts_t* driver_opts) {
    
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
        unsigned int indent_level = added_spaces/4 + added_tabs;
        
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
            
            // Read in invocation up to end; saving token pointers
            char* tok_read_in = malloc(pp_end-pp_start);
            char* tok_read_ptr = tok_read_in;
            char* tok_arr[PSPL_MAX_PREPROCESSOR_TOKENS];
            tok_arr[0] = tok_read_ptr;
            unsigned int tok_c = 0;
            uint8_t just_read_tok = 0;
            for (cur_chr=pp_start;cur_chr<pp_end;++cur_chr) {
                // Ignore whitespace (or treat as token delimit)
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
                    
                    // Call out to preprocessor hook
                    hook(ext_driver_ctx, tok_c, (const char**)tok_arr);
                    
                } else
                    pspl_warn("Unrecognised preprocessor directive",
                              "directive `%s` not handled by any installed extensions; skipping",
                              tok_arr[0]);
                
            }
            
            
        } else {
            
        }
        
        ++line_idx;
    } while ((current_line = strchr(current_line, '\n')));
    
    
}

