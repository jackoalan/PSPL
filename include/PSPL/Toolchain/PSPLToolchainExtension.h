//
//  PSPLToolchainExtension.h
//  PSPL
//
//  Created by Jack Andersen on 4/15/13.
//
//

#ifndef PSPL_PSPLToolchainExtension_h
#define PSPL_PSPLToolchainExtension_h

#include <PSPL/PSPLExtension.h>


/* This will be set by build system. Contains unique extension ID.
 * The toolchain and runtime use this (in conjunction with preprocessor macros) 
 * to transparently maintain an extension namespace */
#define PSPL_EXT_NAME "TEST_EXT"




/* The PSPL toolchain driver uses a single-pass, phase-based execution model
 * in order to prepare data for the `psplb` file.
 *
 * The extension uses C function pointer to declare hooks
 * to be executed for each phase; providing relevant data where needed.
 *
 * From the toolchain extension's perspective, the phases include the following:
 *  * Init - called once for each extension when driver begins parsing a PSPL source (in no particular order)
 *      * Within an extension init, the extension may manually request the PSPL toolchain to initialise another extension right then and there (thereby implementing a simple dependency resolution system)
 *  * Global command call - when the extension claims a command signature, and it's called within the global heading context, this hook is activated
 *  * Context line read - when a claimed context is present, this hook is activated once for each line provided
 *  * Context line indent-sensitive read - overrides regular line read, provides indent context structure
 *  * Finish - called once for each extension before driver ends (in no particular order)
 *
 *
 * In addition to the phase hooks, a toolchain extension is responsible for defining
 * some metadata values to stipulate the extension's responsibility in the toolchain.
 *
 * Here are the types of metadata entries the class may provide:
 *  * Claimed heading names (sub headings implicitly also claimed, able to specify as weak)
 *  * Claimed global command names (able to specify as weak)
 *  * Claimed global preprocessor directives
 */


/* Toolchain driver context (data consistent from init to finish) */
typedef struct {
    pspl_runtime_platform_t* target_runtime_platform;
    const char* pspl_filename;
    unsigned int pspl_current_line;
} pspl_toolchain_context_t;

/* Init hook type */
typedef int(*pspl_toolchain_init_hook)(const pspl_toolchain_context_t* driver_context);

/* Finish hook type */
typedef void(*pspl_toolchain_finish_hook)(const pspl_toolchain_context_t* driver_context);


/* Global line preprocessor directive substitution hook 
 * The hook implementation should call `pspl_preprocessor_add_line` 
 * for each line of code that the preprocessor should emit in place of directive call */
typedef void(*pspl_toolchain_line_preprocessor_hook)(const pspl_toolchain_context_t* driver_context,
                                                     const char* directive_name,
                                                     unsigned int directive_argc,
                                                     const char* const * directive_argv);


/* Heading context type */
typedef struct _pspl_toolchain_heading_context {
    // Literal name of heading
    const char* heading_name;
    
    // Heading argument count
    unsigned int heading_argc;
    // Heading argument array
    const char* const * heading_argv;
    
    // Level of heading (0 is primary; increments from there)
    unsigned int heading_level;
    // "Stack-trace" link-list of heading levels (towards top-level reference; NULL if primary heading)
    const struct _pspl_toolchain_heading_context* heading_trace;
    
} pspl_toolchain_heading_context_t;


/* Command call hook type (when C-style command syntax detected)
 * will fall back to line-read hook if negative value returned
 * `current_heading` is NULL if global context is active */
typedef int(*pspl_toolchain_command_call_hook)(const pspl_toolchain_context_t* driver_context,
                                               const pspl_toolchain_heading_context_t* current_heading,
                                               const char* command_name,
                                               unsigned int command_argc,
                                               const char* const * command_argv);



/* Line read hook type */
typedef void(*pspl_toolchain_line_read_hook)(const pspl_toolchain_context_t* driver_context,
                                             const pspl_toolchain_heading_context_t* current_heading,
                                             const char* line_text);

/* Generate platform-native bullet enumerations */
static const unsigned int PSPL_BULLET_MASK = (~(((~0)<<2)>>2));
static const unsigned int PSPL_BULLET_STAR = (((~0)>>1)&PSPL_BULLET_MASK);
static const unsigned int PSPL_BULLET_DASH = (((~0)>>1)&PSPL_BULLET_MASK);

/* Indentation context type */
typedef struct _pspl_toolchain_indent_read {
    // Line text (including bullet/numbering characters and leading whitespace)
    const char* line_text;
    
    // Bullet/numbering character/value
    // ('*':PSPL_BULLET_STAR, '-':PSPL_BULLET_DASH, <UNSIGNED NUMERIC VALUE FOR NUMBERED LIST>)
    unsigned int bullet_value;
    
    // Line text (stripped of bullet/numbering characters and whitespace)
    const char* line_text_stripped;
    
    // Level of indent (0 is no indent; increments from there)
    unsigned int indent_level;
    // "Stack-trace" link-list of indent levels (towards top-level indent; NULL if no indent)
    const struct _pspl_toolchain_indent_read* indent_trace;
    
} pspl_toolchain_indent_read_t;

/* Indentation-sensitive line read hook type 
 * Falls back to regular line read hook if negative value returned */
typedef int(*pspl_toolchain_indent_line_read_hook)(const pspl_toolchain_context_t* driver_context,
                                                   const pspl_toolchain_heading_context_t* current_heading,
                                                   const pspl_toolchain_indent_read_t* indent_line_read);


/* Main toolchain extension structure */
typedef struct {
    
    // Metadata fields
    // All arrays are `NULL` terminated
    const char** claimed_heading_names;
    const char** weak_claimed_heading_names;
    
    const char** claimed_global_command_names;
    const char** weak_claimed_global_command_names;
    
    const char** claimed_global_preprocessor_directives;
    const char** weak_claimed_global_preprocessor_directives;
    
    // Hook fields
    pspl_toolchain_init_hook* init_hook;
    pspl_toolchain_finish_hook* finish_hook;
    pspl_toolchain_line_preprocessor_hook* line_preprocessor_hook;
    pspl_toolchain_command_call_hook* command_call_hook;
    pspl_toolchain_line_read_hook* line_read_hook;
    pspl_toolchain_indent_line_read_hook* indent_line_read_hook;
    
} pspl_toolchain_extension_t;

/* A macro to abstract the global details of extension namespacing */
#define PSPL_INSTALLED_TOOLCHAIN_EXT pspl_toolchain_extension_t PSPL_EXT_NAME##_extension

/* A sample toolchain extension installation (all NULL hooks) */
static const char* claimed_heading_names[] = {"VERTEX", "FRAGMENT", NULL};

PSPL_INSTALLED_TOOLCHAIN_EXT = {
    
    .claimed_heading_names = claimed_heading_names,
    
    .init_hook = NULL,
    .finish_hook = NULL,
    .command_call_hook = NULL,
    .line_read_hook = NULL,
    .indent_line_read_hook = NULL
};


#endif
