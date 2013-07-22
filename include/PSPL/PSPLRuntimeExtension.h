//
//  PSPLRuntimeExtension.h
//  PSPL
//
//  Created by Jack Andersen on 4/15/13.
//
//

#ifndef PSPL_PSPLRuntimeExtension_h
#define PSPL_PSPLRuntimeExtension_h
#ifdef PSPL_RUNTIME

#include <PSPLExtension.h>
#include <PSPLRuntime.h>

/**
 * @file PSPL/PSPLRuntimeExtension.h
 * @brief Runtime extension API
 * @ingroup PSPL
 * @{
 */

#pragma mark Hook Types

/** 
 * Embedded data object type 
 */
typedef struct {
    size_t object_len;
    void* object_data;
} pspl_data_object_t;

/**
 * Runtime extension/platform hooks
 */
typedef void(*pspl_runtime_shutdown_hook)();
typedef void(*pspl_runtime_load_object_hook)(pspl_runtime_psplc_t* object);
typedef void(*pspl_runtime_unload_object_hook)(pspl_runtime_psplc_t* object);
typedef void(*pspl_runtime_bind_object_hook)(pspl_runtime_psplc_t* object);


#pragma mark Extension/Platform Load API

/* These *must* be called within the `load` or `bind` hook of platform or extension */

/**
 * Get embedded data object for extension by string key (used in hash lookup)
 *
 * * **This routine may only be called within extension/platform `load`, `bind`, and `unload` hooks**
 * * This routine will only lookup objects in *hashed namespace*
 * 
 * @param object Read-only handle identifying embedded PSPLC object
 *        to get data object from
 * @param key Key string to hash and use for data object lookup
 * @param hash_out optional pointer to save computed hash to (useful for caching the hash)
 * @param data_object_out Pointer to app-allocated data object structure
 *        needing to be populated (composing data ptr and length)
 */
int pspl_runtime_get_embedded_data_object_from_key(const pspl_runtime_psplc_t* object,
                                                   const char* key,
                                                   pspl_hash* hash_out,
                                                   pspl_data_object_t* data_object_out);

/**
 * Count embedded object in PSPLC keyed by hash
 *
 * * **This routine may only be called within extension/platform `load`, `bind`, and `unload` hooks**
 * * This routine will only lookup objects in *hashed namespace*
 *
 * @param object Read-only handle identifying embedded PSPLC object
 *        to count objects from
 * @return Count of hash-keyed objects
 */
int pspl_runtime_count_hash_embedded_data_objects(const pspl_runtime_psplc_t* object);

/**
 * Get embedded object for extension by direct hash key (skips string hashing)
 *
 * * **This routine may only be called within extension/platform `load`, `bind`, and `unload` hooks**
 * * This routine will only lookup objects in *hashed namespace*
 *
 * @param object Read-only handle identifying embedded PSPLC object
 *        to get data object from
 * @param hash Hash to use for data object lookup
 * @param data_object_out Pointer to app-allocated data object structure
 *        needing to be populated (composing data ptr and length)
 */
int pspl_runtime_get_embedded_data_object_from_hash(const pspl_runtime_psplc_t* object,
                                                    const pspl_hash* key,
                                                    pspl_data_object_t* data_object_out);

typedef int(*pspl_hash_enumerate_hook)(pspl_data_object_t* data_object, const pspl_hash* key, void* user_ptr);
/**
 * Enumerate hashed embedded objects
 *
 * * **This routine may only be called within extension/platform `load`, `bind`, and `unload` hooks**
 * * This routine will only lookup objects in *hashed namespace*
 *
 * @param object Read-only handle identifying embedded PSPLC object
 *        to get data object from
 * @param hook Function pointer to call for each hash-keyed object.
 *        Returning -1 from hook will cancel enumeration.
 * @param user_ptr Pointer address to pass to hook function
 */
void pspl_runtime_enumerate_hash_embedded_data_objects(const pspl_runtime_psplc_t* object,
                                                       pspl_hash_enumerate_hook hook,
                                                       void* user_ptr);

/**
 * Count embedded objects in PSPLC keyed by integer
 *
 * * **This routine may only be called within extension/platform `load`, `bind`, and `unload` hooks**
 * * This routine will only lookup objects in *integer namespace*
 *
 * @param object Read-only handle identifying embedded PSPLC object
 *        to count objects from
 * @return Count of integer-keyed objects
 */
