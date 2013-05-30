//
//  PSPL.h
//  PSPL
//
//  Created by Jack Andersen on 4/15/13.
//
//

#ifndef PSPL_PSPL_h
#define PSPL_PSPL_h

#include <string.h>
#include <PSPL/PSPLExtension.h>

/* PSPL's method of defining numeric values in a bi-endian manner */
#include <PSPL/PSPLValue.h>

/**
 * @file PSPL/PSPL.h
 * @brief General Toolchain *and* Runtime Public API Bits
 * @defgroup pspl_malloc Memory management context
 * @defgroup pspl_hash Hash Manipulation Routines
 * @defgroup pspl_runtime Runtime Public API
 * @ingroup PSPL
 * @{
 */

/**
 * Endianness enumerations
 */
enum pspl_endianness {
    PSPL_UNSPEC_ENDIAN = 0,
    PSPL_LITTLE_ENDIAN = 1,
    PSPL_BIG_ENDIAN    = 2,
    PSPL_BI_ENDIAN     = 3
};

#pragma mark Error Reporting

#ifdef __clang__
void pspl_error(int exit_code, const char* brief, const char* msg, ...) __printflike(3, 4);
void pspl_warn(const char* brief, const char* msg, ...) __printflike(2, 3);
#else
/**
 * Raise globally-recognised error condition (terminating the entire program)
 *
 * @param exit_code Error code to use with `exit`
 * @param brief Short string briefly describing error
 * @param msg Format string elaborating error details
 */
void pspl_error(int exit_code, const char* brief, const char* msg, ...);
/**
 * Raise globally-recognised warning
 *
 * @param brief Short string briefly describing warning
 * @param msg Format string elaborating warning details
 */
void pspl_warn(const char* brief, const char* msg, ...);
#endif


#pragma mark Malloc Zone Context

/**
 * Malloc context type 
 */
/* Malloc context */
typedef struct _malloc_context {
    unsigned int object_num;
    unsigned int object_cap;
    void** object_arr;
} pspl_malloc_context_t;

/**
 * Init Malloc Context for tracking allocated objects
 *
 * @param context Context object to populate
 */
void pspl_malloc_context_init(pspl_malloc_context_t* context);

/**
 * Destroy Malloc Context and free tracked memory objects with it
 *
 * @param context Context object to destroy
 */
void pspl_malloc_context_destroy(pspl_malloc_context_t* context);

/**
 * Allocate memory object and track within context
 *
 * @param context Context object to add memory object within
 * @return Newly-allocated memory object pointer
 */
void* pspl_malloc_malloc(pspl_malloc_context_t* context, size_t size);

/**
 * Free memory object from within context
 *
 * @param context Context object to free memory object from
 * @param ptr Previously allocated memory object
 */
void pspl_malloc_free(pspl_malloc_context_t* context, void* ptr);


#pragma mark Hash Stuff

/**
 * PSPL stored hash type 
 */
typedef union {
    uint8_t b[20];
    uint32_t w[5];
} pspl_hash;

/**
 * Compare the contents of two hash objects
 *
 * @param a First Hash
 * @param b Second Hash
 * @return 0 if identical, positive or negative otherwise
 */
static inline int pspl_hash_cmp(const pspl_hash* a, const pspl_hash* b) {
    return memcmp(a, b, sizeof(pspl_hash));
}

/**
 * Copy hash to another location in memory
 *
 * @param dest Hash location to copy to
 * @param src Hash location to copy from
 */
static inline void pspl_hash_cpy(pspl_hash* dest, const pspl_hash* src) {
    memcpy(dest, src, sizeof(pspl_hash));
}

/**
 * Length of string-buffer filled by `pspl_hash_fmt`
 */
#define PSPL_HASH_STRING_LEN 41

/**
 * Write hash data as string
 *
 * @param out String-buffer to fill (allocated to length of `PSPL_HASH_STRING_LEN`)
 * @param hash Hash object to be read
 */
extern void pspl_hash_fmt(char* out, const pspl_hash* hash);

