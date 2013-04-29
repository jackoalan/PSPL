//
//  PSPLToolchainExtension.h
//  PSPL
//
//  Created by Jack Andersen on 4/15/13.
//
//

#ifndef PSPL_PSPLToolchainExtension_h
#define PSPL_PSPLToolchainExtension_h
#ifdef PSPL_TOOLCHAIN

#include <stdlib.h>
#include <PSPL/PSPLExtension.h>


/* This will be set by build system. Contains unique extension ID.
 * The toolchain and runtime use this (in conjunction with preprocessor macros) 
 * to transparently maintain an extension namespace system */
//#define PSPL_EXT_NAME TEST_EXT




/* The PSPL toolchain driver uses a two-pass, phase-based execution model
 * in order to prepare data for the `psplc` file and resulting package.
 *
 * The extension uses C function pointers to declare hooks
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


#pragma mark Error Reporting

/* During the toolchain execution phases, the extension may wish to notify
 * the driver that an error condition has arisn (perhaps due to bad syntax by 
 * the shader author). These functions allow the extension to either raise a 
 * *warning* or *error* and bring it to the art pipeline executor's attention.
 */

void pspl_error(int exit_code, const char* brief, const char* msg, ...);

void pspl_warn(const char* brief, const char* msg, ...);


#pragma mark Driver Context

/* Toolchain driver context (data consistent from init to finish, except line number) */
typedef struct {
    // Array of current target runtime platform(s) for toolchain driver
    unsigned int target_runtime_platforms_c; // Count of target platforms
    const pspl_runtime_platform_t* const * target_runtime_platforms; // Platform array
    
    // Name of PSPL source (including extension)
    const char* pspl_name;
    
    // Absolute path of PSPL-enclosing directory (with trailing slash)
    const char* pspl_enclosing_dir;
    
    // Driver-definitions (added with `-D` flag)
    unsigned int def_c; // Count of defs
    const char** def_k; // Array (of length `def_c`) containing defined key strings
    const char** def_v; // Array (of length `def_c`) containing index-associated values (or NULL if no values)
    
} pspl_toolchain_context_t;

/* Init hook type */
typedef int(*pspl_toolchain_init_hook)(const pspl_toolchain_context_t* driver_context);

/* Finish hook type */
typedef void(*pspl_toolchain_finish_hook)(const pspl_toolchain_context_t* driver_context);

/* Request the immediate initialisation of another extension 
 * (only valid within init hook) */
int pspl_toolchain_init_other_extension(pspl_extension_t* extension);


#pragma mark Preprocessor Directive Hooking

/* All preprocessing occurs within the first read pass of the PSPL source */

/* Global line preprocessor directive substitution hook 
 * The hook implementation should call `pspl_preprocessor_add_line` 
 * for each line of code that the preprocessor should emit in place of directive call */
typedef void(*pspl_toolchain_line_preprocessor_hook)(const pspl_toolchain_context_t* driver_context,
                                                     const char* directive_name,
                                                     unsigned int directive_argc,
                                                     const char** directive_argv);

/* All 'pspl_preprocessor_*' functions must be called within preprocessor hook */

/* Emit PSPL line (may be called repeatedly, in desired order)
 * Newlines ('\n') are chomped off */
void pspl_preprocessor_add_line(const char* line_text);

/* Convenience function to add line with specified indent level (0 is primary) */
void pspl_preprocessor_add_indent_line(const char* line_text, unsigned int indent_level);

/* Convenience function to emit PSPL primary-heading push
 * (emits 'PSPL_HEADING_PUSH(primary_heading_name)') 
 * Saves heading state onto internal toolchain-driver stack */
void pspl_preprocessor_add_heading_push(const char* primary_heading_name);

/* Convenience function to emit PSPL primary-heading push
 * (emits 'PSPL_HEADING_POP()')
 * restores the heading state before previous PSPL_HEADING_PUSH() */
void pspl_preprocessor_add_heading_pop();

/* Convenience function to emit command call with arguments. 
 * All varadic arguments *must* be C-strings */
void pspl_preprocessor_add_command_call(const char* command_name, ...);


#pragma mark Hierarchical Heading Context

/* Used within all parsing routines in the PSPL compilation pass */

/* Heading context type */
typedef struct _pspl_toolchain_heading_context {
    // Literal name of (sub)heading
    const char* heading_name;
    
    // Heading argument count
    unsigned int heading_argc;
    // Heading argument array
    const char** heading_argv;
    
    // Level of heading (0 is primary; increments from there)
    unsigned int heading_level;
    // "Stack-trace" link-list of heading levels (towards top-level reference; NULL if primary heading)
    const struct _pspl_toolchain_heading_context* heading_trace;
    
} pspl_toolchain_heading_context_t;


#pragma mark C-style Command Call Hooking

/* Command call hook type (when C-style command syntax detected)
 * will fall back to line-read hook if negative value returned
 * `current_heading` is NULL if global context is active */
typedef int(*pspl_toolchain_command_call_hook)(const pspl_toolchain_context_t* driver_context,
                                               const pspl_toolchain_heading_context_t* current_heading,
                                               const char* command_name,
                                               unsigned int command_argc,
                                               const char** command_argv);

#pragma mark Generic Line Read Hooking

