//
//  Compiler.c
//  PSPL
//
//  Created by Jack Andersen on 4/28/13.
//
//

#define PSPL_INTERNAL

#include <string.h>
#include <sys/param.h>

#include <PSPLExtension.h>
#include <PSPLInternal.h>
#include <PSPLHash.h>

#include "Compiler.h"
#include "ObjectIndexer.h"

/* Global IR staging state (from PSPL_IR extension) */
extern pspl_ir_state_t pspl_ir_state;

/* Generate platform-native bullet enumerations */
#ifdef _WIN32
const unsigned long PSPL_BULLET_MASK = 0xC0000000;
const unsigned long PSPL_BULLET_STAR = (((~0)>>1)&0xC0000000);
const unsigned long PSPL_BULLET_DASH = (((~0)>>1)&0xC0000000);
#else
const unsigned long PSPL_BULLET_MASK = (~(((~0)<<2)>>2));
const unsigned long PSPL_BULLET_STAR = (((~0)>>1)&PSPL_BULLET_MASK);
const unsigned long PSPL_BULLET_DASH = (((~0)>>1)&PSPL_BULLET_MASK);
#endif

/* Name of GLOBAL context */
static const char* PSPL_GLOBAL_NAME = "GLOBAL";


#define PSPL_HEADING_CTX_STACK_SIZE 8
#define PSPL_INDENT_CTX_STACK_SIZE 16
#define PSPL_MAX_COMMAND_ARGS 64


#pragma mark Internal Compiler State

/* Internal compiler state to convey information between
 * extensions and core PSPL */
static struct _pspl_compiler_state {
    
    // Current source being preprocessed
    pspl_toolchain_driver_source_t* source;
    
    // Current heading context info
    unsigned push_count;
    pspl_toolchain_heading_context_t* heading_context;
    
    // Current indent context info
    pspl_toolchain_indent_read_t* indent_context;
    
    // Resolved extension for heading (NULL if 'GLOBAL' heading)
    const pspl_extension_t* heading_extension;
    
} compiler_state;


#pragma mark Public Extension API Implementation

/* Add data object (keyed with a null-terminated string stored as 32-bit truncated SHA1 hash) */
void pspl_embed_hash_keyed_object(const pspl_platform_t** platforms,
                                  const char* key,
                                  const void* little_object,
                                  const void* big_object,
                                  size_t object_size) {
    if (driver_state.pspl_phase != PSPL_PHASE_INIT_EXTENSION &&
        driver_state.pspl_phase != PSPL_PHASE_COMPILE_EXTENSION &&
        driver_state.pspl_phase != PSPL_PHASE_PREPROCESS_EXTENSION &&
        driver_state.pspl_phase != PSPL_PHASE_FINISH_EXTENSION &&
        driver_state.pspl_phase != PSPL_PHASE_INSTRUCT_PLATFORM)
        return;
    if (!driver_state.indexer_ctx)
        return;
    pspl_indexer_hash_object_augment(driver_state.indexer_ctx, driver_state.proc_extension,
                                     platforms, key, little_object, big_object, object_size,
                                     driver_state.source);
}

/* Add data object (keyed with a non-hashed 32-bit unsigned numeric value)
 * Integer keying uses a separate namespace from hashed keying */
void pspl_embed_integer_keyed_object(const pspl_platform_t** platforms,
                                     uint32_t key,
                                     const void* little_object,
                                     const void* big_object,
                                     size_t object_size) {
    if (driver_state.pspl_phase != PSPL_PHASE_INIT_EXTENSION &&
        driver_state.pspl_phase != PSPL_PHASE_COMPILE_EXTENSION &&
        driver_state.pspl_phase != PSPL_PHASE_PREPROCESS_EXTENSION &&
        driver_state.pspl_phase != PSPL_PHASE_FINISH_EXTENSION &&
        driver_state.pspl_phase != PSPL_PHASE_INSTRUCT_PLATFORM)
        return;
    if (!driver_state.indexer_ctx)
        return;
    pspl_indexer_integer_object_augment(driver_state.indexer_ctx, driver_state.proc_extension,
                                        platforms, key, little_object, big_object, object_size,
                                        driver_state.source);
}

