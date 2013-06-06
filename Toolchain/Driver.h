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

#include <sys/stat.h>
#include <sys/param.h>

#include <PSPLExtension.h>
#include "ReferenceGatherer.h"

#ifdef _WIN32
char* strtok_r(char *str,
               const char *delim,
               char **nextp);
#endif

/* Escape character sequences to control xterm */
#define NOBKD     "\E[47;49m"
#define RED       "\E[47;31m"NOBKD
#define GREEN     "\E[47;32m"NOBKD
#define YELLOW    "\E[47;33m"NOBKD
#define BLUE      "\E[47;34m"NOBKD
#define MAGENTA   "\E[47;35m"NOBKD
#define CYAN      "\E[47;36m"NOBKD
#define NORMAL    "\033[0m"
#define BOLD      "\033[1m"
#define UNDERLINE "\033[4m"
#define SGR0      "\E[m\017"

/* Maximum count of target platforms
 * (logical 32-bit bitfields are used to relate objects to platforms) */
#define PSPL_MAX_PLATFORMS 32

/* How many spaces equates to an indent level? */
#define PSPL_INDENT_SPACES 4

/* Maximum source file size (512K) */
#define PSPL_MAX_SOURCE_SIZE (512*1024)

/* Available extensions (NULL-terminated array) */
extern pspl_extension_t* pspl_available_extensions[];

/* Available target platforms (NULL-terminated array) */
extern pspl_platform_t* pspl_available_platforms[];

/* xterm Colour */
extern uint8_t xterm_colour;

/* Mode bits for logical setting/testing (set in `pspl_mode_opts`) */
#define PSPL_MODE_COMPILE_ONLY     (1<<0)
#define PSPL_MODE_PREPROCESS_ONLY  (1<<1)

/* Error and warning reporting */
#ifdef __clang__
void pspl_error(int exit_code, const char* brief, const char* msg, ...) __printflike(3, 4);
void pspl_warn(const char* brief, const char* msg, ...) __printflike(2, 3);
#else
void pspl_error(int exit_code, const char* brief, const char* msg, ...);
void pspl_warn(const char* brief, const char* msg, ...);
#endif


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
    const pspl_platform_t* const * platform_a;
    
    // Default endianness
    unsigned int default_endianness;
    
} pspl_toolchain_driver_opts_t;

/* State for a per-line preprocessor run */
struct _pspl_expansion_line_node {
    
    // Number of lines expanded
    unsigned int expanded_line_count;
    
    // Child source if this is an include (otherwise NULL)
    struct _pspl_toolchain_driver_source* included_source;
    
};

/* Source file state */
typedef struct _pspl_toolchain_driver_source {
    
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
    struct _pspl_expansion_line_node* expansion_line_nodes;
    
    // Parent source (for preprocessor inclusion backtrace)
    struct _pspl_toolchain_driver_source* parent_source;
    
    // Required extension set (NULL-terminated array)
    const pspl_extension_t** required_extension_set;

    
} pspl_toolchain_driver_source_t;


/* Loaded PSPLC file state */
typedef struct {
    
    // File path (as provided)
    const char* file_path;
    
    // Absolute enclosing directory (with trailing slash)
    const char* file_enclosing_dir;
    
    // Base file name (prepending path chomped)
    const char* file_name;
    
    
    // Required extension set (NULL-terminated array)
    const pspl_extension_t** required_extension_set;
    
    // Required platform set (NULL-terminated array)
    const pspl_platform_t** required_platform_set;
    
    // Name hash of PSPLC
    pspl_hash psplc_name_hash;
    
    
    // Compiled object data
    uint8_t* psplc_data;
    size_t psplc_data_len;
    
    
} pspl_toolchain_driver_psplc_t;

#include "ObjectIndexer.h"
void check_psplc_underflow(pspl_toolchain_driver_psplc_t* psplc, const void* cur_ptr);

/* Driver state (for error reporting and other global operations) */
typedef struct {
    
    // Driver phase
    enum {
        PSPL_PHASE_NONE                  = 0,
        PSPL_PHASE_INIT                  = 1,
        PSPL_PHASE_INIT_EXTENSION        = 2,
        PSPL_PHASE_PREPARE               = 3,
        PSPL_PHASE_PREPROCESS            = 4,
        PSPL_PHASE_PREPROCESS_EXTENSION  = 5,
        PSPL_PHASE_COMPILE               = 6,
        PSPL_PHASE_COMPILE_EXTENSION     = 7,
        PSPL_PHASE_COMPILE_PLATFORM      = 8,
        PSPL_PHASE_PACKAGE               = 9,
        PSPL_PHASE_FINISH_EXTENSION      = 10
    } pspl_phase;
    
    // File name
    const char* file_name;
    
    // Line number
    unsigned int line_num;
    
    // Processing extension
    union {
        const pspl_extension_t* proc_extension;
        const pspl_platform_t* proc_platform;
    };
    
    // Count of available extensions
    unsigned int ext_count;
    
    // Bitfield to track which extensions have been initialised
    uint32_t* ext_init_bits;
    
    // Toolchain context (public subset of this structure for extension use)
    const pspl_toolchain_context_t* tool_ctx;
    
    // Reference gathering context
    pspl_gatherer_context_t* gather_ctx;
    
    // Object indexing context
    pspl_indexer_context_t* indexer_ctx;
    
    // Staging area path (set to working dir otherwise)
    char staging_path[MAXPATHLEN];
    
    // Current source file
    pspl_toolchain_driver_source_t* source;
    
    // Out file path
    //const char* out_path;
    
} pspl_toolchain_driver_state_t;


extern pspl_toolchain_driver_state_t driver_state;


#endif // PSPL_INTERNAL
#endif

