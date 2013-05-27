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
const pspl_data_object_t* pspl_get_embedded_data_object_from_key(psplc_object_t* object,
                                                                 const char* key) {
    
}

/* Get embedded object for extension by hash */
const pspl_data_object_t* pspl_get_embedded_data_object_from_hash(psplc_object_t* object,
                                                                  pspl_hash* hash) {
    
}

/* Get embedded object for extension by index */
const pspl_data_object_t* pspl_get_embedded_data_object_from_index(psplc_object_t* object,
                                                                   uint32_t index) {
    
}

#pragma mark Runtime API

/* Init PSPL Runtime */
int pspl_init() {
    
}

/* Shutdown PSPL Runtime */
void pspl_shutdown() {
    
}


#pragma mark Packages

/* Package handle type */
struct _pspl_loaded_package {
    
};


/* Load PSPL package */
int pspl_load_package_file(const char* package_path, const pspl_package_t** package_out) {
    
}
int pspl_load_package_provider(const char* package_path, const pspl_data_provider_t* data_provider,
                               const pspl_package_t** package_out) {
    
}
int pspl_load_package_membuf(const void* package_data, size_t package_len,
                             const pspl_package_t** package_out) {
    
}

/* Unload PSPL package */
void pspl_unload_package(const pspl_package_t* package) {
    
}


#pragma mark PSPLC Objects

/* Count PSPLC objects within package */
unsigned int pspl_count_psplc_objects(const pspl_package_t* package) {
    
}

/* Enumerate PSPLC objects within package
 * returning negative value from hook will cancel enumeration */
void pspl_enumerate_psplc_objects(const pspl_package_t* package,
                                  pspl_enumerate_psplc_object_hook hook) {
    
}

/* Get PSPLC object from key string and optionally perform retain */
const psplc_object_t* pspl_get_psplc_object_from_key(const char* key, int retain) {
    
}

/* Get PSPLC object from hash and optionally perform retain */
const psplc_object_t* pspl_get_psplc_object_from_hash(pspl_hash* hash, int retain) {
    
}

/* Retain/release PSPLC object */
void pspl_retain_psplc_object(const psplc_object_t* psplc_object) {
    
}
void pspl_release_psplc_object(const psplc_object_t* psplc_object) {
    
}


#pragma mark Archived Files

/* Count archived files within package */
unsigned int pspl_count_archived_files(const pspl_package_t* package) {
    
}

/* Enumerate archived files within package
 * returning negative value from hook will cancel enumeration */
void pspl_enumerate_archived_files(const pspl_package_t* package,
                                   pspl_enumerate_archived_file_hook hook) {
    
}

/* Get archived file from hash and optionally perform retain */
const pspl_archived_file_t* pspl_get_archived_file_from_hash(pspl_hash* hash, int retain) {
    
}

/* Retain/release archived file */
void pspl_retain_archived_file(const pspl_archived_file_t* archived_file) {
    
}
void pspl_release_archived_file(const pspl_archived_file_t* archived_file) {
    
}


