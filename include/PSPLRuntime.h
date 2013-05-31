//
//  PSPLRuntime.h
//  PSPL
//
//  Created by Jack Andersen on 5/30/13.
//
//

#ifndef PSPL_PSPLRuntime_h
#define PSPL_PSPLRuntime_h

#ifndef PSPL_RUNTIME
#define PSPL_RUNTIME
#endif

#include <PSPL/PSPLCommon.h>

/**
 * @file PSPLRuntime.h
 * @brief Runtime Public API
 * @defgroup pspl_runtime Runtime Public API
 * @ingroup PSPL
 * @{
 */


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
const pspl_runtime_psplc_t* pspl_runtime_get_psplc_from_key(const pspl_runtime_package_t* package,
                                                            const char* key, int retain);

/**
 * Get PSPLC representation from hash and optionally perform retain
 *
 * @param hash Hash to use to look up PSPLC representation
 * @param retain If non-zero, the PSPLC representation will have internal
 *        reference count set to 1 when found
 * @return PSPLC representation (or NULL if not available)
 */
const pspl_runtime_psplc_t* pspl_runtime_get_psplc_from_hash(const pspl_runtime_package_t* package,
                                                             pspl_hash* hash, int retain);

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
const pspl_runtime_arc_file_t* pspl_runtime_get_archived_file_from_hash(const pspl_runtime_package_t* package,
                                                                        pspl_hash* hash, int retain);

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

/** @} */

#endif
