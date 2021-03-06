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
#include <PSPLExtension.h>

#define PSPL_MAX_HEADING_ARGS 16

/**
 * @file PSPL/PSPLToolchainExtension.h
 * @brief API for extending the PSPL language
 * @defgroup pspl_toolchain API for extending the PSPL language
 * @ingroup PSPL
 * @{
 */


#pragma mark Copyright Reproduction

/* A static copy of the entire MIT licence */
extern const char* PSPL_MIT_LICENCE;

/* A static copy of the entire FreeBSD licence */
extern const char* PSPL_FREEBSD_LICENCE;

/**
 * Provide software licencing details for extension
 *
 * Call this method (within copyright hook) for each software component 
 * contained in extension or platform
 *
 * @param component_name Name of software component contained
 * @param copyright Copyright string of software component
 * @param licence Shortest-representable copy of attached licence 
 *        (line-wrapped to 80 columns)
 */
void pspl_toolchain_provide_copyright(const char* component_name,
                                      const char* copyright,
                                      const char* licence);

/**
 * Extension hook for providing copyright details using `pspl_toolchain_provide_copyright` 
 */
typedef void(*pspl_toolchain_copyright_hook)(void);


/**
 * Provide sub-extension details for built-in help
 *
 * Call this method (within subext hook) for each sub-extension contained
 * within extension
 *
 * @param subext_name Name of sub-extension
 * @param subext_desc Description of sub-extension
 * @param indent_level Level of sub-intent; 0 and 1 are both *one* indent level
 */
void pspl_toolchain_provide_subext(const char* subext_name, const char* subext_desc,
                                   unsigned indent_level);

/**
 * Extension hook for providing sub-extension details using `pspl_toolchain_provide_subext`
 */
typedef void(*pspl_toolchain_subext_hook)(void);


#pragma mark Compiler Line-Read-Only Mode

/**
 * Put compiler into line-read-only mode
 *
 * Useful for building source-text buffers where syntax conflicts
 * with PSPL may occur
 */
void pspl_toolchain_line_read_only();

/**
 * Take compiler out of line-read-only mode
 *
 * Reverses effects of `pspl_toolchain_line_read_only`
 */
void pspl_toolchain_line_read_auto();


#pragma mark Driver Context

/**
 * Toolchain driver context (data consistent from init to finish) 
 */
typedef struct {
    /**< Array of current target runtime platform(s) for toolchain driver */
    unsigned int target_runtime_platforms_c; /**< Count of target platforms */
    const pspl_platform_t* const * target_runtime_platforms; /**< Platform array */
    
    /**< Target endian mode (PSPL_ENDIAN_LITTLE,PSPL_ENDIAN_BIG,PSPL_ENDIAN_BI) */
    unsigned target_endianness;
    
    /**< Name of PSPL source (including extension) */
    const char* pspl_name;
    
    /**< Absolute path of PSPL-enclosing directory (with trailing slash) */
    const char* pspl_enclosing_dir;
    
    /**< Driver-definitions (added with `-D` flag) */
    unsigned int def_c; /**< Count of defs */
    const char** def_k; /**< Array (of length `def_c`) containing defined key strings */
    const char** def_v; /**< Array (of length `def_c`) containing index-associated values (or NULL if no values) */
    
    /**< Output path */
    const char* output_path;
    
} pspl_toolchain_context_t;

#pragma mark -

#pragma mark Toolchain Extension Hook Types

/**
 * Init hook type
 *
 * Called at the start of *one* PSPL source preprocessing
 */
typedef int(*pspl_toolchain_init_hook)(const pspl_toolchain_context_t* driver_context);

/**
 * Platform instruct hook type
 *
 * Called for each extension *just before* platforms have their generate hooks called
 */
typedef void(*pspl_toolchain_platform_instruct_hook)(const pspl_toolchain_context_t* driver_context);

/**
 * Send instruction to all platforms
 *
 * *Must* be called within `platform_instruct_hook`
 */
void pspl_toolchain_send_platform_instruction(const char* operation, const void* data);

/**
 * Finish hook type
 *
 * Called at the end of *one* PSPL source compiling
 */
typedef void(*pspl_toolchain_finish_hook)(const pspl_toolchain_context_t* driver_context);

/**
 * Request the immediate initialisation of another extension by name
 * (only valid within init hook)
 *
 * @param ext_name Unique name of extension
 * @return 0 if successful
 */
int pspl_toolchain_init_other_extension(const char* ext_name);


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

