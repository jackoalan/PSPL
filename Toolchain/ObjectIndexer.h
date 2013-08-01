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
#include <stdio.h>
#include <PSPLExtension.h>

/* This API is used to maintain an index context for gathering
 * enbedded objects and file stubs. It coordinates with the
 * compiler and compiler extensions to do file-dependency calculations
 * and triggers conversion routines. Key and file-content hashing
 * is also performed by this API. */

/* PSPLC embedded object/file stub index structure */
typedef struct {
    
    // Parent indexer
    struct _pspl_indexer_context* parent;
    
    union {
        // "Owner" extension (unused for stubs)
        const pspl_extension_t* owner_ext;
        // "Owner" platform (for platform data objects)
        const pspl_platform_t* owner_plat;
    };
    
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
    size_t object_padding;
    uint32_t object_off;
    const void* object_little_data;
    const void* object_big_data;
    
    // May also be used to reference file stub source path
    // (unused for objects)
    const char* stub_source_path;
    
    // Source path extension
    const char* stub_source_path_ext;

    // Indirectly used offset variables for PSPLP
    // packager during file write (volatile)
    char file_path[MAXPATHLEN];
    uint32_t file_off;
    uint32_t file_padding;
    
} pspl_indexer_entry_t;

/* PSPLC Indexer context type */
typedef struct _pspl_indexer_context {
    
    // "Definer" (for error reporting)
    const char* definer;
    
    // PSPLC name hash
    pspl_hash psplc_hash;
    
    // Using which extensions
    unsigned int ext_count;
    const pspl_extension_t** ext_array;
    
    // Using which platforms
    unsigned int plat_count;
    const pspl_platform_t** plat_array;
    
    // Embedded PSPLC hash objects
    unsigned int h_objects_count;
    unsigned int h_objects_cap;
    pspl_indexer_entry_t** h_objects_array;
    
    // Embedded PSPLC integer objects
    unsigned int i_objects_count;
    unsigned int i_objects_cap;
    pspl_indexer_entry_t** i_objects_array;
    
    // Embedded PLATFORM PSPLC hash objects
    unsigned int ph_objects_count;
    unsigned int ph_objects_cap;
    pspl_indexer_entry_t** ph_objects_array;
    
    // Embedded PLATFORM PSPLC integer objects
    unsigned int pi_objects_count;
    unsigned int pi_objects_cap;
    pspl_indexer_entry_t** pi_objects_array;
    
    // Embedded PSPLC File stubs
    unsigned int stubs_count;
    unsigned int stubs_cap;
    pspl_indexer_entry_t** stubs_array;
    
    // Indirectly used offset variables for PSPLP
    // packager during file write (volatile)
    uint32_t extension_obj_base_off;
    uint32_t extension_obj_array_off;
    uint32_t extension_obj_array_padding;
    uint32_t extension_obj_data_off;
    
    uint32_t extension_obj_blobs_len;
    
} pspl_indexer_context_t;

/* This type is castable from `pspl_indexer_context_t` *and* `pspl_packager_context_t` */
typedef struct {
    
    // Using which extensions
    unsigned int ext_count;
    const pspl_extension_t** ext_array;
    
    // Using which platforms
    unsigned int plat_count;
    const pspl_platform_t** plat_array;
    
} pspl_indexer_globals_t;


#include "Driver.h"

/* Initialise indexer context */
void pspl_indexer_init(pspl_indexer_context_t* ctx,
                       unsigned int max_extension_count,
                       unsigned int max_platform_count);

/* Augment indexer context with stub entries
 * from an existing PSPLC file */
void pspl_indexer_psplc_stub_augment(pspl_indexer_context_t* ctx,
                                     pspl_toolchain_driver_psplc_t* existing_psplc);

/* Augment indexer context with embedded hash-indexed object */
void pspl_indexer_hash_object_augment(pspl_indexer_context_t* ctx, const pspl_extension_t* owner,
                                      const pspl_platform_t** plats, const char* key,
                                      const void* little_data, const void* big_data, size_t data_len,
                                      pspl_toolchain_driver_source_t* definer);

/* Augment indexer context with embedded integer-indexed object */
void pspl_indexer_integer_object_augment(pspl_indexer_context_t* ctx, const pspl_extension_t* owner,
                                         const pspl_platform_t** plats, uint32_t key,
                                         const void* little_data, const void* big_data, size_t data_len,
                                         pspl_toolchain_driver_source_t* definer);

/* Augment indexer context with embedded hash-indexed platform object */
void pspl_indexer_platform_hash_object_augment(pspl_indexer_context_t* ctx, const pspl_platform_t* owner,
                                               const char* key, const void* little_data,
                                               const void* big_data, size_t data_len,
                                               pspl_toolchain_driver_source_t* definer);

/* Augment indexer context with embedded integer-indexed platform object */
void pspl_indexer_platform_integer_object_augment(pspl_indexer_context_t* ctx, const pspl_platform_t* owner,
                                                  uint32_t key, const void* little_data,
                                                  const void* big_data, size_t data_len,
                                                  pspl_toolchain_driver_source_t* definer);

/* Augment indexer context with file-stub 
 * (triggering conversion hook if provided and output is outdated) */
void pspl_indexer_stub_file_augment(pspl_indexer_context_t* ctx,
                                    const pspl_platform_t** plats, const char* path_in,
                                    const char* path_ext_in,
                                    pspl_converter_file_hook converter_hook, uint8_t move_output,
                                    void* user_ptr,
                                    pspl_hash** hash_out,
                                    pspl_toolchain_driver_source_t* definer);
void pspl_indexer_stub_membuf_augment(pspl_indexer_context_t* ctx,
                                      const pspl_platform_t** plats, const char* path_in,
                                      const char* path_ext_in,
                                      pspl_converter_membuf_hook converter_hook,
                                      void* user_ptr,
                                      pspl_hash** hash_out,
                                      pspl_toolchain_driver_source_t* definer);

/* Translates local per-psplc bits to global per-psplp bits */
uint32_t union_plat_bits(pspl_indexer_globals_t* globals,
                         pspl_indexer_context_t* locals, uint32_t bits);

/* Write out bare PSPLC object (separate for direct PSPLP writing) */
void pspl_indexer_write_psplc_bare(pspl_indexer_context_t* ctx,
                                   uint8_t psplc_endianness,
                                   pspl_indexer_globals_t* globals,
                                   FILE* psplc_file_out);

/* Write out to PSPLC file */
void pspl_indexer_write_psplc(pspl_indexer_context_t* ctx,
                              uint8_t default_endian,
                              FILE* psplc_file_out);


#endif // PSPL_INTERNAL
#endif