/* Same functions for platform data objects
 * These may only be called from *platform* codebases (not *extension* codebases) */

/* Add data object (keyed with a null-terminated string stored as 32-bit truncated SHA1 hash) */
void pspl_embed_platform_hash_keyed_object(const char* key,
                                           const void* little_object,
                                           const void* big_object,
                                           size_t object_size) {
    if (driver_state.pspl_phase != PSPL_PHASE_COMPILE_PLATFORM)
        return;
    if (!driver_state.indexer_ctx)
        return;
    pspl_indexer_platform_hash_object_augment(driver_state.indexer_ctx, driver_state.proc_platform,
                                              key, little_object, big_object, object_size,
                                              driver_state.source);
}

/* Add data object (keyed with a non-hashed 32-bit unsigned numeric value)
 * Integer keying uses a separate namespace from hashed keying */
void pspl_embed_platform_integer_keyed_object(uint32_t key,
                                              const void* little_object,
                                              const void* big_object,
                                              size_t object_size) {
    if (driver_state.pspl_phase != PSPL_PHASE_COMPILE_PLATFORM)
        return;
    if (!driver_state.indexer_ctx)
        return;
    pspl_indexer_platform_integer_object_augment(driver_state.indexer_ctx, driver_state.proc_platform,
                                                 key, little_object, big_object, object_size,
                                                 driver_state.source);
}

/* Add file for PSPL-packaging */
void pspl_package_file_augment(const pspl_platform_t** plats, const char* path_in,
                               const char* path_ext_in,
                               pspl_converter_file_hook converter_hook, uint8_t move_output,
                               void* user_ptr,
                               pspl_hash** hash_out) {
    if (driver_state.pspl_phase != PSPL_PHASE_INIT_EXTENSION &&
        driver_state.pspl_phase != PSPL_PHASE_COMPILE_EXTENSION &&
        driver_state.pspl_phase != PSPL_PHASE_PREPROCESS_EXTENSION &&
        driver_state.pspl_phase != PSPL_PHASE_FINISH_EXTENSION &&
        driver_state.pspl_phase != PSPL_PHASE_INSTRUCT_PLATFORM)
        return;
    if (!driver_state.indexer_ctx)
        return;
    pspl_indexer_stub_file_augment(driver_state.indexer_ctx, plats, path_in, path_ext_in,
                                   converter_hook, move_output, user_ptr, hash_out, driver_state.source);
}
void pspl_package_membuf_augment(const pspl_platform_t** plats, const char* path_in,
                                 const char* path_ext_in,
                                 pspl_converter_membuf_hook converter_hook,
                                 void* user_ptr,
                                 pspl_hash** hash_out) {
    if (driver_state.pspl_phase != PSPL_PHASE_INIT_EXTENSION &&
        driver_state.pspl_phase != PSPL_PHASE_COMPILE_EXTENSION &&
        driver_state.pspl_phase != PSPL_PHASE_PREPROCESS_EXTENSION &&
        driver_state.pspl_phase != PSPL_PHASE_FINISH_EXTENSION &&
        driver_state.pspl_phase != PSPL_PHASE_INSTRUCT_PLATFORM)
        return;
    if (!driver_state.indexer_ctx)
        return;
    pspl_indexer_stub_membuf_augment(driver_state.indexer_ctx, plats, path_in, path_ext_in,
                                     converter_hook, user_ptr, hash_out, driver_state.source);
}


#pragma mark Private Core API Implementation

/* Get primary heading owner from heading name
 * ensure weak convention is followed */
