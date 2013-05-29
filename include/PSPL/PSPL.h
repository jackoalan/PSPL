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
 * @ingroup PSPL
 * @{
 */

/* Endianness enumerations */
#define PSPL_UNSPEC_ENDIAN 0
#define PSPL_LITTLE_ENDIAN 1
#define PSPL_BIG_ENDIAN    2
#define PSPL_BI_ENDIAN     3

/* PSPL stored hash type */
typedef union {
    uint8_t b[20];
    uint32_t w[5];
} pspl_hash;
static inline int pspl_hash_cmp(const pspl_hash* a, const pspl_hash* b) {
    return memcmp(a, b, sizeof(pspl_hash));
}
static inline void pspl_hash_cpy(pspl_hash* dest, const pspl_hash* src) {
    memcpy(dest, src, sizeof(pspl_hash));
}
#define PSPL_HASH_STRING_LEN 41
extern void pspl_hash_fmt(char* out, const pspl_hash* hash);
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
int pspl_init(const pspl_platform_t** platform_out);

/**
 * Shutdown PSPL Runtime
 *
 * Deallocates any objects and archived files owned by the PSPL runtime,
 * closes any open packages, unloads all objects
 */
void pspl_shutdown();


#pragma mark Packages

/* Package handle type */
typedef struct _pspl_loaded_package pspl_package_t;

/* Data provider type */
typedef struct {
    
    // Open handle at path for reading (returning pointer to handle or NULL)
    const void*(*open)(const char* path);
    
    // Close previously opened handle
    void(*close)(const void* handle);
    
    // Tell length of handle
    size_t(*len)(const void* handle);
    
    // Seek to point in handle
    int(*seek)(const void* handle, size_t seek_set);
    
    // Read count of bytes from handle (returning bytes read)
    size_t(*read)(const void* handle, size_t num_bytes, void* data_out);
    
} pspl_data_provider_t;

/* Load PSPL package */
int pspl_load_package_file(const char* package_path, const pspl_package_t** package_out);
int pspl_load_package_provider(const char* package_path, const pspl_data_provider_t* data_provider,
                               const pspl_package_t** package_out);
int pspl_load_package_membuf(const void* package_data, size_t package_len,
                             const pspl_package_t** package_out);

/* Unload PSPL package */
void pspl_unload_package(const pspl_package_t* package);


#pragma mark PSPLC Objects

/* PSPLC runtime object (from PSPLC) 
 * Holds state information about object during runtime */
typedef struct {
    
    // Hash of PSPLC object
    pspl_hash hash;
    
    // Platform-specific shader object
    // (initialised by load hook, freed by unload hook, bound by bind hook)
    pspl_platform_shader_object_t native_shader;
    
    // Opaque object pointer used by PSPL's runtime internals
    const void* pspl_internals;
    
} psplc_object_t;

/* Count PSPLC objects within package */
unsigned int pspl_count_psplc_objects(const pspl_package_t* package);

/* Enumerate PSPLC objects within package 
 * returning negative value from hook will cancel enumeration */
typedef int(*pspl_enumerate_psplc_object_hook)(const psplc_object_t* psplc_object);
void pspl_enumerate_psplc_objects(const pspl_package_t* package,
                                  pspl_enumerate_psplc_object_hook hook);

/* Get PSPLC object from key string and optionally perform retain */
const psplc_object_t* pspl_get_psplc_object_from_key(const char* key, int retain);

/* Get PSPLC object from hash and optionally perform retain */
const psplc_object_t* pspl_get_psplc_object_from_hash(pspl_hash* hash, int retain);

/* Retain/release PSPLC object */
void pspl_retain_psplc_object(const psplc_object_t* psplc_object);
void pspl_release_psplc_object(const psplc_object_t* psplc_object);


#pragma mark Archived Files

/* Archived file object (from PSPLP)
 * Holds state information about file during runtime */
typedef struct {
    
    // Hash of archived file
    pspl_hash hash;
    
    // File length and data
    size_t file_len;
    const void* file_data;
    
    // Opaque object pointer used by PSPL's runtime internals
    const void* pspl_internals;
    
} pspl_archived_file_t;

/* Count archived files within package */
unsigned int pspl_count_archived_files(const pspl_package_t* package);

/* Enumerate archived files within package
 * returning negative value from hook will cancel enumeration */
typedef int(*pspl_enumerate_archived_file_hook)(const pspl_archived_file_t* archived_file);
void pspl_enumerate_archived_files(const pspl_package_t* package,
                                   pspl_enumerate_archived_file_hook hook);

/* Get archived file from hash and optionally perform retain */
const pspl_archived_file_t* pspl_get_archived_file_from_hash(pspl_hash* hash, int retain);

/* Retain/release archived file */
void pspl_retain_archived_file(const pspl_archived_file_t* archived_file);
void pspl_release_archived_file(const pspl_archived_file_t* archived_file);

#endif // PSPL_RUNTIME

/** @} */

#include <PSPL/PSPLToolchainExtension.h>
#include <PSPL/PSPLRuntimeExtension.h>

#endif
