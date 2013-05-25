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
#include <errno.h>

#include <PSPL/PSPLExtension.h>
#include <PSPLInternal.h>

#include "Preprocessor.h"
#include "Buffer.h"


#define PSPL_MAX_PREPROCESSOR_TOKENS 64


#pragma mark Internal Preprocessor State

/* Internal preprocessor state to convey invocation-by-invocation
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
        ++preprocessor_state.source->expansion_line_nodes[driver_state.line_num].expanded_line_count;
        
    } while ((line_str = strtok_r(save_ptr, "\n", &save_ptr)));
    
}


/* Emit PSPL line (may be called repeatedly, in desired order)
 * Newlines ('\n') are automatically tokenised into multiple `add_line` calls */
void pspl_preprocessor_add_line(const char* line_text, ...) {
    if (driver_state.pspl_phase != PSPL_PHASE_PREPROCESS_EXTENSION)
        return;
    
    // Expand format string
    char exp_line_text[2048];
    va_list va;
    va_start(va, line_text);
    vsprintf(exp_line_text, line_text, va);
    va_end(va);
    
    // Common line add
    _add_line(preprocessor_state.indent_level, exp_line_text);
}

/* Convenience function to add line with specified indent level (0 is primary) */
void pspl_preprocessor_add_indent_line(unsigned int indent_level, const char* line_text, ...) {
    if (driver_state.pspl_phase != PSPL_PHASE_PREPROCESS_EXTENSION)
        return;
    
    // Expand format string
    char exp_line_text[2048];
    va_list va;
    va_start(va, line_text);
    vsprintf(exp_line_text, line_text, va);
    va_end(va);
    
    // Common line add
    _add_line(preprocessor_state.indent_level+indent_level, exp_line_text);
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
    ++preprocessor_state.source->expansion_line_nodes[driver_state.line_num].expanded_line_count;
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
    ++preprocessor_state.source->expansion_line_nodes[driver_state.line_num].expanded_line_count;
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
    uint8_t put_space = 0;
    while ((arg = va_arg(va, const char*))) {
        if (put_space) {
            pspl_buffer_addchar(&preprocessor_state.out_buf, ' ');
            put_space = 0;
        }
        pspl_buffer_addstr(&preprocessor_state.out_buf, arg);
        put_space = 1;
    }
    pspl_buffer_addstr(&preprocessor_state.out_buf, ")\n");
    va_end(va);
    
    // Add a line to expansion count
    ++preprocessor_state.source->expansion_line_nodes[driver_state.line_num].expanded_line_count;
}


#pragma mark Private Core API Implementation

/* Perform PSPL source inclusion */
static void pspl_include(const char* path,
                         pspl_toolchain_context_t* ext_driver_ctx,
                         pspl_toolchain_driver_opts_t* driver_opts) {
    pspl_toolchain_driver_source_t* save_source = preprocessor_state.source;
    unsigned int line_num_save = driver_state.line_num;
    
    // New source in heap for included PSPL
    pspl_toolchain_driver_source_t* include_source = malloc(sizeof(pspl_toolchain_driver_source_t));
    include_source->parent_source = save_source;
    save_source->expansion_line_nodes[line_num_save].included_source = include_source;
    
    // Populate filename members
    include_source->file_path = path;
    
    // Make path absolute (if it's not already)
    const char* abs_path = path;
    if (abs_path[0] != '/') {
        abs_path = malloc(MAXPATHLEN);
        sprintf((char*)abs_path, "%s%s", save_source->file_enclosing_dir, path);
    }
    
    // Enclosing dir
    char* last_slash = strrchr(abs_path, '/');
    include_source->file_enclosing_dir = malloc(MAXPATHLEN);
    sprintf((char*)include_source->file_enclosing_dir, "%.*s", (int)(last_slash-abs_path+1), abs_path);
    
    // File name
    include_source->file_name = last_slash+1;
    
    // Load original source
    FILE* source_file = fopen(abs_path, "r");
    if (!source_file)
        pspl_error(-1, "Unable to open PSPL source",
                   "`%s` is unavailable for reading; errno %d (%s)",
                   abs_path, errno, strerror(errno));
    fseek(source_file, 0, SEEK_END);
    long source_len = ftell(source_file);
    fseek(source_file, 0, SEEK_SET);
    if (source_len > PSPL_MAX_SOURCE_SIZE)
        pspl_error(-1, "PSPL Source file exceeded filesize limit",
                   "source file `%s` is %l bytes in length; exceeding %u byte limit",
                   abs_path, source_len, (unsigned)PSPL_MAX_SOURCE_SIZE);
    char* source_buf = malloc(source_len+1);
    if (!source_buf)
        pspl_error(-1, "Unable to allocate memory buffer for PSPL source",
                   "errno %d - `%s`", errno, strerror(errno));
    size_t read_len = fread(source_buf, 1, source_len, source_file);
    source_buf[source_len] = '\0';
    include_source->original_source = source_buf;
    fclose(source_file);
    if (read_len != source_len)
        pspl_error(-1, "Didn't read expected amount from PSPL source",
                   "expected %u bytes; read %u bytes", source_len, read_len);
    
    // Run preprocessor recursively
    pspl_run_preprocessor(include_source, ext_driver_ctx, driver_opts, 0);
    
    if (abs_path != path)
        free((void*)abs_path);
    free(source_buf);
    free((void*)include_source->file_enclosing_dir);
    
    // Restore
    preprocessor_state.source = save_source;
    driver_state.line_num = line_num_save;
}