/**
 * Parse hash string as hash object
 *
 * @param hash Hash object to be populated
 * @param out String-buffer containing hash as generated by `pspl_hash_fmt`
 */
extern void pspl_hash_parse(pspl_hash* out, const char* hash_str);


/* Runtime-only things */
#ifdef PSPL_RUNTIME

#pragma mark Runtime API

/**
 * Init PSPL Runtime
 *
 * Allocates necessary state for the runtime platform compiled into `pspl-rt`.
 * PSPLP package files may be loaded afterwards.
 *
 * @param platform_out Output pointer that's set with a reference to the 
 *        compiled-in platform metadata structure
 * @return 0 if successful, negative otherwise
 */
int pspl_runtime_init(const pspl_platform_t** platform_out);

/**
 * Shutdown PSPL Runtime
 *
 * Deallocates any objects and archived files owned by the PSPL runtime,
 * closes any open packages, unloads all objects
 */
void pspl_runtime_shutdown();


#pragma mark Packages

/**
 * Package representation type 
 */
typedef struct _pspl_loaded_package pspl_runtime_package_t;


/**
 * Data provider type
 */
typedef struct {
    
    /**< Open handle at path for reading (returning pointer to handle or NULL) */
    int(*open)(void* handle, const char* path);
    
    /**< Close previously opened handle */
    void(*close)(const void* handle);
    
    /**< Tell length of handle */
    size_t(*len)(const void* handle);
    
    /**< Seek to point in handle */
    int(*seek)(const void* handle, size_t seek_set);
    
    /**< Read count of bytes from handle (returning bytes read) */
    size_t(*read)(const void* handle, size_t num_bytes, const void** data_out);
    
} pspl_data_provider_t;


/* Various ways to load a PSPLP */

/**
 * Load a PSPLP package file using `<stdio.h>` routines
 *
 * @param package_path path (absolute or relative to working directory)
 *        expressing location to PSPLP file
 * @param package_out Output pointer conveying package representation
 * @return 0 if successful, or negative otherwise
 */
int pspl_runtime_load_package_file(const char* package_path, const pspl_runtime_package_t** package_out);

/**
 * Load a PSPLP package file using data-providing hook routines provided by application
 *
 * @param package_path path expressing location to PSPLP file
 *        (supplied to 'open' hook)
 * @param data_provider_handle application-allocated handle object for use
 *        with provided hooks
 * @param data_provider_hooks application-populated data structure with hook
 *        implementations for data-handling routines
 * @param package_out Output pointer conveying package representation
 * @return 0 if successful, or negative otherwise
 */
int pspl_runtime_load_package_provider(const char* package_path,
                                       void* data_provider_handle,
                                       const pspl_data_provider_t* data_provider_hooks,
                                       const pspl_runtime_package_t** package_out);

/**
 * Load a PSPLP package file from application-provided memory buffer
 *
 * @param package_data pointer to PSPLP data
 * @param package_len length of PSPLP buffer in memory
 * @param package_out Output pointer conveying package representation
 * @return 0 if successful, or negative otherwise
 */
int pspl_runtime_load_package_membuf(const void* package_data, size_t package_len,
                                     const pspl_runtime_package_t** package_out);

/**
 * Unload PSPL package
 *
 * Frees all allocated objects represented through this package.
 * All extension and platform runtimes will be instructed to unload
 * their instances of the objects.
 *
 * @param package Package representation to unload
 */
void pspl_runtime_unload_package(const pspl_runtime_package_t* package);


#pragma mark PSPLC Objects

/**
 * PSPLC runtime representation (from PSPLC)
 * Holds state information about object during runtime 
 */
typedef struct {
    
    /**< Hash of PSPLC object */
    pspl_hash hash;
    
    /**< Platform-specific shader object
     *   (initialised by load hook, freed by unload hook, bound by bind hook) */
    pspl_platform_shader_object_t native_shader;
    
} pspl_runtime_psplc_t;

