//
//  ObjectIndexer.c
//  PSPL
//
//  Created by Jack Andersen on 5/13/13.
//
//

#define PSPL_INTERNAL
#define PSPL_TOOLCHAIN

#include <stdio.h>
#include <string.h>
#include <PSPLInternal.h>
#include "ObjectIndexer.h"

#define PSPL_INDEXER_INITIAL_CAP 50

/* Initialise indexer context */
void pspl_indexer_init(pspl_indexer_context_t* ctx,
                       unsigned int max_extension_count,
                       unsigned int max_platform_count) {
    
    ctx->ext_count = 0;
    ctx->ext_array = calloc(max_extension_count, sizeof(pspl_extension_t*));
    
    ctx->plat_count = 0;
    ctx->plat_array = calloc(max_platform_count, sizeof(pspl_runtime_platform_t*));
    
    ctx->h_objects_count = 0;
    ctx->h_objects_cap = PSPL_INDEXER_INITIAL_CAP;
    ctx->h_objects_array = calloc(PSPL_INDEXER_INITIAL_CAP, sizeof(pspl_indexer_entry_t*));
    
    ctx->i_objects_count = 0;
    ctx->i_objects_cap = PSPL_INDEXER_INITIAL_CAP;
    ctx->i_objects_array = calloc(PSPL_INDEXER_INITIAL_CAP, sizeof(pspl_indexer_entry_t*));
    
    ctx->stubs_count = 0;
    ctx->stubs_cap = PSPL_INDEXER_INITIAL_CAP;
    ctx->stubs_array = calloc(PSPL_INDEXER_INITIAL_CAP, sizeof(pspl_indexer_entry_t*));
    
}

/* Platform array from PSPLC availability bits */
static void __plat_array_from_bits(pspl_runtime_platform_t* plats_out,
                                   pspl_toolchain_driver_psplc_t* psplc,
                                   uint32_t bits) {
    
}

/* Augment indexer context with embedded hash-indexed object (already hashed) */
static void __pspl_indexer_keyhash_object_augment(pspl_indexer_context_t* ctx, const pspl_extension_t* owner,
                                                  const pspl_runtime_platform_t* plats, pspl_hash* key_hash,
                                                  const void* little_data, const void* big_data, size_t data_len) {
    
}

/* Augment indexer context with stub entries
 * from an existing PSPLC file */