/* Emit formatted PSPL line (may be called repeatedly, in desired order)
 * Newlines ('\n') are automatically tokenised into multiple `add_line` calls */
void pspl_preprocessor_add_line(const char* line_text, ...);

/* Convenience function to add formatted line with specified indent level (0 is primary) */
void pspl_preprocessor_add_indent_line(unsigned int indent_level, const char* line_text, ...);

/* Convenience function to emit PSPL primary-heading push
 * (emits 'PUSH_HEADING(primary_heading_name)') 
 * Saves heading state onto internal toolchain-driver stack */
void pspl_preprocessor_add_heading_push(const char* primary_heading_name);

/* Convenience function to emit PSPL primary-heading push
 * (emits 'POP_HEADING()')
 * restores the heading state before previous PSPL_HEADING_PUSH() */
void pspl_preprocessor_add_heading_pop();

/* Convenience function to emit command call with arguments. 
 * All variadic arguments *must* be C-strings */
void _pspl_preprocessor_add_command_call(const char* command_name, ...);
#define pspl_preprocessor_add_command_call(command_name, ...) _pspl_preprocessor_add_command_call(command_name, __VA_ARGS__, NULL)


#pragma mark Hierarchical Heading Context

/* Used within all parsing routines in the PSPL compilation pass */

/* Heading context type */
typedef struct _pspl_toolchain_heading_context {
    // Literal name of (sub)heading
    const char* heading_name;
    
    // Heading argument count
    unsigned int heading_argc;
    // Heading argument array
    const char* heading_argv[PSPL_MAX_HEADING_ARGS];
    
    // Level of heading (0 is primary; increments from there)
    unsigned int heading_level;
    // "Stack-trace" link-list of heading levels (towards top-level reference; NULL if primary heading)
    const struct _pspl_toolchain_heading_context* heading_trace;
    
} pspl_toolchain_heading_context_t;


#pragma mark Heading Context Switch Hooking

/* Called whenever a heading context switch occurs (into the extension's ownership) */
typedef void(*pspl_toolchain_heading_switch_hook)(const pspl_toolchain_context_t* driver_context,
                                                  const pspl_toolchain_heading_context_t* current_heading);


#pragma mark C-style Command Call Hooking

/* Command call hook type (when C-style command syntax detected)
 * will fall back to line-read hook if negative value returned
 * `current_heading` is NULL if global context is active */
typedef void(*pspl_toolchain_command_call_hook)(const pspl_toolchain_context_t* driver_context,
                                                const pspl_toolchain_heading_context_t* current_heading,
                                                const char* command_name,
                                                unsigned int command_argc,
                                                const char** command_argv);


#pragma mark Whitespace Line Read Hooking

/* When consecutive line(s) of whitespace are encountered, this method may be 
 * implemented to handle the situation. */
typedef void(*pspl_toolchain_whitespace_line_read_hook)(const pspl_toolchain_context_t* driver_context,
                                                        const pspl_toolchain_heading_context_t* current_heading,
                                                        unsigned int white_line_count);


#pragma mark Generic Line Read Hooking

/* Line read hook type */
typedef void(*pspl_toolchain_line_read_hook)(const pspl_toolchain_context_t* driver_context,
                                             const pspl_toolchain_heading_context_t* current_heading,
                                             const char* line_text);


#pragma mark Indented (and/or Markdown-list) Line Read Hooking

/* Platform-native bullet enumerations */
extern const unsigned long PSPL_BULLET_MASK;
extern const unsigned long PSPL_BULLET_STAR;
extern const unsigned long PSPL_BULLET_DASH;
#define PSPL_BULLET_PLUS PSPL_BULLET_MASK

