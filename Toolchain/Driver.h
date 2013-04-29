//
//  Driver.h
//  PSPL
//
//  Created by Jack Andersen on 4/28/13.
//
//

#ifndef PSPL_Driver_h
#define PSPL_Driver_h
#ifdef PSPL_INTERNAL

#include <PSPL/PSPLExtension.h>

/* Available extensions (NULL-terminated array) */
extern pspl_extension_t* pspl_available_extensions[];

/* Available target platforms (NULL-terminated array) */
extern pspl_runtime_platform_t* pspl_available_target_platforms[];

/* Default target platform */
extern pspl_runtime_platform_t* pspl_default_target_platform;

/* Mode bits for logical setting/testing (set in `pspl_mode_opts`) */
#define PSPL_MODE_COMPILE_ONLY (1<<0)
#define PSPL_MODE_PREPROCESS_ONLY (1<<1)

/* Error and warning reporting */
void pspl_error(int exit_code, const char* brief, const char* msg, ...);
void pspl_warn(const char* brief, const char* msg, ...);


/* Structure composing all global options together */
typedef struct {
    // Bits indicating pspl's behaviour
    uint32_t pspl_mode_opts;
    
    // Output path (or NULL for stdout)
    const char* out_path;
    
    // Reflist output path (or NULL for no reflist)
    const char* reflist_out_path;
    
    // Staging area path (set to working dir otherwise)
    const char* staging_path;
    
    // Count of sources
    unsigned int source_c;
    
    // Array of sources
    const char** source_a;
    
    // Count of defs
    unsigned int def_c;
    
    // Array of def keys
    const char** def_k;
    
    // Array of def values
    const char** def_v;
    
    // Count of target platforms
    unsigned int platform_c;
    
    // Array of target platforms
    const pspl_runtime_platform_t* const * platform_a;
    
} pspl_toolchain_driver_opts_t;


/* Driver state (for error reporting) */
typedef struct {
    
    // Driver phase
    enum {
        PSPL_PHASE_NONE = 0,
        PSPL_PHASE_INIT = 1,
        PSPL_PHASE_INIT_EXTENSION = 2,
        PSPL_PHASE_PREPARE = 3,
        PSPL_PHASE_PREPROCESS = 4,
        PSPL_PHASE_PREPROCESS_EXTENSION = 5,
        PSPL_PHASE_COMPILE = 6,
        PSPL_PHASE_COMPILE_EXTENSION = 7,
        PSPL_PHASE_PACKAGE = 8,
        PSPL_PHASE_FINISH_EXTENSION = 9
    } pspl_phase;
    
    // File name
    const char* file_name;
    
    // Line number
    unsigned int line_num;
    
    // Processing extension
    pspl_extension_t* proc_extension;
    
} pspl_toolchain_driver_state_t;


/* Source file state */
typedef struct {
    
    // File path (as provided)
    const char* file_path;
    
    // Absolute enclosing directory (with trailing slash)
    const char* file_enclosing_dir;
    
    // Base file name (prepending path chomped)
    const char* file_name;
    
    // Full original source buffer
    const char* original_source;
    
    // Preprocessed source
    const char* preprocessed_source;
    
    // Array of individual preprocessor expansion line counts
    // (mapping each original line to a count of expanded lines)
    // (an array of all 1s will indicate a preprocessed file of the same length as original)
    unsigned int* expansion_line_counts;
    
} pspl_toolchain_driver_source_t;

extern pspl_toolchain_driver_state_t driver_state;


#endif // PSPL_INTERNAL
#endif
