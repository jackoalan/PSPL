//
//  PSPLRuntime.h
//  PSPL
//
//  Created by Jack Andersen on 5/30/13.
//
//

#ifndef PSPL_PSPLRuntime_h
#define PSPL_PSPLRuntime_h

/**
 * @file PSPLRuntime.h
 * @brief Runtime Public API
 * @defgroup pspl_runtime Runtime Public API
 * @ingroup PSPL
 * @{
 */

#include <stdio.h>
#include <PSPL/PSPLCommon.h>
#include <PSPL/PSPLRuntimeThreads.h>


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
 * Opaque duplicate data provider handle 
 */
#define MAX_DATA_PROVIDER_HANDLE_SZ 32
typedef uint8_t pspl_dup_data_provider_handle_t[MAX_DATA_PROVIDER_HANDLE_SZ];

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
    
    /**< Tell current point in handle */
    size_t(*tell)(const void* handle);
    
    /**< Read count of bytes from handle (returning bytes read) */
    size_t(*read)(const void* handle, size_t num_bytes, void** data_out);
    
    /**< Read count of bytes from handle into already-allocated buffer (returning bytes read) */
    size_t(*read_direct)(const void* handle, size_t num_bytes, void* data_buf);
    
    
    /**< Duplicate data provider handle (for potentially multi-threaded loading scenarios) 
     * Used by PSPL's own `access_archived_file` routine */
    void(*duplicate_handle)(pspl_dup_data_provider_handle_t* dup_handle, const void* handle);
    
    /**< Destroy duplicate data provider handle */
    void(*destroy_duplicate_handle)(pspl_dup_data_provider_handle_t* dup_handle);
    
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
int pspl_runtime_load_package_membuf(void* package_data, size_t package_len,
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
    const pspl_hash hash;
    
    /**< Parent package */
    const pspl_runtime_package_t* parent;
    
#   ifdef PSPL_RUNTIME
    /**< Platform-specific shader object
     *   (initialised by load hook, freed by unload hook, bound by bind hook) */
    pspl_platform_shader_object_t native_shader;
#   endif
    
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
typedef int(*pspl_runtime_enumerate_psplc_hook)(pspl_runtime_psplc_t* psplc_object);

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

/**
 * Bind PSPLC representation to GPU (implicitly retains if unloaded)
 *
 * @param psplc PSPLC representation
 */
void pspl_runtime_bind_psplc(const pspl_runtime_psplc_t* psplc);


#pragma mark Archived Files

/**
 * Archived file object (from PSPLP)
 * Holds state information about file during runtime
 */
typedef struct {
    
    /**< Hash of archived file */
    const pspl_hash hash;
    
    /**< Parent package */
    const pspl_runtime_package_t* parent;
    
    /**< File length and data */
    size_t file_len;
    void* file_data;
    
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
                                                                        const pspl_hash* hash, int retain);

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

/**
 * Create duplicate, pre-seeked FILE pointer and length of archived file
 *
 * An advanced API to bypass the reference-counted loading mechanism of
 * pspl-rt and receive a FILE pointer ready to load data from disk directly
 *
 * The handle is duplicated so that it may be used from a different thread (if need be)
 *
 * @param file Archived file object
 * @param provider_hooks_out Hook structure used to access data
 * @param provider_handle_out File instance containing requested data (pre-seeked)
 * @param len_out Length of file record
 * @return 0 if successful, otherwise negative
 */
int pspl_runtime_access_archived_file(const pspl_runtime_arc_file_t* file,
                                      const pspl_data_provider_t** provider_hooks_out,
                                      pspl_dup_data_provider_handle_t* provider_handle_out,
                                      size_t* len_out);

/**
 * Destroy duplicate, pre-seeked FILE pointer
 *
 * This should be called sometime after each `pspl_runtime_access_archived_file`
 *
 * @param provider_hooks Hook structure used to access data
 * @param provider_handle File instance containing requested data
 */
#define pspl_runtime_unaccess_archived_file(provider_hooks, provider_handle) (provider_hooks)->destroy_duplicate_handle(provider_handle)


/** @} */

#endif