/* Indentation context type */
typedef struct _pspl_toolchain_indent_read {
    // Line text (including bullet/numbering characters and leading whitespace)
    const char* line_text;
    
    // Bullet/numbering character/value
    // ('*':PSPL_BULLET_STAR, '-':PSPL_BULLET_DASH, <UNSIGNED NUMERIC VALUE FOR NUMBERED LIST>)
    unsigned long bullet_value;
    
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


#pragma mark Add PSPLC-Embedded Bi-endian Data Object (for transit to runtime)

/* These embed routines may be called within any toolchain-extension hook
 * (including init, preprocessor, and finish) */

/* Objects may be inclusively specified for specific platforms by 
 * providing a NULL-terminated array of `pspl_platform_t` references 
 * sourced from the driver-context's `target_runtime_platforms`.
 *
 * This array is provided to the `platforms` argument. `NULL` may be provided
 * instead to indicate inclusion into all platforms */

/* Object types must be generated with DEF_BI_OBJ_TYPE() and the instances
 * must be populated with SET_BI() */

/* Each extension has its own object namespace, so identical key-strings may be 
 * provided between toolchain extensions. Setting the same key multiple times
 * results in the previous object being overloaded with the new object */

/* Add data object (keyed with a null-terminated string stored as 32-bit truncated SHA1 hash) */
void pspl_embed_hash_keyed_object(const pspl_platform_t** platforms,
                                  const char* key,
                                  const void* little_object,
                                  const void* big_object,
                                  size_t object_size);

/* Add data object (keyed with a non-hashed 32-bit unsigned numeric value) 
 * Integer keying uses a separate namespace from hashed keying */
void pspl_embed_integer_keyed_object(const pspl_platform_t** platforms,
                                     uint32_t key,
                                     const void* little_object,
                                     const void* big_object,
                                     size_t object_size);


#pragma mark Add File For Packaging Into PSPLP Flat-File and/or PSPLPD Directory

/* Standard data conversion interface */
void pspl_converter_progress_update(double progress);
typedef int(*pspl_converter_file_hook)(char* path_out, const char* path_in, const char* path_ext_in, const char* suggested_path, void* user_ptr);
typedef int(*pspl_converter_membuf_hook)(void** buf_out, size_t* len_out, const char* path_in, void* user_ptr);

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
void pspl_package_file_augment(const pspl_platform_t** platforms, const char* path_in,
                               const char* path_ext_in,
                               pspl_converter_file_hook converter_hook, uint8_t move_output,
                               void* user_ptr,
                               pspl_hash** hash_out);
void pspl_package_membuf_augment(const pspl_platform_t** platforms, const char* path_in,
                                 const char* path_ext_in,
                                 pspl_converter_membuf_hook converter_hook,
                                 void* user_ptr,
                                 pspl_hash** hash_out);



#pragma mark Main Toolchain Extension Structure (every extension needs one)

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
    pspl_toolchain_platform_instruct_hook platform_instruct_hook;
    pspl_toolchain_copyright_hook copyright_hook;
    pspl_toolchain_subext_hook subext_hook;
    pspl_toolchain_line_preprocessor_hook line_preprocessor_hook;
    pspl_toolchain_command_call_hook command_call_hook;
    pspl_toolchain_heading_switch_hook heading_switch_hook;
    pspl_toolchain_whitespace_line_read_hook whitespace_line_read_hook;
    pspl_toolchain_line_read_hook line_read_hook;
    pspl_toolchain_indent_line_read_hook indent_line_read_hook;
    
} pspl_toolchain_extension_t;


#pragma mark -

#pragma mark Toolchain Platform Hook Types

#include <PSPL/PSPL_IR.h>

/* Platform instruction hook
 * Sent from participating extensions to *all* platforms 
 * Intended to unify all platforms together in a similar way */
typedef void(*pspl_toolchain_platform_instruction_hook)(const pspl_toolchain_context_t* driver_context,
                                                        const pspl_extension_t* sending_extension,
                                                        const char* operation,
                                                        const void* data);

/* Platform generator hook (called once per PSPLC compile after all receive calls)
 * actually generates final data objects for storage into PSPLC 
 * also serves as a finish routine */
typedef void(*pspl_toolchain_platform_generate_hook)(const pspl_toolchain_context_t* driver_context);

/* Embed data objects for generator */

/* Add data object (keyed with a null-terminated string stored as 32-bit truncated SHA1 hash) */
void pspl_embed_platform_hash_keyed_object(const char* key,
                                           const void* little_object,
                                           const void* big_object,
                                           size_t object_size);

/* Add data object (keyed with a non-hashed 32-bit unsigned numeric value)
 * Integer keying uses a separate namespace from hashed keying */
void pspl_embed_platform_integer_keyed_object(uint32_t key,
                                              const void* little_object,
                                              const void* big_object,
                                              size_t object_size);


#pragma mark Main Toolchain Platform Structure (every platform needs one)

/* Main toolchain platform structure */
typedef struct _pspl_toolchain_platform {
    
    // All fields are optional and may be set `NULL`
    
    // Hook fields
    pspl_toolchain_init_hook init_hook;
    pspl_toolchain_copyright_hook copyright_hook;
    pspl_toolchain_platform_instruction_hook instruction_hook;
    pspl_toolchain_platform_generate_hook generate_hook;
    
} pspl_toolchain_platform_t;

/** @} */

#endif // PSPL_TOOLCHAIN
#endif