static const pspl_extension_t* get_heading_ext(const char* name) {
    
    const pspl_extension_t* ext;
    const pspl_toolchain_extension_t* tool_ext;
    
    // Track candidate weak and strong global command hooks
    const pspl_extension_t* weak_ext = NULL;
    const pspl_extension_t* strong_ext = NULL;
    
    // Enumerate all extensions for hook
    unsigned int i = 0;
    while ((ext = pspl_available_extensions[i++])) {
        if ((tool_ext = ext->toolchain_extension)) {
            
            const char* ext_heading;
            unsigned int j = 0;
            
            // Weak heading enumeration
            if (tool_ext->weak_claimed_heading_names)
                while ((ext_heading = tool_ext->weak_claimed_heading_names[j++]))
                    if (!strcasecmp(name, ext_heading)) {
                        if (weak_ext)
                            pspl_warn("Multiple weakly-claimed primary heading",
                                      "Primary heading named `%s` claimed by both `%s` extension and `%s` extension; latter will overload former",
                                      name, strong_ext->extension_name, ext->extension_name);
                        weak_ext = ext;
                    }
            
            // Strong heading enumeration
            j = 0;
            if (tool_ext->claimed_heading_names)
                while ((ext_heading = tool_ext->claimed_heading_names[j++]))
                    if (!strcasecmp(name, ext_heading)) {
                        if (!strong_ext)
                            strong_ext = ext;
                        else
                            pspl_error(-1, "Multiple strongly-claimed command",
                                       "Primary heading named `%s` claimed by both `%s` extension and `%s` extension; this is disallowed by PSPL",
                                       name, strong_ext->extension_name, ext->extension_name);
                    }
            
        }
    }
    
    // Return strong if it exists; otherwise return weak
    if (strong_ext)
        return strong_ext;
    if (weak_ext)
        return weak_ext;
    
    // No matching command found
    return NULL;
    
}

/* Get command hook from command name
 * ensure weak convention is followed */
static const pspl_extension_t* get_com_hook_ext(const char* name) {
    
    const pspl_extension_t* ext;
    const pspl_toolchain_extension_t* tool_ext;
    
    // Determine if active primary heading's extension claims this command
    // Early return if so
    if ((ext = compiler_state.heading_extension) &&
        (tool_ext = ext->toolchain_extension) &&
        tool_ext->command_call_hook) {
        return ext;
    }
    
    // Track candidate weak and strong global command hooks
    const pspl_extension_t* weak_ext = NULL;
    const pspl_extension_t* strong_ext = NULL;
    
    // Enumerate all extensions for hook
    unsigned int i = 0;
    while ((ext = pspl_available_extensions[i++])) {
        if ((tool_ext = ext->toolchain_extension)) {
            
            const char* ext_command;
            unsigned int j = 0;
            
            // Weak global command enumeration
            if (tool_ext->weak_claimed_global_command_names)
                while ((ext_command = tool_ext->weak_claimed_global_command_names[j++]))
                    if (!strcasecmp(name, ext_command) && tool_ext->command_call_hook) {
                        if (weak_ext)
                            pspl_warn("Multiple weakly-claimed command",
                                      "Command named `%s` claimed by both `%s` extension and `%s` extension; latter will overload former",
                                      name, strong_ext->extension_name, ext->extension_name);
                        weak_ext = ext;
                    }
            
            // Strong global command enumeration
            j = 0;
            if (tool_ext->claimed_global_command_names)
                while ((ext_command = tool_ext->claimed_global_command_names[j++]))
                    if (!strcasecmp(name, ext_command) && tool_ext->command_call_hook) {
                        if (!strong_ext)
                            strong_ext = ext;
                        else
                            pspl_error(-1, "Multiple strongly-claimed command",
                                       "Command named `%s` claimed by both `%s` extension and `%s` extension; this is disallowed by PSPL",
                                       name, strong_ext->extension_name, ext->extension_name);
                    }
            
        }
    }
    
    // Return strong if it exists; otherwise return weak
    if (strong_ext)
        return strong_ext;
    if (weak_ext)
        return weak_ext;
    
    // No matching command found
    return NULL;
    
}