int pspl_runtime_count_integer_embedded_data_objects(const pspl_runtime_psplc_t* object);

/**
 * Get embedded data object for extension by integer-key (used in integer lookup)
 *
 * * **This routine may only be called within extension/platform `load`, `bind`, and `unload` hooks**
 * * This routine will only lookup objects in *integer namespace*
 *
 * @param object Read-only handle identifying embedded PSPLC object
 *        to get data object from
 * @param index Integer to use for data object lookup
 * @param data_object_out Pointer to app-allocated data object structure
 *        needing to be populated (composing data ptr and length)
 */
int pspl_runtime_get_embedded_data_object_from_integer(const pspl_runtime_psplc_t* object,
                                                       uint32_t key,
                                                       pspl_data_object_t* data_object_out);

typedef int(*pspl_integer_enumerate_hook)(pspl_data_object_t* data_object, uint32_t key, void* user_ptr);
/**
 * Enumerate integer-keyed embedded objects
 *
 * * **This routine may only be called within extension/platform `load`, `bind`, and `unload` hooks**
 * * This routine will only lookup objects in *integer namespace*
 *
 * @param object Read-only handle identifying embedded PSPLC object
 *        to get data object from
 * @param hook Function pointer to call for each integer-keyed object.
 *        Returning -1 from hook will cancel enumeration.
 * @param user_ptr Pointer address to pass to hook function
 */
void pspl_runtime_enumerate_integer_embedded_data_objects(const pspl_runtime_psplc_t* object,
                                                          pspl_integer_enumerate_hook hook,
                                                          void* user_ptr);


/**
 * Set extension-specific user-data pointer for individual PSPLC object
 *
 * * **This routine may only be called within extension/platform `load`, `bind`, and `unload` hooks**
 *
 * @param object Read-only handle identifying embedded PSPLC object
 *        to get user pointer of
 * @param ptr Pointer value to assign for extension PSPLC object
 * @return Negative if unsuccessful
 */
int pspl_runtime_set_extension_user_data_pointer(const pspl_runtime_psplc_t* object, void* ptr);

/**
 * Get extension-specific user-data pointer for individual PSPLC object
 *
 * * **This routine may only be called within extension/platform `load`, `bind`, and `unload` hooks**
 *
 * @param object Read-only handle identifying embedded PSPLC object
 *        to get user pointer of
 * @return Currently set pointer value
 */
void* pspl_runtime_get_extension_user_data_pointer(const pspl_runtime_psplc_t* object);


#pragma mark Main Runtime Extension Structure

/* Main toolchain platform structure */
typedef struct _pspl_runtime_extension {
    
    // All fields are optional and may be set `NULL`
    
    // Hook fields
    
    // Extension init (called once at runtime init)
    int(*init_hook)(const pspl_extension_t* extension);
    
    // Shutdown (called once at runtime shutdown)
    pspl_runtime_shutdown_hook shutdown_hook;
    
    // Object load
    pspl_runtime_load_object_hook load_object_hook;
    
    // Object unload
    pspl_runtime_unload_object_hook unload_object_hook;
    
    // Object bind
    pspl_runtime_bind_object_hook bind_object_hook;
    
} pspl_runtime_extension_t;



#pragma mark Main Runtime Platform Structure (every platform needs one)

/* Main toolchain platform structure */
typedef struct _pspl_runtime_platform {
    
    // All fields are optional and may be set `NULL`
    
    // Hook fields
    
    // Platform init (called once at runtime init)
    int(*init_hook)(const pspl_platform_t* platform);
    
    // Shutdown (called once at runtime shutdown)
    pspl_runtime_shutdown_hook shutdown_hook;
    
    // Object load (platform implementation needs to initialise `native_shader`)
    pspl_runtime_load_object_hook load_object_hook;
    
    // Object unload (platform implementation needs to free `native_shader`)
    pspl_runtime_unload_object_hook unload_object_hook;
    
    // Object bind
    // (platform implementation needs to set GPU into using specified shader object)
    pspl_runtime_bind_object_hook bind_object_hook;
    
} pspl_runtime_platform_t;

/** @} */

#endif // PSPL_RUNTIME
#endif
