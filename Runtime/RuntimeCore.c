//
//  RuntimeCore.c
//  PSPL
//
//  Created by Jack Andersen on 5/25/13.
//
//

#include <stdio.h>
#include <PSPL/PSPLExtension.h>

/* Active runtime platform */
extern const pspl_platform_t* pspl_available_platforms[];
#define pspl_runtime_platform pspl_available_platforms[0]


#pragma mark Extension/Platform Load API

/* These *must* be called within the `load` hook of platform or extension */

/* Get embedded data object for extension by key (to hash) */
const pspl_data_object_t* pspl_get_embedded_data_object_from_key(pspl_runtime_psplc_rep_t* object,
                                                                 const char* key) {
    
}

/* Get embedded object for extension by hash */
const pspl_data_object_t* pspl_get_embedded_data_object_from_hash(pspl_runtime_psplc_rep_t* object,
                                                                  pspl_hash* hash) {
    
}

/* Get embedded object for extension by index */
const pspl_data_object_t* pspl_get_embedded_data_object_from_index(pspl_runtime_psplc_rep_t* object,
                                                                   uint32_t index) {
    
}

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
int pspl_runtime_init(const pspl_platform_t** platform_out) {
    
}

/**
 * Shutdown PSPL Runtime
 *
 * Deallocates any objects and archived files owned by the PSPL runtime,
 * closes any open packages, unloads all objects
 */
void pspl_runtime_shutdown() {
    
}


#pragma mark Packages

/**
 * Package representation type
 */
struct _pspl_loaded_package {
    
};





/* Various ways to load a PSPLP */

/**
 * Load a PSPLP package file using `<stdio.h>` routines
 *
 * @param package_path path (absolute or relative to working directory)
 *        expressing location to PSPLP file
 * @param package_out Output pointer conveying package representation
 * @return 0 if successful, or negative otherwise
 */
int pspl_runtime_load_package_file(const char* package_path, const pspl_runtime_package_rep_t** package_out) {
    
}

/**
 * Load a PSPLP package file using data-providing hook routines provided by application
 *
 * @param package_path path expressing location to PSPLP file
 *        (supplied to 'open' hook)
 * @param data_provider application-populated data structure with hook
 *        implementations for data-handling routines
 * @param package_out Output pointer conveying package representation
 * @return 0 if successful, or negative otherwise
 */
int pspl_runtime_load_package_provider(const char* package_path, const pspl_data_provider_t* data_provider,
                                       const pspl_runtime_package_rep_t** package_out) {
    
}

/**
 * Load a PSPLP package file from application-provided memory buffer
 *
 * @param package_data pointer to PSPLP data
 * @param package_len length of PSPLP buffer in memory
 * @param package_out Output pointer conveying package representation
 * @return 0 if successful, or negative otherwise
 */
int pspl_runtime_load_package_membuf(const void* package_data, size_t package_len,
                                     const pspl_runtime_package_rep_t** package_out) {
    
}

/**
 * Unload PSPL package
 *
 * Frees all allocated objects represented through this package.
 * All extension and platform runtimes will be instructed to unload
 * their instances of the objects.
 *
 * @param package Package representation to unload
 */
void pspl_runtime_unload_package(const pspl_runtime_package_rep_t* package) {
    
}


#pragma mark PSPLC Objects

/**
 * Count embedded PSPLCs within package
 *
 * @param package Package representation to count from
 * @return PSPLC count
 */
unsigned int pspl_runtime_count_psplcs(const pspl_runtime_package_rep_t* package) {
    
}

/**
 * Enumerate embedded PSPLCs within package
 *
 * @param package Package representation to enumerate from
 * @param hook Function to call for each PSPLC
 */
void pspl_runtime_enumerate_psplcs(const pspl_runtime_package_rep_t* package,
                                   pspl_runtime_enumerate_psplc_hook hook) {
    
}

/**
 * Get PSPLC representation from key string and optionally perform retain
 *
 * @param key Key-string to hash and use to look up PSPLC representation
 * @param retain If non-zero, the PSPLC representation will have internal
 *        reference count set to 1 when found
 * @return PSPLC representation (or NULL if not available)
 */
const pspl_runtime_psplc_rep_t* pspl_runtime_get_psplc_from_key(const char* key, int retain) {
    
}

/**
 * Get PSPLC representation from hash and optionally perform retain
 *
 * @param hash Hash to use to look up PSPLC representation
 * @param retain If non-zero, the PSPLC representation will have internal
 *        reference count set to 1 when found
 * @return PSPLC representation (or NULL if not available)
 */
const pspl_runtime_psplc_rep_t* pspl_runtime_get_psplc_from_hash(pspl_hash* hash, int retain) {
    
}

/**
 * Increment reference-count of PSPLC representation
 *
 * @param psplc PSPLC representation
 */
void pspl_runtime_retain_psplc(const pspl_runtime_psplc_rep_t* psplc) {
    
}

/**
 * Decrement reference-count of PSPLC representation
 *
 * @param psplc PSPLC representation
 */
void pspl_runtime_release_psplc(const pspl_runtime_psplc_rep_t* psplc) {
    
}


#pragma mark Archived Files

/**
 * Count archived files within package
 *
 * @param package Package representation to count from
 * @return Archived file count
 */
unsigned int pspl_runtime_count_archived_files(const pspl_runtime_package_rep_t* package) {
    
}

/**
 * Enumerate archived files within package
 *
 * @param package Package representation to enumerate from
 * @param hook Function to call for each file
 */
void pspl_runtime_enumerate_archived_files(const pspl_runtime_package_rep_t* package,
                                           pspl_runtime_enumerate_archived_file_hook hook) {
    
}

/**
 * Get archived file from hash and optionally perform retain
 *
 * @param hash Hash to use to look up archived file
 * @param retain If non-zero, the archived file will have internal
 *        reference count set to 1 when found
 * @return File representation (or NULL if not available)
 */
const pspl_runtime_arc_file_rep_t* pspl_get_archived_file_from_hash(pspl_hash* hash, int retain) {
    
}

/**
 * Increment reference-count of archived file
 *
 * @param file File representation
 */
void pspl_runtime_retain_archived_file(const pspl_runtime_arc_file_rep_t* file) {
    
}

/**
 * Decrement reference-count of archived file
 *
 * @param file File representation
 */
void pspl_runtime_release_archived_file(const pspl_runtime_arc_file_rep_t* file) {
    
}