/* Main entry point for compiling a single PSPL source */
void pspl_run_compiler(pspl_toolchain_driver_source_t* source,
                       pspl_toolchain_context_t* ext_driver_ctx,
                       pspl_toolchain_driver_opts_t* driver_opts) {
    
    // Initialise target platforms
    driver_state.pspl_phase = PSPL_PHASE_COMPILE_PLATFORM;
    int i;
    for (i=0 ; i<driver_opts->platform_c ; ++i) {
        const pspl_platform_t* plat = driver_opts->platform_a[i];
        if (plat && plat->toolchain_platform && plat->toolchain_platform->init_hook) {
            driver_state.proc_platform = plat;
            int err;
            if ((err = plat->toolchain_platform->init_hook(ext_driver_ctx)))
                pspl_error(-1, "Platform threw error on init",
                           "platform named '%s' gave error %d on init",
                           plat->platform_name, err);
        }
    }
    
    // Set error-handling phase
    driver_state.pspl_phase = PSPL_PHASE_COMPILE;
    
    // Propogate source reference
    compiler_state.source = source;
    
    
    // Determine purpose of each line and handle appropriately
    driver_state.line_num = 0;
    const char* cur_line = compiler_state.source->preprocessed_source;
    const char* cur_chr = NULL;
    
    // Command invocation state
    uint8_t in_com = 0;
    char* tok_read_in;
    char* tok_read_ptr;
    char* tok_arr[PSPL_MAX_COMMAND_ARGS];
    unsigned int tok_c;
    uint8_t just_read_tok = 0;
    uint8_t in_quote = 0;
    
    // Command name
    char* com_name;
    
    // Command argument start and end pointers
    const char* com_arg_start;
    const char* com_arg_end;
    
    // Whitespace line counter
    unsigned int white_line_count = 0;
    
    // Heading context stack
    pspl_toolchain_heading_context_t heading_ctx_stack[PSPL_HEADING_CTX_STACK_SIZE];
    
    // Indent context stack
    pspl_toolchain_indent_read_t indent_ctx_stack[PSPL_INDENT_CTX_STACK_SIZE];
    
    // Initial (Global) heading context
    compiler_state.heading_extension = NULL;
    heading_ctx_stack[0].heading_name = PSPL_GLOBAL_NAME;
    heading_ctx_stack[0].heading_argc = 0;
    heading_ctx_stack[0].heading_level = 0;
    heading_ctx_stack[0].heading_trace = NULL;
    compiler_state.heading_context = &heading_ctx_stack[0];
    
    // Initial indent context
    indent_ctx_stack[0].indent_level = 0;
    indent_ctx_stack[0].indent_trace = NULL;
    indent_ctx_stack[0].line_text = NULL;
    compiler_state.indent_context = &indent_ctx_stack[0];
    
    compiler_state.push_count = 0;
    
    // Start reading lines
    
    do { // Per line
        if (driver_state.line_num && *cur_line == '\n')
            ++cur_line;
        
        // Chomp leading whitespace (and track indent level)
        const char* cur_line_unchomped = cur_line;
        unsigned int added_spaces = 0;
        unsigned int added_tabs = 0;
        while (*cur_line == ' ' || *cur_line == '\t') {
            if (*cur_line == ' ')
                ++added_spaces;
            else if(*cur_line == '\t')
                ++added_tabs;
            ++cur_line;
        }
        int indent_level = added_spaces/PSPL_INDENT_SPACES + added_tabs;
        
        // End of this line
        const char* end_of_line = strchr(cur_line, '\n');
        if (!end_of_line)
            end_of_line = strchr(cur_line, '\0');
        
        
        char* line_buf = NULL;
        if (!in_com) {
            
#           pragma mark Scan For Whitespace
            
            // This line is whitespace if first character is newline
            if (*cur_line == '\n') {
                
                ++white_line_count;
                continue;
                
            } else if (white_line_count && compiler_state.heading_extension &&
                       compiler_state.heading_extension->toolchain_extension) {
                
                pspl_toolchain_whitespace_line_read_hook ws_hook =
                compiler_state.heading_extension->toolchain_extension->whitespace_line_read_hook;
                
                // Advise current heading extension that whitespace has occured
                if (ws_hook) {
                    
                    // Set callout context accordingly
                    driver_state.pspl_phase = PSPL_PHASE_COMPILE_EXTENSION;
                    driver_state.proc_extension = compiler_state.heading_extension;
                    
                    // Call hook
                    ws_hook(ext_driver_ctx, compiler_state.heading_context, white_line_count);
                    
                    // Unset callout context
                    driver_state.pspl_phase = PSPL_PHASE_COMPILE;
                    
                }
                
            }
            white_line_count = 0;
            
            
            
#           pragma mark Indent Context
            
            // Set indent context accordingly (after whitespace since it doesn't count)
            if (indent_level > compiler_state.indent_context->indent_level)
                indent_level = compiler_state.indent_context->indent_level + 1;
            else { // Deallocate same-and-higher-level contexts and bring stack down
                int down_level = compiler_state.indent_context->indent_level;
                while (indent_level <= down_level) {
                    pspl_toolchain_indent_read_t* de_ctx = &indent_ctx_stack[down_level];
                    free((void*)de_ctx->line_text);
                    --down_level;
                }
            }
            if (indent_level >= PSPL_INDENT_CTX_STACK_SIZE)
                pspl_error(-1, "Unsupported indent level",
                           "specified indent level would cause stack overflow; maximum level count: %u",
                           PSPL_INDENT_CTX_STACK_SIZE);
            
            // Target indent context
            pspl_toolchain_indent_read_t* target_ctx = &indent_ctx_stack[indent_level];
            compiler_state.indent_context = target_ctx;
            target_ctx->indent_level = indent_level;
            target_ctx->indent_trace = (indent_level)?&indent_ctx_stack[indent_level-1]:NULL;
            
            // Generate unchomped line buffer
            size_t line_len = end_of_line-cur_line_unchomped;
            if (line_len)
                --line_len;
            line_buf = malloc(line_len+1);
            strncpy(line_buf, cur_line_unchomped, line_len);
            line_buf[line_len] = '\0';
            target_ctx->line_text = line_buf;
            
            // Chomp leading and end whitespace
            char* line_start = line_buf;
            while (*line_start == ' ' || *line_start == '\t')
                ++line_start;
            
            char* line_end = line_buf + line_len;
            while (line_end > line_start &&
                   (*line_end == '\0' || *line_end == ' ' || *line_end == '\t'))
                --line_end;
            *(line_end+1) = '\0';
            
            // See if there is a bullet or number
            unsigned long bullet = 0;
            if (*line_start == '*')
                bullet = PSPL_BULLET_STAR;
            else if (*line_start == '-')
                bullet = PSPL_BULLET_DASH;
            else if (*line_start == '+')
                bullet = PSPL_BULLET_PLUS;
            else if ((bullet = strtol(line_start, NULL, 10))) {
                bullet &= ~PSPL_BULLET_MASK;
                char* dot = strchr(line_start, '.');
                if (dot)
                    line_start = dot;
                else
                    bullet = 0;
            }
            
            // Chomp leading whitespace again if bullet present
            if (bullet) {
                ++line_start;
                while (*line_start == ' ' || *line_start == '\t')
                    ++line_start;
            }
            
            
            target_ctx->bullet_value = bullet;
            target_ctx->line_text_stripped = line_start;
            
            
#           pragma mark Scan For Heading
            
            // Heading scan state
            uint8_t is_heading = 0;
            int level = 0;
            
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
                    while (*next_line == '=' || *next_line == ' ' || *next_line == '\t')
                        ++next_line;
                    if (*next_line == '\n' || *next_line == '\0') {
                        is_heading = 2;
                        level = 0;
                    }
                } else if (*next_line == '-') {
                    while (*next_line == '-' || *next_line == ' ' || *next_line == '\t')
                        ++next_line;
                    if (*next_line == '\n' || *next_line == '\0') {
                        is_heading = 2;
                        level = 1;
                    }
                }
            }
            
            if (is_heading) { // This is a heading spec
                
                // Validate heading level (and push one level if level greater)
                if (level > compiler_state.heading_context->heading_level)
                    level = compiler_state.heading_context->heading_level + 1;
                else { // Deallocate same-and-higher-level contexts and bring stack down
                    int down_level = compiler_state.heading_context->heading_level;
                    while (level <= down_level) {
                        pspl_toolchain_heading_context_t* de_ctx = &heading_ctx_stack[down_level];
                        if (de_ctx->heading_name != PSPL_GLOBAL_NAME)
                            free((void*)de_ctx->heading_name);
                        if (de_ctx->heading_argc)
                            free((void*)de_ctx->heading_argv[0]);
                        --down_level;
                    }
                }
                if (level >= PSPL_HEADING_CTX_STACK_SIZE)
                    pspl_error(-1, "Unsupported heading level",
                               "specified heading level would cause stack overflow; maximum level count: %u",
                               PSPL_HEADING_CTX_STACK_SIZE);
                
                // Target heading context
                pspl_toolchain_heading_context_t* target_ctx = &heading_ctx_stack[level];
                compiler_state.heading_context = target_ctx;
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
                        break; // Done with heading args
                        
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
                char* name = malloc(name_end-name_start+2);
                strncpy(name, name_start, name_end-name_start+1);
                name[name_end-name_start+1] = '\0';
                target_ctx->heading_name = name;
                
                
                // Skip line if using '='s or '-'s in heading
                if (is_heading == 2)
                    cur_line = end_of_line + 1;
                
                // Resolve "owner" extension from primary heading name
                if (!level) {
                    if (!strcasecmp(name, "GLOBAL"))
                        compiler_state.heading_extension = NULL;
                    else
                        compiler_state.heading_extension = get_heading_ext(name);
                }
                
                // Notify extension that it is now active
                if (compiler_state.heading_extension &&
                    compiler_state.heading_extension->toolchain_extension &&
                    compiler_state.heading_extension->toolchain_extension->heading_switch_hook) {
                    
                    // Set callout context accordingly
                    driver_state.pspl_phase = PSPL_PHASE_COMPILE_EXTENSION;
                    driver_state.proc_extension = compiler_state.heading_extension;
                    
                    // Callout to hook
                    compiler_state.heading_extension->toolchain_extension->
                    heading_switch_hook(ext_driver_ctx, compiler_state.heading_context);
                    
                    // Unset callout context
                    driver_state.pspl_phase = PSPL_PHASE_COMPILE;
                    
                }
                
                continue; // On to next line
                
            } // Done with heading spec
        }
    