/**
 * Count embedded PSPLCs within package
 *
 * @param package Package representation to count from
 * @return PSPLC count
 */
unsigned int pspl_runtime_count_psplcs(const pspl_runtime_package_t* package);

/**
 * Funtion type to enumerate PSPLC representations within package.
 * `psplc_object` provides the complete PSPLC representation
 * Returning negative value from hook will cancel enumeration
 */
typedef int(*pspl_runtime_enumerate_psplc_hook)(const pspl_runtime_psplc_t* psplc_object);

/**
 * Enumerate embedded PSPLCs within package
 *
 * @param package Package representation to enumerate from
 * @param hook Function to call for each PSPLC
 */
void pspl_runtime_enumerate_psplcs(const pspl_runtime_package_t* package,
                                   pspl_runtime_enumerate_psplc_hook hook);

/**
 * Get PSPLC representation from key string and optionally perform retain
 *
 * @param key Key-string to hash and use to look up PSPLC representation
 * @param retain If non-zero, the PSPLC representation will have internal 
 *        reference count set to 1 when found
 * @return PSPLC representation (or NULL if not available)
 */
const pspl_runtime_psplc_t* pspl_runtime_get_psplc_from_key(const char* key, int retain);

/**
 * Get PSPLC representation from hash and optionally perform retain 
 *
 * @param hash Hash to use to look up PSPLC representation
 * @param retain If non-zero, the PSPLC representation will have internal
 *        reference count set to 1 when found
 * @return PSPLC representation (or NULL if not available)
 */
const pspl_runtime_psplc_t* pspl_runtime_get_psplc_from_hash(pspl_hash* hash, int retain);

/**
 * Increment reference-count of PSPLC representation
 *
 * @param psplc PSPLC representation
 */
void pspl_runtime_retain_psplc(const pspl_runtime_psplc_t* psplc);

/**
 * Decrement reference-count of PSPLC representation
 *
 * @param psplc PSPLC representation
 */
void pspl_runtime_release_psplc(const pspl_runtime_psplc_t* psplc);


#pragma mark Archived Files

/**
 * Archived file object (from PSPLP)
 * Holds state information about file during runtime 
 */
typedef struct {
    
    /**< Hash of archived file */
    pspl_hash hash;
    
    /**< File length and data */
    size_t file_len;
    const void* file_data;
    
} pspl_runtime_arc_file_t;

/**
 * Count archived files within package
 *
 * @param package Package representation to count from
 * @return Archived file count
 */
unsigned int pspl_runtime_count_archived_files(const pspl_runtime_package_t* package);

/**
 * Function type to enumerate archived files within package.
 * `archived_file` provides the complete file representation
 * Returning negative value from hook will cancel enumeration 
 */
typedef int(*pspl_runtime_enumerate_archived_file_hook)(const pspl_runtime_arc_file_t* archived_file);

/**
 * Enumerate archived files within package
 *
 * @param package Package representation to enumerate from
 * @param hook Function to call for each file
 */
void pspl_runtime_enumerate_archived_files(const pspl_runtime_package_t* package,
                                           pspl_runtime_enumerate_archived_file_hook hook);

/**
 * Get archived file from hash and optionally perform retain
 *
 * @param hash Hash to use to look up archived file
 * @param retain If non-zero, the archived file will have internal
 *        reference count set to 1 when found
 * @return File representation (or NULL if not available)
 */
const pspl_runtime_arc_file_t* pspl_runtime_get_archived_file_from_hash(pspl_hash* hash, int retain);

/**
 * Increment reference-count of archived file
 *
 * @param file File representation
 */
void pspl_runtime_retain_archived_file(const pspl_runtime_arc_file_t* file);

/**
 * Decrement reference-count of archived file
 *
 * @param file File representation
 */
void pspl_runtime_release_archived_file(const pspl_runtime_arc_file_t* file);

#endif // PSPL_RUNTIME

/** @} */

#include <PSPL/PSPLToolchainExtension.h>
#include <PSPL/PSPLRuntimeExtension.h>

#endif
