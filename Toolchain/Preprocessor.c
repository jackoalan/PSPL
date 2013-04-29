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


#pragma mark Public Extension API Implementation



#pragma mark Private Implementation

/* Main entry point for preprocessing a single PSPL source */
void _pspl_run_preprocessor(pspl_toolchain_driver_source_t* source,
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
            
            // Read in invocation up to end
            for (cur_chr=pp_start;cur_chr<pp_end;++cur_chr) {
                
            }
            
            // Tokenise first word
            
            
            
        } else {
            
        }
        
        ++line_idx;
    } while ((current_line = strchr(current_line, '\n')));
    
    
}