/* Get preprocessor hook from directive name
 * ensure weak convention is followed */
static const pspl_extension_t* get_pp_hook_ext(const char* name) {
    
    // Track candidate weak and strong hooks
    const pspl_extension_t* weak_ext = NULL;
    const pspl_extension_t* strong_ext = NULL;
    
    // Enumerate all extensions for hook
    const pspl_extension_t* ext;
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
void pspl_run_preprocessor(pspl_toolchain_driver_source_t* source,
                           pspl_toolchain_context_t* ext_driver_ctx,
                           pspl_toolchain_driver_opts_t* driver_opts,
                           uint8_t root) {
    
    // Set error-handling phase
    driver_state.pspl_phase = PSPL_PHASE_PREPROCESS;
    
    // Propogate source reference
    preprocessor_state.source = source;
    
    // Allocate space for preprocessed expansion (and maintain reallocation state)
    if (root)
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
    preprocessor_state.source->expansion_line_nodes = calloc(line_count, sizeof(struct _pspl_expansion_line_node));
    
    // Examine each line for preprocessor invocation ('[')
    driver_state.line_num = 0;
    const char* cur_line = preprocessor_state.source->original_source;
    const char* cur_chr = NULL;
    
    // Preprocessor invocation state
    uint8_t in_pp = 0;
    char* tok_read_in;
    char* tok_read_ptr;
    char* tok_arr[PSPL_MAX_PREPROCESSOR_TOKENS];
    unsigned int tok_c;
    uint8_t just_read_tok;
    uint8_t in_quote;
    uint8_t in_comment;
    
    // Preprocessor start and end pointers
    const char* pp_start;
    const char* pp_end;
    
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
        preprocessor_state.indent_level = added_spaces/PSPL_INDENT_SPACES + added_tabs;
        
        // Start expansion line count at 0
        preprocessor_state.source->expansion_line_nodes[driver_state.line_num].expanded_line_count = 0;
        preprocessor_state.source->expansion_line_nodes[driver_state.line_num].included_source = NULL;
        
        if (in_pp || *cur_line == '[') { // Invoke preprocessor for this line
            
            if (!in_pp) { // We haven't started preprocessor invocation; start now
                in_pp = 1;
                pp_start = cur_line+1;
                
                // Find preprocessor invocation end (']') (ensure it exists)
                cur_chr = pp_start;
                in_quote = 0;
                in_comment = 0;
                uint8_t unclosed_dir = 0;
                for (;;) {
                    if (in_comment) {
                        if (*cur_chr == '\0')
                            unclosed_dir = 1;
                        else if (in_comment == 1 && *cur_chr == '\n')
                            in_comment = 0;
                        else if (in_comment == 2 && (*cur_chr == '*' && *(cur_chr+1) == '/')) {
                            ++cur_chr;
                            in_comment = 0;
                        }
                    } else if (!in_quote && (*cur_chr == '/' && (*(cur_chr+1) == '*' ||
                                                                 *(cur_chr+1) == '/'))) {
                        ++cur_chr;
                        if (*cur_chr == '/')
                            in_comment = 1;
                        else if (*cur_chr == '*')
                            in_comment = 2;
                    } else if (*cur_chr == '"') {
                        in_quote ^= 1;
                    } else if (!in_quote && *cur_chr == ']' && *(cur_chr-1) != '\\') {
                        pp_end = cur_chr;
                        break;
                    } else if ((!in_quote && *cur_chr == '[' && *(cur_chr-1) != '\\') || *cur_chr == '\0')
                        unclosed_dir = 1;
                    if (unclosed_dir)
                        pspl_error(-2, "Unclosed preprocessor directive",
                                   "please add closing ']'");
                    ++cur_chr;
                }
                
                // Prepare PP state
                tok_read_in = malloc(pp_end-pp_start);
                tok_read_ptr = tok_read_in;
                tok_arr[0] = tok_read_ptr;
                tok_c = 0;
                just_read_tok = 0;
                in_quote = 0;
                in_comment = 0;
                cur_chr = pp_start;
                
            }
            
            
            // Read in invocation up to end of PP directive;
            // saving token pointers using a buffer-array
            for (; cur_chr<pp_end ; ++cur_chr) {
                
                // If we're in a comment, handle accordingly
                if (in_comment) {
                    if (*cur_chr == '\n') {
                        ++cur_chr;
                        if (in_comment == 1)
                            in_comment = 0;
                        break;
                    }
                    if (in_comment == 2 && *cur_chr == '*' && *(cur_chr+1) == '/') {
                        ++cur_chr;
                        in_comment = 0;
                    }
                    continue;
                }
                
                // Check for escapable character
                if (*cur_chr == '\\') {
                    ++cur_chr;
                    if (*cur_chr == 'n') {
                        *tok_read_ptr = '\n';
                        ++tok_read_ptr;
                        continue;
                    } else if (*cur_chr == 't') {
                        *tok_read_ptr = '\t';
                        ++tok_read_ptr;
                        continue;
                    } else if (cur_chr>=pp_end)
                        break;
                } else {
                    
                    // Check for quoted token start or end (toggle)
                    if (*cur_chr == '"') {
                        in_quote ^= 1;
                        continue;
                    }
                    
                    // Ignore whitespace (or treat as token delimiter)
                    if (!in_quote && (*cur_chr == ' ' || *cur_chr == '\t' || *cur_chr == '\n' ||
                                      (*cur_chr == '/' && (*(cur_chr+1) == '/' || *(cur_chr+1) == '*')))) {
                        if (just_read_tok) {
                            just_read_tok = 0;
                            *tok_read_ptr = '\0';
                            ++tok_read_ptr;
                            tok_arr[tok_c] = tok_read_ptr;
                            if (tok_c >= PSPL_MAX_PREPROCESSOR_TOKENS)
                                pspl_error(-2, "Maximum preprocessor tokens exceeded",
                                           "Up to %u tokens supported", PSPL_MAX_PREPROCESSOR_TOKENS);
                        }
                        if (*cur_chr == '/') { // Handle comment start
                            ++cur_chr;
                            if (*cur_chr == '/')
                                in_comment = 1;
                            else if (*cur_chr == '*')
                                in_comment = 2;
                        } else if (*cur_chr == '\n') { // Handle line break
                            ++cur_chr;
                            break;
                        }
                        continue;
                    }
                    
                }
                
                // Copy character into read ptr otherwise
                if (!just_read_tok) {
                    just_read_tok = 1;
                    ++tok_c;
                }
                *tok_read_ptr = *cur_chr;
                ++tok_read_ptr;
                
            }
            
            // Do next line if there is more of the directive
            if (cur_chr < pp_end)
                continue;
            
            // Directive has finished by this point
            in_pp = 0;
            
            // Ensure the directive was not empty
            if (tok_c) {
                
                // See if this is an INCLUDE directive
                if (!strcasecmp("INCLUDE", tok_arr[0])) {
                    
                    if (tok_c < 2)
                        pspl_error(-1, "[INCLUDE] path must be specified",
                                   "too few arguments to perform include");
                    pspl_include(tok_arr[1], ext_driver_ctx, driver_opts);
                    
                } else {
                    
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
                
            }
            
            // Free token buffer array
            free(tok_read_in);
            
        } else { // Not in preprocessor invocation (perform copy for compiler; stripping comments)
            
            // Copy line into preprocessed buffer *without* expansion
            const char* end_of_line = strchr(cur_line, '\n');
            if (!end_of_line)
                end_of_line = cur_line + strlen(cur_line);
            
            // If we're in a comment already, find out if it ends
            const char* slash = NULL;
            if (in_comment == 2) {
                slash = strstr(cur_line, "*/");
                if (slash && slash<end_of_line) {
                    cur_line = slash + 2;
                    in_comment = 0;
                } else
                    continue; // This line is all comment
            }
            
            // Check for starting line comment as well
            slash = strchr(cur_line, '/');
            if (slash && slash<end_of_line && (*(slash+1) == '/' || *(slash+1) == '*')) {
                pspl_buffer_addstrn(&preprocessor_state.out_buf, cur_line, slash-cur_line);
                if ((*(slash+1) == '*')) {
                    slash += 2;
                    slash = strstr(slash, "*/");
                    if (slash && slash<end_of_line) {
                        slash += 2;
                        pspl_buffer_addstrn(&preprocessor_state.out_buf, slash, end_of_line-slash);
                    } else
                        in_comment = 2;
                }
            } else
                pspl_buffer_addstrn(&preprocessor_state.out_buf, cur_line, end_of_line-cur_line);
            
            pspl_buffer_addchar(&preprocessor_state.out_buf, '\n');
            preprocessor_state.source->expansion_line_nodes[driver_state.line_num].expanded_line_count = 1;
            
        }
        
    } while ((cur_line = strchr(cur_line, '\n')) && ++driver_state.line_num);
    
    
    // Load expanded buffer into source object
    preprocessor_state.source->preprocessed_source = preprocessor_state.out_buf.buf;
    
}


