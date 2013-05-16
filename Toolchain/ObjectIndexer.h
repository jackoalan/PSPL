//
//  ObjectIndexer.h
//  PSPL
//
//  Created by Jack Andersen on 5/13/13.
//
//

#ifndef PSPL_ObjectIndexer_h
#define PSPL_ObjectIndexer_h
#ifdef PSPL_INTERNAL

#include <stdint.h>
#include <PSPL/PSPLExtension.h>

/* This API is used to maintain an index context for gathering
 * enbedded objects and file stubs. It coordinates with the
 * compiler and compiler extensions to do file-dependency calculations
 * and triggers conversion routines. Key and file-content hashing
 * is also performed by this API. */

/* PSPLC embedded object/file stub index structure */
typedef struct {
    
    // "Definer" (for error reporting)
    const char* definer;
    
    // "Owner" extension (unused for stubs)
    const pspl_extension_t* owner_ext;
    
    // Object index/hash
    union {
        pspl_hash object_hash;
        uint32_t object_index;
    };
    
    // Platform availability bitfield
    uint32_t platform_availability_bits;
    
    // Embedded object length and buffer
    // (unused for file stubs)
    size_t object_len;
    const void* object_little_data;
    const void* object_big_data;
    
    // May also be used to reference file stub source path
    // (unused for objects)
    const char* stub_source_path;
    
    // Source path extension
    const char* stub_source_path_ext;

    
} pspl_indexer_entry_t;

/* Indexer context type */
typedef struct {
    
    // Using which extensions
    unsigned int ext_count;
    const pspl_extension_t** ext_array;
    
    // Using which platforms
    unsigned int plat_count;
    const pspl_runtime_platform_t** plat_array;
    
    // Embedded PSPLC hash objects
    unsigned int h_objects_count;
    unsigned int h_objects_cap;
    pspl_indexer_entry_t** h_objects_array;
    
    // Embedded PSPLC integer objects
    unsigned int i_objects_count;
    unsigned int i_objects_cap;
    pspl_indexer_entry_t** i_objects_array;
    
    // Embedded PSPLC File stubs
    unsigned int stubs_count;
    unsigned int stubs_cap;
    pspl_indexer_entry_t** stubs_array;
    
} pspl_indexer_context_t;

#include "Driver.h"

/* Initialise indexer context */
void pspl_indexer_init(pspl_indexer_context_t* ctx,
                       unsigned int max_extension_count,
                       unsigned int max_platform_count);

/* Augment indexer context with stub entries
 * from an existing PSPLC file */
void pspl_indexer_psplc_stub_augment(pspl_indexer_context_t* ctx, pspl_toolchain_driver_psplc_t* existing_psplc);

/* Augment indexer context with embedded hash-indexed object */
void pspl_indexer_hash_object_augment(pspl_indexer_context_t* ctx, const pspl_extension_t* owner,
                                      const pspl_runtime_platform_t** plats, const char* key,
                                      const void* little_data, const void* big_data, size_t data_len,
                                      pspl_toolchain_driver_source_t* definer);

/* Augment indexer context with embedded integer-indexed object */
void pspl_indexer_integer_object_augment(pspl_indexer_context_t* ctx, const pspl_extension_t* owner,
                                         const pspl_runtime_platform_t** plats, uint32_t key,
                                         const void* little_data, const void* big_data, size_t data_len,
                                         pspl_toolchain_driver_source_t* definer);

/* Augment indexer context with file-stub 
 * (triggering conversion hook if provided and output is outdated) */
void pspl_indexer_stub_file_augment(pspl_indexer_context_t* ctx,
                                    const pspl_runtime_platform_t** plats, const char* path_in,
                                    const char* path_ext_in,
                                    pspl_converter_file_hook converter_hook, uint8_t move_output,
                                    pspl_hash** hash_out,
                                    pspl_toolchain_driver_source_t* definer);
void pspl_indexer_stub_membuf_augment(pspl_indexer_context_t* ctx,
                                      const pspl_runtime_platform_t** plats, const char* path_in,
                                      const char* path_ext_in,
                                      pspl_converter_membuf_hook converter_hook,
                                      pspl_hash** hash_out,
                                      pspl_toolchain_driver_source_t* definer);

#endif // PSPL_INTERNAL
#endif