void pspl_indexer_psplc_stub_augment(pspl_indexer_context_t* ctx, pspl_toolchain_driver_psplc_t* existing_psplc) {
    
    int i,j;
    pspl_header_t* pspl_head = (pspl_header_t*)existing_psplc->psplc_data;
    
    // First, our PSPLC head
    pspl_psplc_header_t* psplc_head_little = NULL;
    pspl_psplc_header_t* psplc_head_big = NULL;
#   if defined(__LITTLE_ENDIAN__)
    if (pspl_head->endian_flags == PSPL_LITTLE_ENDIAN) {
        psplc_head_little = (pspl_psplc_header_t*)(existing_psplc->psplc_data + sizeof(pspl_header_t));
    } else if (pspl_head->endian_flags == PSPL_BIG_ENDIAN) {
        psplc_head_big = (pspl_psplc_header_t*)(existing_psplc->psplc_data + sizeof(pspl_header_t));
        pspl_psplc_header_t swapped_header = {
            .tier1_object_array_count = swap_uint32(psplc_head_big->tier1_object_array_count),
            .tier1_object_array_off = swap_uint32(psplc_head_big->tier1_object_array_off),
            .file_stub_count = swap_uint32(psplc_head_big->file_stub_count),
            .file_stub_array_off = swap_uint32(psplc_head_big->file_stub_array_off)
        };
        memcpy(&swapped_header.psplc_object_hash, psplc_head_little->psplc_object_hash, sizeof(pspl_hash));
        psplc_head_big = &swapped_header;
    } else if (pspl_head->endian_flags == PSPL_BI_ENDIAN) {
        psplc_head_little = &((pspl_psplc_header_bi_t*)(existing_psplc->psplc_data + sizeof(pspl_header_t)))->little;
        psplc_head_big = &((pspl_psplc_header_bi_t*)(existing_psplc->psplc_data + sizeof(pspl_header_t)))->big;
        pspl_psplc_header_t swapped_header = {
            .tier1_object_array_count = swap_uint32(psplc_head_big->tier1_object_array_count),
            .tier1_object_array_off = swap_uint32(psplc_head_big->tier1_object_array_off),
            .file_stub_count = swap_uint32(psplc_head_big->file_stub_count),
            .file_stub_array_off = swap_uint32(psplc_head_big->file_stub_array_off)
        };
        memcpy(&swapped_header.psplc_object_hash, psplc_head_big->psplc_object_hash, sizeof(pspl_hash));
        psplc_head_big = &swapped_header;
    }
#   elif defined(__BIG_ENDIAN__)
    if (pspl_head->endian_flags == PSPL_LITTLE_ENDIAN) {
        psplc_head_little = (pspl_psplc_header_t*)(existing_psplc->psplc_data + sizeof(pspl_header_t));
        pspl_psplc_header_t swapped_header = {
            .tier1_object_array_count = swap_uint32(psplc_head_little->tier1_object_array_count),
            .tier1_object_array_off = swap_uint32(psplc_head_little->tier1_object_array_off),
            .file_stub_count = swap_uint32(psplc_head_little->file_stub_count),
            .file_stub_array_off = swap_uint32(psplc_head_little->file_stub_array_off)
        };
        memcpy(&swapped_header.psplc_object_hash, psplc_head_little->psplc_object_hash, sizeof(pspl_hash));
        psplc_head_little = &swapped_header;
    } else if (pspl_head->endian_flags == PSPL_BIG_ENDIAN) {
        psplc_head_big = (pspl_psplc_header_t*)(existing_psplc->psplc_data + sizeof(pspl_header_t));
    } else if (pspl_head->endian_flags == PSPL_BI_ENDIAN) {
        psplc_head_big = &((pspl_psplc_header_bi_t*)(existing_psplc->psplc_data + sizeof(pspl_header_t)))->big;
        psplc_head_little = &((pspl_psplc_header_bi_t*)(existing_psplc->psplc_data + sizeof(pspl_header_t)))->little;
        pspl_psplc_header_t swapped_header = {
            .tier1_object_array_count = swap_uint32(psplc_head_little->tier1_object_array_count),
            .tier1_object_array_off = swap_uint32(psplc_head_little->tier1_object_array_off),
            .file_stub_count = swap_uint32(psplc_head_little->file_stub_count),
            .file_stub_array_off = swap_uint32(psplc_head_little->file_stub_array_off)
        };
        memcpy(&swapped_header.psplc_object_hash, psplc_head_little->psplc_object_hash, sizeof(pspl_hash));
        psplc_head_little = &swapped_header;
    }
#   endif

    
    // If bi-endian, calculate little-to-big offset
    size_t ltob_off = 0;
    if (psplc_head_little && psplc_head_big)
        ltob_off = psplc_head_big->tier1_object_array_off - psplc_head_little->tier1_object_array_off;
    
    
    // Next, embedded objects
    pspl_object_array_tier2_t* ext_array =
    (pspl_object_array_tier2_t*)(existing_psplc->psplc_data + psplc_head_little->tier1_object_array_off);
    pspl_object_array_tier2_t* ext_use_array = NULL;
    for (i=0 ; i<psplc_head_little->tier1_object_array_count ; ++i) {
        check_psplc_underflow(existing_psplc, ext_array+sizeof(pspl_object_array_tier2_t)-1);
        
#       if defined(__LITTLE_ENDIAN__)
        if (pspl_head->endian_flags == PSPL_LITTLE_ENDIAN) {
            ext_use_array = ext_array;
        } else if (pspl_head->endian_flags == PSPL_BIG_ENDIAN) {
            pspl_object_array_tier2_t swapped = {
                .tier2_extension_index = swap_uint32(ext_array->tier2_extension_index),
                .tier2_hash_indexed_object_count = swap_uint32(ext_array->tier2_hash_indexed_object_count),
                .tier2_int_indexed_object_count = swap_uint32(ext_array->tier2_int_indexed_object_count)
            };
            ext_use_array = &swapped;
        }
#       elif defined(__BIG_ENDIAN__)
        if (pspl_head->endian_flags == PSPL_LITTLE_ENDIAN) {
            pspl_object_array_tier2_t swapped = {
                .tier2_extension_index = swap_uint32(ext_array->tier2_extension_index),
                .tier2_hash_indexed_object_count = swap_uint32(ext_array->tier2_hash_indexed_object_count),
                .tier2_int_indexed_object_count = swap_uint32(ext_array->tier2_int_indexed_object_count)
            };
            ext_use_array = &swapped;
        } else if (pspl_head->endian_flags == PSPL_BIG_ENDIAN) {
            ext_use_array = ext_array;
        }
#       endif
        else if (pspl_head->endian_flags == PSPL_BI_ENDIAN) {
            ext_use_array = ext_array;
        }
        
        // Resolve extension
        const pspl_extension_t* extension = existing_psplc->required_extension_set[ext_use_array->tier2_extension_index];
        
        // Parse object records
        
        // Hash objects
        pspl_object_hash_record_t* obj_hash_array =
        (pspl_object_hash_record_t*)(ext_array + sizeof(pspl_object_array_tier2_t));
        pspl_object_hash_record_t* obj_hash_use_array = NULL;
        
        for (j=0 ; j<ext_use_array->tier2_hash_indexed_object_count ; ++j) {
            check_psplc_underflow(existing_psplc, obj_hash_array+sizeof(pspl_object_hash_record_t)-1);
            
            const void* little_data = NULL;
            const void* big_data = NULL;
            
#           if defined(__LITTLE_ENDIAN__)
            if (pspl_head->endian_flags == PSPL_LITTLE_ENDIAN) {
                obj_hash_use_array = obj_hash_array;
                little_data = existing_psplc->psplc_data + obj_hash_use_array->object_off;
            } else if (pspl_head->endian_flags == PSPL_BIG_ENDIAN) {
                pspl_object_hash_record_t swapped = {
                    .platform_availability_bits = swap_uint32(obj_hash_array->platform_availability_bits),
                    .object_off = swap_uint32(obj_hash_array->object_off),
                    .object_len = swap_uint32(obj_hash_array->object_len)
                };
                memcpy(&swapped.object_hash, obj_hash_array->object_hash, sizeof(pspl_hash));
                obj_hash_use_array = &swapped;
                big_data = existing_psplc->psplc_data + obj_hash_use_array->object_off;
            }
#           elif defined(__BIG_ENDIAN__)
            if (pspl_head->endian_flags == PSPL_LITTLE_ENDIAN) {
                pspl_object_hash_record_t swapped = {
                    .platform_availability_bits = swap_uint32(obj_hash_array->platform_availability_bits),
                    .object_off = swap_uint32(obj_hash_array->object_off),
                    .object_len = swap_uint32(obj_hash_array->object_len)
                };
                memcpy(&swapped.object_hash, obj_hash_array->object_hash, sizeof(pspl_hash));
                obj_hash_use_array = &swapped;
                little_data = existing_psplc->psplc_data + obj_hash_use_array->object_off;
            } else if (pspl_head->endian_flags == PSPL_BIG_ENDIAN) {
                obj_hash_use_array = obj_hash_array;
                big_data = existing_psplc->psplc_data + obj_hash_use_array->object_off;
            }
#           endif
            else if (pspl_head->endian_flags == PSPL_BI_ENDIAN) {
                obj_hash_use_array = obj_hash_array;
#               if defined(__LITTLE_ENDIAN__)
                little_data = existing_psplc->psplc_data + obj_hash_use_array->object_off;
                big_data = existing_psplc->psplc_data + obj_hash_use_array->object_off + ltob_off;
#               elif defined(__BIG_ENDIAN__)
                big_data = existing_psplc->psplc_data + obj_hash_use_array->object_off;
                little_data = existing_psplc->psplc_data + obj_hash_use_array->object_off - ltob_off;
#               endif
            }
            
            // Add hashed record
            pspl_runtime_platform_t plats[PSPL_MAX_PLATFORMS+1];
            __plat_array_from_bits(plats, existing_psplc, obj_hash_use_array->platform_availability_bits);
            __pspl_indexer_keyhash_object_augment(ctx, extension, plats, &obj_hash_use_array->object_hash,
                                                  little_data, big_data, obj_hash_use_array->object_len);

            
            obj_hash_array += sizeof(pspl_object_hash_record_t);
        }
        
        // Integer objects
        pspl_object_int_record_t* obj_int_array =
        (pspl_object_int_record_t*)(ext_array + sizeof(pspl_object_array_tier2_t));
        pspl_object_int_record_t* obj_int_use_array = NULL;
        
        for (j=0 ; j<ext_use_array->tier2_int_indexed_object_count ; ++j) {
            check_psplc_underflow(existing_psplc, obj_int_array+sizeof(pspl_object_int_record_t)-1);
            
            const void* little_data = NULL;
            const void* big_data = NULL;
            
#           if defined(__LITTLE_ENDIAN__)
            if (pspl_head->endian_flags == PSPL_LITTLE_ENDIAN) {
                obj_int_use_array = obj_int_array;
                little_data = existing_psplc->psplc_data + obj_int_use_array->object_off;
            } else if (pspl_head->endian_flags == PSPL_BIG_ENDIAN) {
                pspl_object_int_record_t swapped = {
                    .object_index = swap_uint32(obj_int_array->object_index),
                    .platform_availability_bits = swap_uint32(obj_int_array->platform_availability_bits),
                    .object_off = swap_uint32(obj_int_array->object_off),
                    .object_len = swap_uint32(obj_int_array->object_len)
                };
                obj_int_use_array = &swapped;
                big_data = existing_psplc->psplc_data + obj_int_use_array->object_off;
            }
#           elif defined(__BIG_ENDIAN__)
            if (pspl_head->endian_flags == PSPL_LITTLE_ENDIAN) {
                pspl_object_int_record_t swapped = {
                    .object_index = swap_uint32(obj_int_array->object_index),
                    .platform_availability_bits = swap_uint32(obj_int_array->platform_availability_bits),
                    .object_off = swap_uint32(obj_int_array->object_off),
                    .object_len = swap_uint32(obj_int_array->object_len)
                };
                obj_int_use_array = &swapped;
                little_data = existing_psplc->psplc_data + obj_int_use_array->object_off;
            } else if (pspl_head->endian_flags == PSPL_BIG_ENDIAN) {
                obj_int_use_array = obj_int_array;
                big_data = existing_psplc->psplc_data + obj_int_use_array->object_off;
            }
#           endif
            else if (pspl_head->endian_flags == PSPL_BI_ENDIAN) {
                obj_int_use_array = obj_int_array;
#               if defined(__LITTLE_ENDIAN__)
                little_data = existing_psplc->psplc_data + obj_int_use_array->object_off;
                big_data = existing_psplc->psplc_data + obj_int_use_array->object_off + ltob_off;
#               elif defined(__BIG_ENDIAN__)
                big_data = existing_psplc->psplc_data + obj_int_use_array->object_off;
                little_data = existing_psplc->psplc_data + obj_int_use_array->object_off - ltob_off;
#               endif
            }
            
            // Add integer record
            pspl_runtime_platform_t plats[PSPL_MAX_PLATFORMS+1];
            __plat_array_from_bits(plats, existing_psplc, obj_int_use_array->platform_availability_bits);
            pspl_indexer_integer_object_augment(ctx, extension, plats, obj_int_use_array->object_index,
                                                little_data, big_data, obj_int_use_array->object_len);
            
            
            obj_int_array += sizeof(pspl_object_int_record_t);
        }
        
        ext_array += sizeof(pspl_object_array_tier2_t);
    }
    
    // Last, file entries
    
}