/* Line read hook type */
typedef void(*pspl_toolchain_line_read_hook)(const pspl_toolchain_context_t* driver_context,
                                             const pspl_toolchain_heading_context_t* current_heading,
                                             const char* line_text);


#pragma mark Indented (and/or Markdown-list) Line Read Hooking

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


#pragma mark Notify Toolchain of Referenced File Dependency

/* Add referenced source file to Reference Gathering list */
void pspl_gather_referenced_file(const char* file_path);


#pragma mark Add PSPLC-Embedded Bi-endian Data Object (for transit to runtime)

/* These embed routines may be called within any toolchain-extension hook
 * (including init, preprocessor, and finish) */

/* Objects may be inclusively specified for specific platforms by 
 * providing a NULL-terminated array of `pspl_runtime_platform_t` references 
 * sourced from the driver-context's `target_runtime_platforms`.
 *
 * This array is provided to the `platforms` argument. `NULL` may be provided
 * instead to indicate inclusion into all platforms */

/* Object types must be generated with DECL_BI_OBJ_TYPE() and the instances
 * must be populated with SET_BI() */

/* Each extension has its own object namespace, so identical key-strings may be 
 * provided between toolchain extensions. Setting the same key multiple times
 * results in the previous object being overloaded with the new object */

/* Add data object (keyed with a null-terminated string stored as 32-bit truncated SHA1 hash) */
int __pspl_psplc_embed_hash_keyed_object(pspl_runtime_platform_t* platforms,
                                         const char* key,
                                         const void* little_object,
                                         const void* big_object,
                                         size_t object_size);
#define pspl_psplc_embed_hash_keyed_object(platforms,key,object) \
__pspl_psplc_embed_hash_keyed_object(platforms,key,&(object.little),&(object.big),sizeof(object.native))

/* Add data object (keyed with a non-hashed 32-bit unsigned numeric value) 
 * Integer keying uses a separate namespace from hashed keying */
int __pspl_psplc_embed_integer_keyed_object(pspl_runtime_platform_t* platforms,
                                            uint32_t key,
                                            const void* little_object,
                                            const void* big_object,
                                            size_t object_size);
#define pspl_psplc_embed_integer_keyed_object(platforms,key,object) \
__pspl_psplc_embed_integer_keyed_object(platforms,key,&(object.little),&(object.big),sizeof(object.native))


#pragma mark Add File For Packaging Into PSPLP Flat-File and/or PSPLPD Directory

/* The `platforms` argument works as described for psplc_embed_* functions above
 * `path` may be relative to PSPL file or absolute to toolchain host */

/* The toolchain will either move or copy (based on `move` argument) to the
 * toolchain's staging directory and rename it to a 32-bit truncated SHA1 hash 
 * to eliminate redundancies */
 
/* The toolchain will also take a complete SHA1 hash of the file data and use it to 
 * eliminate duplicates from the final PSPL package. The hash is then truncated
 * to 32-bits and provided to the toolchain extension via `hash_out`. This hash may then
 * be stored in an embedded PSPLC object and used to uniquely load and access 
 * That file's contents using PSPL's runtime extension API */
 
/* Add file for PSPL-packaging */
int pspl_package_add_file(pspl_runtime_platform_t* platforms,
                          const char* path,
                          int move,
                          uint32_t* hash_out);


#pragma mark Main Extension Composition Structure (every extension needs one)

/* Main toolchain extension structure */
typedef struct _pspl_toolchain_extension {
    
    // All fields are optional and may be set `NULL`
    
    // Metadata fields
    // All arrays need to be `NULL` terminated
    const char** claimed_heading_names;
    const char** weak_claimed_heading_names;
    
    const char** claimed_global_command_names;
    const char** weak_claimed_global_command_names;
    
    const char** claimed_global_preprocessor_directives;
    const char** weak_claimed_global_preprocessor_directives;
    
    // Hook fields
    pspl_toolchain_init_hook init_hook;
    pspl_toolchain_finish_hook finish_hook;
    pspl_toolchain_line_preprocessor_hook line_preprocessor_hook;
    pspl_toolchain_command_call_hook command_call_hook;
    pspl_toolchain_line_read_hook line_read_hook;
    pspl_toolchain_indent_line_read_hook indent_line_read_hook;
    
} pspl_toolchain_extension_t;

/* A macro to abstract the global details of extension namespacing */
#define _PSPL_INSTALLED_TOOLCHAIN_EXT_CAT2(a) a ## _extension
#define _PSPL_INSTALLED_TOOLCHAIN_EXT_CAT(a) _PSPL_INSTALLED_TOOLCHAIN_EXT_CAT2(a)
#define PSPL_INSTALLED_TOOLCHAIN_EXT static const pspl_toolchain_extension_t _PSPL_INSTALLED_TOOLCHAIN_EXT_CAT(PSPL_EXT_NAME)

/* A sample toolchain extension installation (all NULL hooks) */
/*
static const char* claimed_heading_names[] = {"VERTEX", "FRAGMENT", NULL};

PSPL_INSTALLED_TOOLCHAIN_EXT = {
    
    .claimed_heading_names = claimed_heading_names,
    
    .init_hook = NULL,
    .finish_hook = NULL,
    .command_call_hook = NULL,
    .line_read_hook = NULL,
    .indent_line_read_hook = NULL
};
 */

#endif // PSPL_TOOLCHAIN
#endif