#       pragma mark Scan For Command
        
        if (!in_com) { // Open parenthesis test
            
            // See if '(' exists on same line
            com_arg_start = strchr(cur_line, '(');
            if (com_arg_start && com_arg_start < end_of_line) {
                in_com = 1;
                
                // Ensure there is only whitespace between command name and '('
                const char* first_ws = strchr(cur_line, ' ');
                if (first_ws && first_ws < com_arg_start) {
                    while (*first_ws == ' ' || *first_ws == '\t')
                        ++first_ws;
                    if (first_ws != com_arg_start)
                        in_com = 0;
                }
                
                if (in_com) { // Same test for tabs
                    first_ws = strchr(cur_line, '\t');
                    if (first_ws && first_ws < com_arg_start) {
                        while (*first_ws == ' ' || *first_ws == '\t')
                            ++first_ws;
                        if (first_ws != com_arg_start)
                            in_com = 0;
                    }
                }
                
                if (in_com) { // This is a valid command; find argument end then get name
                    com_arg_end = strchr(com_arg_start, ')');
                    if (!com_arg_end)
                        pspl_error(-1, "Unclosed command arguments",
                                   "please add closing ')'");
                    
                    // Command name
                    const char* name_end = com_arg_start;
                    const char* test = strchr(cur_line, ' ');
                    if (test && test < name_end)
                        name_end = test;
                    test = strchr(cur_line, '\t');
                    if (test && test < name_end)
                        name_end = test;
                    
                    if (cur_line == name_end)
                        pspl_error(-1, "No-name command",
                                   "commands must have a name in PSPL");
                    com_name = malloc(name_end-cur_line+1);
                    strncpy(com_name, cur_line, name_end-cur_line);
                    com_name[name_end-cur_line] = '\0';
                    
                    // Initial command state
                    tok_read_in = malloc(com_arg_end-com_arg_start);
                    tok_read_ptr = tok_read_in;
                    tok_arr[0] = tok_read_ptr;
                    tok_c = 0;
                    just_read_tok = 0;
                    in_quote = 0;
                    cur_chr = com_arg_start + 1;
                }
                
            }
            
        }
        
        if (in_com) { // In command
            
            // Read in invocation up to end of command args;
            // saving token pointers using a buffer-array
            for (; cur_chr<com_arg_end ; ++cur_chr) {
                
                // Check for quoted token start or end (toggle)
                if (*cur_chr == '"' && !(in_quote && *(cur_chr-1) == '\\')) {
                    in_quote ^= 1;
                    continue;
                }
                
                // Ignore whitespace (or treat as token delimiter)
                if (!in_quote && (*cur_chr == ' ' || *cur_chr == '\t' || *cur_chr == '\n')) {
                    if (just_read_tok) {
                        just_read_tok = 0;
                        *tok_read_ptr = '\0';
                        ++tok_read_ptr;
                        tok_arr[tok_c] = tok_read_ptr;
                        if (tok_c >= PSPL_MAX_COMMAND_ARGS)
                            pspl_error(-2, "Maximum command arguments exceeded",
                                       "Up to %u arguments supported", PSPL_MAX_COMMAND_ARGS);
                    }
                    if (*cur_chr == '\n') { // Handle line break
                        ++cur_chr;
                        break;
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
            
            // Do next line if there is more of the directive
            if (cur_chr < com_arg_end)
                continue;
            
            // Directive has finished by this point
            in_com = 0;
            
            // Now determine which extension's command hook needs to be dispatched
            if (!strcasecmp(com_name, "NAME")) {
                
                if (!tok_c)
                    pspl_error(-1, "Invalid command use", "NAME command must have *one* argument");
                
                pspl_hash_ctx_t hash_ctx;
                pspl_hash_init(&hash_ctx);
                pspl_hash_write(&hash_ctx, tok_arr[0], strlen(tok_arr[0]));
                pspl_hash* result;
                pspl_hash_result(&hash_ctx, result);
                pspl_hash_cpy(&driver_state.indexer_ctx->psplc_hash, result);
                
            } else if (!strcasecmp(com_name, "PUSH_HEADING")) {
                
                if (!tok_c)
                    pspl_error(-1, "Invalid command use",
                               "PUSH_HEADING command must have *one* argument");
                
                pspl_toolchain_heading_context_t* pushed_ctx = &compiler_state.heading_context[1];
                pushed_ctx->heading_argc = tok_c - 1;
                int l;
                for (l=0 ; l<pushed_ctx->heading_argc ; ++l)
                    pushed_ctx->heading_argv[l] = tok_arr[l+1];
                pushed_ctx->heading_level = 0;
                pushed_ctx->heading_name = tok_arr[0];
                pushed_ctx->heading_trace = NULL;
                compiler_state.heading_context = pushed_ctx;
                ++compiler_state.push_count;
                
            } else if (!strcasecmp(com_name, "POP_HEADING")) {
                
                if (!compiler_state.push_count)
                    pspl_warn("Over-popped heading", "`%s` popped heading more times than pushed",
                              source->file_path);
                else {
                    const pspl_toolchain_heading_context_t* climb_ctx = compiler_state.heading_context;
                    while (climb_ctx->heading_trace)
                        climb_ctx = climb_ctx->heading_trace;
                    compiler_state.heading_context = (pspl_toolchain_heading_context_t*)&climb_ctx[-1];
                    --compiler_state.push_count;
                }
                
            } else {
                
                const pspl_extension_t* hook_ext = get_com_hook_ext(com_name);
                
                if (hook_ext) {
                    
                    // Set callout context accordingly
                    driver_state.pspl_phase = PSPL_PHASE_COMPILE_EXTENSION;
                    driver_state.proc_extension = hook_ext;
                    
                    // Callout to command hook
                    hook_ext->toolchain_extension->
                    command_call_hook(ext_driver_ctx, compiler_state.heading_context,
                                      com_name, tok_c, (const char**)tok_arr);
                    
                    // Unset callout context
                    driver_state.pspl_phase = PSPL_PHASE_COMPILE;
                    
                } else
                    pspl_warn("Unrecognised command",
                              "command `%s` not handled by any installed extensions; skipping",
                              com_name);
                
            }
            
            // Free resources
            free(com_name);
            free(tok_read_in);
            continue;
            
        }
        
        
#       pragma mark Fall Back To Line Read In
        
        if (compiler_state.heading_extension) { // Heading context required
            
            const pspl_extension_t* readin_ext = compiler_state.heading_extension;
            if (readin_ext->toolchain_extension) {
                
                const pspl_toolchain_extension_t* readin_toolext = readin_ext->toolchain_extension;
                
                // Set callout context accordingly
                driver_state.pspl_phase = PSPL_PHASE_COMPILE_EXTENSION;
                driver_state.proc_extension = compiler_state.heading_extension;
                

                // Determine if current heading context extension supports indenting
                uint8_t fallback_non_indent = 1;
                if (readin_toolext->indent_line_read_hook) {
                    
                    // Callout to indent line read hook
                    fallback_non_indent = (readin_toolext->indent_line_read_hook(ext_driver_ctx,
                                                                                 compiler_state.heading_context,
                                                                                 compiler_state.indent_context) < 0)?1:0;
                    
                }
                
                // Fallback to non-indent read in
                if (fallback_non_indent && readin_toolext->line_read_hook) {
                    
                    // Callout to line read hook
                    readin_toolext->line_read_hook(ext_driver_ctx,
                                                   compiler_state.heading_context,
                                                   line_buf);
                    
                }
                
                // Unset callout context
                driver_state.pspl_phase = PSPL_PHASE_COMPILE;
                
            }
            
        }
        
        
    } while ((cur_line = strchr(cur_line, '\n')) && ++driver_state.line_num);
    
    
#   pragma mark Platform Instruction Sending
    
    i=0;
    const pspl_extension_t* ext;
    while ((ext = pspl_available_extensions[i++])) {
        const pspl_toolchain_extension_t* tool_ext;
        if ((tool_ext = ext->toolchain_extension)) {
            if (tool_ext->platform_instruct_hook) {
                
                // Set callout context accordingly
                driver_state.pspl_phase = PSPL_PHASE_INSTRUCT_PLATFORM;
                driver_state.proc_extension = ext;
                
                // Callout
                tool_ext->platform_instruct_hook(ext_driver_ctx);
                
                // Unset callout context
                driver_state.pspl_phase = PSPL_PHASE_COMPILE;
                
            }
        }
    }
    
}

/**
 * Send instruction to all platforms
 *
 * *Must* be called within `platform_instruct_hook`
 */
void pspl_send_platform_instruction(const char* operation, const void* data) {
    int i;
    if (driver_state.pspl_phase != PSPL_PHASE_INSTRUCT_PLATFORM)
        return;
    driver_state.pspl_phase = PSPL_PHASE_COMPILE_PLATFORM;
    
    for (i=0 ; i<driver_state.tool_ctx->target_runtime_platforms_c ; ++i) {
        const pspl_platform_t* plat = driver_state.tool_ctx->target_runtime_platforms[i];
        if (plat && plat->toolchain_platform && plat->toolchain_platform->instruction_hook) {
            const pspl_extension_t* save_ext = driver_state.proc_extension;
            driver_state.proc_platform = plat;
            plat->toolchain_platform->instruction_hook(driver_state.tool_ctx, save_ext, operation, data);
            driver_state.proc_extension = save_ext;
        }
    }
}