/* Augment indexer context with embedded hash-indexed object */
void pspl_indexer_hash_object_augment(pspl_indexer_context_t* ctx, const pspl_extension_t* owner,
                                      const pspl_runtime_platform_t* plats, const char* key,
                                      const void* little_data, const void* big_data, size_t data_len) {
    
}

/* Augment indexer context with embedded integer-indexed object */
void pspl_indexer_integer_object_augment(pspl_indexer_context_t* ctx, const pspl_extension_t* owner,
                                         const pspl_runtime_platform_t* plats, uint32_t key,
                                         const void* little_data, const void* big_data, size_t data_len) {
    
}

/* Augment indexer context with file-stub
 * (triggering conversion hook if provided and output is outdated) */
void pspl_indexer_stub_file_augment(pspl_indexer_context_t* ctx, const pspl_extension_t* owner,
                                    const pspl_runtime_platform_t* plats, const char* path_in,
                                    pspl_converter_file_hook converter_hook, uint8_t move_output,
                                    pspl_hash* hash_out) {
    
}
void pspl_indexer_stub_membuf_augment(pspl_indexer_context_t* ctx, const pspl_extension_t* owner,
                                      const pspl_runtime_platform_t* plats, const char* path_in,
                                      pspl_converter_membuf_hook converter_hook,
                                      pspl_hash* hash_out) {
    
}
