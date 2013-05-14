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
#include <sys/param.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <errno.h>

#include <PSPLInternal.h>
#include <PSPLHash.h>

#include "ObjectIndexer.h"
#include "Driver.h"
#include "ReferenceGatherer.h"

#define PSPL_INDEXER_INITIAL_CAP 50

/* Determine if source is newer than last-known hashed binary object */
static int is_source_newer_than_object(const char* path, const pspl_hash* hash) {
    // Stat source
    struct stat src_stat;
    if (stat(path, &src_stat))
        return 1;
    
    // Stat object
    char obj_path[MAXPATHLEN];
    obj_path[0] = '\0';
    strcat(obj_path, driver_state.staging_path);
    strcat(obj_path, "/PSPLFiles/");
    char hash_str[PSPL_HASH_STRING_LEN];
    pspl_hash_fmt(hash_str, hash);
    strcat(obj_path, hash_str);
    struct stat obj_stat;
    if (stat(obj_path, &obj_stat))
        pspl_error(-1, "Unable to stat object file",
                   "object `%s` (corresponding to `%s`) doesn't exist;"
                   " recompilation may be necessary",
                   hash_str, path);
    
    // Compare modtimes
    if (src_stat.st_mtimespec.tv_sec >= obj_stat.st_mtimespec.tv_sec)
        return 1;
    
    return 0;
}

/* Determine if source is newer than set output */
static int is_source_newer_than_output(const char* path) {
    // Integrate this later
    return 1;
    
    // Stat source
    struct stat src_stat;
    if (stat(path, &src_stat))
        return 1;
    
    if (!driver_state.out_path)
        return 1;
    
    // Stat object
    struct stat out_stat;
    if (stat(driver_state.out_path, &out_stat))
        pspl_error(-1, "Unable to stat output file",
                   "can't stat `%s`",
                   path);
    
    // Compare modtimes
    if (src_stat.st_mtimespec.tv_sec >= out_stat.st_mtimespec.tv_sec)
        return 1;
    
    return 0;
}

/* Hash file content */
#define PSPL_HASH_READBUF_LEN 4096
static int hash_file(pspl_hash* hash_out, const char* path) {
    FILE* file = fopen(path, "r");
    if (file) {
        uint8_t file_buf[PSPL_HASH_READBUF_LEN];
        size_t read_len;
        pspl_hash_ctx_t hash_ctx;
        pspl_hash_init(&hash_ctx);
        do {
            read_len = fread(file_buf, 1, PSPL_HASH_READBUF_LEN, file);
            pspl_hash_write(&hash_ctx, file_buf, read_len);
        } while (read_len);
        fclose(file);
        pspl_hash* hash_result;
        pspl_hash_result(&hash_ctx, hash_result);
        pspl_hash_cpy(hash_out, hash_result);
        return 0;
    }
    
    return -1;
}

/* Copy file */
static int copy_file(const char* dest_path, const char* src_path) {
    int in_fd = open(src_path, O_RDONLY);
    if (in_fd < 0)
        return -1;
    int out_fd = open(dest_path, O_WRONLY|O_CREAT, 0644);
    if (out_fd < 0)
        return -1;
    char buf[8192];
    
    while (1) {
        ssize_t result = read(in_fd, &buf[0], sizeof(buf));
        if (!result) break;
        if (result < 0)
            return -1;
        if (write(out_fd, &buf[0], result) != result)
            return -1;
    }
    
    close(in_fd);
    close(out_fd);
    return 0;
}

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
static void __plat_array_from_bits(const pspl_runtime_platform_t** plats_out,
                                   pspl_toolchain_driver_psplc_t* psplc,
                                   uint32_t bits) {
    int i;
    int j=0;
    for (i=0 ; i<32 ; ++i)
        if ((bits>>i) & 0x1)
            plats_out[j++] = psplc->required_platform_set[i];
    plats_out[j] = NULL;
}

/* Augment indexer context with embedded hash-indexed object (from PSPLC) */
static void __pspl_indexer_hash_object_post_augment(pspl_indexer_context_t* ctx, const pspl_extension_t* owner,
                                                    const pspl_runtime_platform_t** plats, pspl_hash* key_hash,
                                                    const void* little_data, const void* big_data, size_t data_len,
                                                    pspl_toolchain_driver_psplc_t* definer) {
    int i;
    
    // Ensure object doesn't already exist
    for (i=0 ; i<ctx->h_objects_count ; ++i)
        if (!pspl_hash_cmp(&ctx->h_objects_array[i]->object_hash, key_hash) &&
            ctx->h_objects_array[i]->owner_ext == owner) {
            char hash_fmt[PSPL_HASH_STRING_LEN];
            pspl_hash_fmt(hash_fmt, key_hash);
            pspl_error(-1, "PSPLC Embedded object multiply defined",
                       "object `%s` previously defined in `%s` as well as `%s` for extension `%s`",
                      hash_fmt, ctx->h_objects_array[i]->definer, definer->file_path, owner->extension_name);
        }
    
    // Allocate and add
    ++ctx->h_objects_count;
    if (ctx->h_objects_count >= ctx->h_objects_cap) {
        ctx->h_objects_cap *= 2;
        ctx->h_objects_array = realloc(ctx->h_objects_array, sizeof(pspl_indexer_entry_t)*ctx->h_objects_cap);
    }
    pspl_indexer_entry_t* new_entry = malloc(sizeof(pspl_indexer_entry_t));
    ctx->h_objects_array[ctx->h_objects_count-1] = new_entry;
        
    // Ensure extension is added
    for (i=0 ; i<ctx->ext_count ; ++i)
        if (ctx->ext_array[i] == owner)
            break;
    if (i == ctx->ext_count)
        ctx->ext_array[ctx->ext_count++] = owner;
    
    // Ensure platforms are added
    if (plats) {
        new_entry->platform_availability_bits = 0;
        --plats;
        while (*(++plats)) {
            for (i=0 ; i<ctx->plat_count ; ++i)
                if (ctx->plat_array[i] == *plats) {
                    new_entry->platform_availability_bits |= 1<<i;
                    break;
                }
            if (i == ctx->plat_count) {
                ctx->plat_array[ctx->plat_count++] = *plats;
                new_entry->platform_availability_bits |= 1<<i;
            }
        }
    } else
        new_entry->platform_availability_bits = ~0;
    
    // Populate structure
    new_entry->definer = definer->file_path;
    new_entry->owner_ext = owner;
    pspl_hash_cpy(&new_entry->object_hash, key_hash);
    new_entry->object_len = data_len;
    new_entry->object_little_data = little_data;
    new_entry->object_big_data = big_data;
    
}

/* Augment indexer context with embedded integer-indexed object (from PSPLC) */
static void __pspl_indexer_integer_object_post_augment(pspl_indexer_context_t* ctx, const pspl_extension_t* owner,
                                                       const pspl_runtime_platform_t** plats, uint32_t key,
                                                       const void* little_data, const void* big_data, size_t data_len,
                                                       pspl_toolchain_driver_psplc_t* definer) {
    int i;
    
    // Ensure object doesn't already exist
    for (i=0 ; i<ctx->i_objects_count ; ++i)
        if (ctx->i_objects_array[i]->object_index == key &&
            ctx->i_objects_array[i]->owner_ext == owner) {
            pspl_error(-1, "PSPLC Embedded object multiply defined",
                       "indexed object `%u` previously defined in `%s` as well as `%s` for extension `%s`",
                       key, ctx->i_objects_array[i]->definer, definer->file_path, owner->extension_name);
        }
    
    // Allocate and add
    ++ctx->i_objects_count;
    if (ctx->i_objects_count >= ctx->i_objects_cap) {
        ctx->i_objects_cap *= 2;
        ctx->i_objects_array = realloc(ctx->i_objects_array, sizeof(pspl_indexer_entry_t)*ctx->i_objects_cap);
    }
    pspl_indexer_entry_t* new_entry = malloc(sizeof(pspl_indexer_entry_t));
    ctx->i_objects_array[ctx->i_objects_count-1] = new_entry;
    
    // Ensure extension is added
    for (i=0 ; i<ctx->ext_count ; ++i)
        if (ctx->ext_array[i] == owner)
            break;
    if (i == ctx->ext_count)
        ctx->ext_array[ctx->ext_count++] = owner;
    
    // Ensure platforms are added
    if (plats) {
        new_entry->platform_availability_bits = 0;
        --plats;
        while (*(++plats)) {
            for (i=0 ; i<ctx->plat_count ; ++i)
                if (ctx->plat_array[i] == *plats)
                    break;
            if (i == ctx->plat_count)
                ctx->plat_array[ctx->plat_count++] = *plats;
            new_entry->platform_availability_bits |= 1<<i;
        }
    } else
        new_entry->platform_availability_bits = ~0;
    
    // Populate structure
    new_entry->definer = definer->file_path;
    new_entry->owner_ext = owner;
    new_entry->object_index = key;
    new_entry->object_len = data_len;
    new_entry->object_little_data = little_data;
    new_entry->object_big_data = big_data;
    
}

/* Augment indexer context with file stub (from PSPLC) */
static void __pspl_indexer_stub_file_post_augment(pspl_indexer_context_t* ctx,
                                                  const pspl_runtime_platform_t** plats, const char* path_in,
                                                  pspl_hash* hash_in, pspl_toolchain_driver_psplc_t* definer) {
    int i;
    
    // Ensure object doesn't already exist
    for (i=0 ; i<ctx->stubs_count ; ++i)
        if (!strcmp(ctx->stubs_array[i]->stub_source_path, path_in))
            return;
    
    // Warn user if the source is newer than the object (necessitating recompile)
    if (is_source_newer_than_object(path_in, hash_in))
        pspl_warn("Newer data source detected",
                  "PSPL detected that `%s` has changed since `%s` was last compiled. "
                  "Old converted object will be used. Recompile if this is not desired.",
                  path_in, definer->file_path);
    
    // Allocate and add
    ++ctx->stubs_count;
    if (ctx->stubs_count >= ctx->stubs_cap) {
        ctx->stubs_cap *= 2;
        ctx->stubs_array = realloc(ctx->stubs_array, sizeof(pspl_indexer_entry_t)*ctx->stubs_cap);
    }
    pspl_indexer_entry_t* new_entry = malloc(sizeof(pspl_indexer_entry_t));
    ctx->stubs_array[ctx->stubs_count-1] = new_entry;
    
    // Ensure platforms are added
    if (plats) {
        new_entry->platform_availability_bits = 0;
        --plats;
        while (*(++plats)) {
            for (i=0 ; i<ctx->plat_count ; ++i)
                if (ctx->plat_array[i] == *plats)
                    break;
            if (i == ctx->plat_count)
                ctx->plat_array[ctx->plat_count++] = *plats;
            new_entry->platform_availability_bits |= 1<<i;
        }
    } else
        new_entry->platform_availability_bits = ~0;
    
    // Populate structure
    new_entry->definer = definer->file_path;
    pspl_hash_cpy(&new_entry->object_hash, hash_in);
    size_t cpy_len = strlen(path_in)+1;
    char* cpy_path = malloc(cpy_len);
    strncpy(cpy_path, path_in, cpy_len);
    new_entry->stub_source_path = cpy_path;
    
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
        memcpy(&swapped_header.psplc_object_hash, &psplc_head_little->psplc_object_hash, sizeof(pspl_hash));
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
        memcpy(&swapped_header.psplc_object_hash, &psplc_head_big->psplc_object_hash, sizeof(pspl_hash));
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
        memcpy(&swapped_header.psplc_object_hash, &psplc_head_little->psplc_object_hash, sizeof(pspl_hash));
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
        memcpy(&swapped_header.psplc_object_hash, &psplc_head_little->psplc_object_hash, sizeof(pspl_hash));
        psplc_head_little = &swapped_header;
    }
#   endif

    // Select best native psplc head
#   if defined(__LITTLE_ENDIAN__)
    pspl_psplc_header_t* best_head = (psplc_head_little)?psplc_head_little:psplc_head_big;
#   elif defined(__BIG_ENDIAN__)
    pspl_psplc_header_t* best_head = (psplc_head_big)?psplc_head_big:psplc_head_little;
#   endif
    
    // If bi-endian, calculate little-to-big offset
    size_t ltob_off = 0;
    if (psplc_head_little && psplc_head_big)
        ltob_off = psplc_head_big->tier1_object_array_off - psplc_head_little->tier1_object_array_off;
    
    
    // Next, embedded objects
    pspl_object_array_tier2_t* ext_array =
    (pspl_object_array_tier2_t*)(existing_psplc->psplc_data + best_head->tier1_object_array_off);
    pspl_object_array_tier2_t* ext_use_array = NULL;
    for (i=0 ; i<best_head->tier1_object_array_count ; ++i) {
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
                memcpy(&swapped.object_hash, &obj_hash_array->object_hash, sizeof(pspl_hash));
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
                memcpy(&swapped.object_hash, &obj_hash_array->object_hash, sizeof(pspl_hash));
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
                pspl_object_hash_record_t* big_record = obj_hash_use_array + ltob_off;
                big_data = existing_psplc->psplc_data + swap_uint32(big_record->object_off);
#               elif defined(__BIG_ENDIAN__)
                big_data = existing_psplc->psplc_data + obj_hash_use_array->object_off;
                pspl_object_hash_record_t* little_record = obj_hash_use_array - ltob_off;
                little_data = existing_psplc->psplc_data + swap_uint32(little_record->object_off);
#               endif
            }
            
            // Add hashed record
            const pspl_runtime_platform_t* plats[PSPL_MAX_PLATFORMS+1];
            __plat_array_from_bits(plats, existing_psplc, obj_hash_use_array->platform_availability_bits);
            __pspl_indexer_hash_object_post_augment(ctx, extension, plats, &obj_hash_use_array->object_hash,
                                                    little_data, big_data, obj_hash_use_array->object_len,
                                                    existing_psplc);

            
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
                pspl_object_int_record_t* big_record = obj_int_use_array + ltob_off;
                big_data = existing_psplc->psplc_data + swap_uint32(big_record->object_off);
#               elif defined(__BIG_ENDIAN__)
                big_data = existing_psplc->psplc_data + obj_int_use_array->object_off;
                pspl_object_int_record_t* little_record = obj_int_use_array - ltob_off;
                little_data = existing_psplc->psplc_data + swap_uint32(little_record->object_off);
#               endif
            }
            
            // Add integer record
            const pspl_runtime_platform_t* plats[PSPL_MAX_PLATFORMS+1];
            __plat_array_from_bits(plats, existing_psplc, obj_int_use_array->platform_availability_bits);
            __pspl_indexer_integer_object_post_augment(ctx, extension, plats, obj_int_use_array->object_index,
                                                       little_data, big_data, obj_int_use_array->object_len,
                                                       existing_psplc);
            
            
            obj_int_array += sizeof(pspl_object_int_record_t);
        }
        
        ext_array += sizeof(pspl_object_array_tier2_t);
    }
    
    // Last, file entries
    pspl_object_stub_t* stub_array = (pspl_object_stub_t*)(existing_psplc->psplc_data + best_head->file_stub_array_off);
    for (i=0 ; i<best_head->file_stub_count ; ++i) {
        
        const pspl_runtime_platform_t* plats[PSPL_MAX_PLATFORMS+1];
        const char* source_path = NULL;
#       if defined(__LITTLE_ENDIAN__)
        if (pspl_head->endian_flags == PSPL_LITTLE_ENDIAN || pspl_head->endian_flags == PSPL_BI_ENDIAN) {
            __plat_array_from_bits(plats, existing_psplc, stub_array->platform_availability_bits);
            source_path = (char*)(existing_psplc->psplc_data + stub_array->object_source_path_off);
        } else if (pspl_head->endian_flags == PSPL_BIG_ENDIAN) {
            __plat_array_from_bits(plats, existing_psplc, swap_uint32(stub_array->platform_availability_bits));
            source_path = (char*)(existing_psplc->psplc_data + swap_uint32(stub_array->object_source_path_off));
        }
#       elif defined(__BIG_ENDIAN__)
        if (pspl_head->endian_flags == PSPL_BIG_ENDIAN  || pspl_head->endian_flags == PSPL_BI_ENDIAN) {
            __plat_array_from_bits(plats, existing_psplc, stub_array->platform_availability_bits);
            source_path = (char*)(existing_psplc->psplc_data + stub_array->object_source_path_off);
        } else if (pspl_head->endian_flags == PSPL_LITTLE_ENDIAN) {
            __plat_array_from_bits(plats, existing_psplc, swap_uint32(stub_array->platform_availability_bits));
            source_path = (char*)(existing_psplc->psplc_data + swap_uint32(stub_array->object_source_path_off));
        }
#       endif
        __pspl_indexer_stub_file_post_augment(ctx, plats, source_path, &stub_array->object_hash, existing_psplc);
        stub_array += sizeof(pspl_object_stub_t);
        
    }
    
}

/* Augment indexer context with embedded hash-indexed object */
void pspl_indexer_hash_object_augment(pspl_indexer_context_t* ctx, const pspl_extension_t* owner,
                                      const pspl_runtime_platform_t** plats, const char* key,
                                      const void* little_data, const void* big_data, size_t data_len,
                                      pspl_toolchain_driver_source_t* definer) {
    int i;
    
    // Make key hash
    pspl_hash_ctx_t hash_ctx;
    pspl_hash_init(&hash_ctx);
    pspl_hash_write(&hash_ctx, key, strlen(key));
    pspl_hash* key_hash;
    pspl_hash_result(&hash_ctx, key_hash);
    
    // Ensure object doesn't already exist
    for (i=0 ; i<ctx->h_objects_count ; ++i)
        if (!pspl_hash_cmp(&ctx->h_objects_array[i]->object_hash, key_hash) &&
            ctx->h_objects_array[i]->owner_ext == owner) {
            char hash_fmt[PSPL_HASH_STRING_LEN];
            pspl_hash_fmt(hash_fmt, key_hash);
            pspl_error(-1, "PSPLC Embedded object multiply defined",
                       "object `%s` previously defined in `%s` as well as `%s` for extension `%s`",
                       hash_fmt, ctx->h_objects_array[i]->definer, definer->file_path, owner->extension_name);
        }
    
    // Allocate and add
    ++ctx->h_objects_count;
    if (ctx->h_objects_count >= ctx->h_objects_cap) {
        ctx->h_objects_cap *= 2;
        ctx->h_objects_array = realloc(ctx->h_objects_array, sizeof(pspl_indexer_entry_t)*ctx->h_objects_cap);
    }
    pspl_indexer_entry_t* new_entry = malloc(sizeof(pspl_indexer_entry_t));
    ctx->h_objects_array[ctx->h_objects_count-1] = new_entry;
    
    // Ensure extension is added
    for (i=0 ; i<ctx->ext_count ; ++i)
        if (ctx->ext_array[i] == owner)
            break;
    if (i == ctx->ext_count)
        ctx->ext_array[ctx->ext_count++] = owner;
    
    // Ensure platforms are added
    if (plats) {
        new_entry->platform_availability_bits = 0;
        --plats;
        while (*(++plats)) {
            for (i=0 ; i<ctx->plat_count ; ++i)
                if (ctx->plat_array[i] == *plats) {
                    new_entry->platform_availability_bits |= 1<<i;
                    break;
                }
            if (i == ctx->plat_count) {
                ctx->plat_array[ctx->plat_count++] = *plats;
                new_entry->platform_availability_bits |= 1<<i;
            }
        }
    } else
        new_entry->platform_availability_bits = ~0;
    
    // Populate structure
    new_entry->definer = definer->file_path;
    new_entry->owner_ext = owner;
    pspl_hash_cpy(&new_entry->object_hash, key_hash);
    new_entry->object_len = data_len;
    void* little_buf = malloc(data_len);
    memcpy(little_buf, little_data, data_len);
    void* big_buf = little_buf;
    if (little_data != big_data) {
        big_buf = malloc(data_len);
        memcpy(big_buf, big_data, data_len);
    }
    new_entry->object_little_data = little_buf;
    new_entry->object_big_data = big_buf;
}

/* Augment indexer context with embedded integer-indexed object */
void pspl_indexer_integer_object_augment(pspl_indexer_context_t* ctx, const pspl_extension_t* owner,
                                         const pspl_runtime_platform_t** plats, uint32_t key,
                                         const void* little_data, const void* big_data, size_t data_len,
                                         pspl_toolchain_driver_source_t* definer) {
    int i;
    
    // Ensure object doesn't already exist
    for (i=0 ; i<ctx->i_objects_count ; ++i)
        if (ctx->i_objects_array[i]->object_index == key &&
            ctx->i_objects_array[i]->owner_ext == owner) {
            pspl_error(-1, "PSPLC Embedded object multiply defined",
                       "indexed object `%u` previously defined in `%s` as well as `%s` for extension `%s`",
                       key, ctx->i_objects_array[i]->definer, definer->file_path, owner->extension_name);
        }
    
    // Allocate and add
    ++ctx->i_objects_count;
    if (ctx->i_objects_count >= ctx->i_objects_cap) {
        ctx->i_objects_cap *= 2;
        ctx->i_objects_array = realloc(ctx->i_objects_array, sizeof(pspl_indexer_entry_t)*ctx->i_objects_cap);
    }
    pspl_indexer_entry_t* new_entry = malloc(sizeof(pspl_indexer_entry_t));
    ctx->i_objects_array[ctx->i_objects_count-1] = new_entry;
    
    // Ensure extension is added
    for (i=0 ; i<ctx->ext_count ; ++i)
        if (ctx->ext_array[i] == owner)
            break;
    if (i == ctx->ext_count)
        ctx->ext_array[ctx->ext_count++] = owner;
    
    // Ensure platforms are added
    if (plats) {
        new_entry->platform_availability_bits = 0;
        --plats;
        while (*(++plats)) {
            for (i=0 ; i<ctx->plat_count ; ++i)
                if (ctx->plat_array[i] == *plats) {
                    new_entry->platform_availability_bits |= 1<<i;
                    break;
                }
            if (i == ctx->plat_count) {
                ctx->plat_array[ctx->plat_count++] = *plats;
                new_entry->platform_availability_bits |= 1<<i;
            }
        }
    } else
        new_entry->platform_availability_bits = ~0;
    
    // Populate structure
    new_entry->definer = definer->file_path;
    new_entry->owner_ext = owner;
    new_entry->object_index = key;
    new_entry->object_len = data_len;
    void* little_buf = malloc(data_len);
    memcpy(little_buf, little_data, data_len);
    void* big_buf = little_buf;
    if (little_data != big_data) {
        big_buf = malloc(data_len);
        memcpy(big_buf, big_data, data_len);
    }
    new_entry->object_little_data = little_buf;
    new_entry->object_big_data = big_buf;
}

/* Augment indexer context with file-stub
 * (triggering conversion hook if provided and output is outdated) */
static struct {
    uint8_t last_prog;
    const char* path;
} converter_state;
void pspl_converter_progress_update(double progress) {
    uint8_t prog_int = progress*100;
    if (prog_int == converter_state.last_prog)
        return; // Ease load on terminal if nothing is textually changing
    if (xterm_colour)
        fprintf(stderr, "\r\033[1m[");
    else
        fprintf(stderr, "\r[");
    if (prog_int >= 100)
        fprintf(stderr, "100");
    else if (prog_int >= 10)
        fprintf(stderr, " %u", prog_int);
    else
        fprintf(stderr, "  %u", prog_int);
    if (xterm_colour)
        fprintf(stderr, "%c]\E[m\017 \E[47;32m\E[47;49mConverting \033[1m%s\E[m\017", '%', converter_state.path);
    else
        fprintf(stderr, "%c] Converting `%s`", '%', converter_state.path);
    converter_state.last_prog = prog_int;
}
void pspl_indexer_stub_file_augment(pspl_indexer_context_t* ctx,
                                    const pspl_runtime_platform_t** plats, const char* path_in,
                                    pspl_converter_file_hook converter_hook, uint8_t move_output,
                                    pspl_hash** hash_out,
                                    pspl_toolchain_driver_source_t* definer) {
    converter_state.path = path_in;
    converter_state.last_prog = 1;
    char abs_path[MAXPATHLEN];
    abs_path[0] = '\0';
    
    // Ensure staging directory exists
    strcat(abs_path, driver_state.staging_path);
    strcat(abs_path, "/PSPLFiles");
    if(mkdir(abs_path, 0755))
        if (errno != EEXIST)
            pspl_error(-1, "Error creating staging directory",
                       "unable to create `%s` as staging directory",
                       abs_path);
    
    
    // Make path absolute
    if (*path_in != '/') {
        abs_path[0] = '\0';
        strcat(abs_path, definer->file_enclosing_dir);
        strcat(abs_path, path_in);
        path_in = abs_path;
    }
    
    int i;
    
    // Ensure object doesn't already exist
    for (i=0 ; i<ctx->stubs_count ; ++i)
        if (!strcmp(ctx->stubs_array[i]->stub_source_path, path_in))
            return;
    
    // Hash path for temporary output use
    pspl_hash_ctx_t hash_ctx;
    pspl_hash_init(&hash_ctx);
    pspl_hash_write(&hash_ctx, path_in, strlen(path_in));
    pspl_hash* path_hash;
    pspl_hash_result(&hash_ctx, path_hash);
    char path_hash_str[PSPL_HASH_STRING_LEN];
    pspl_hash_fmt(path_hash_str, path_hash);
    char sug_path[MAXPATHLEN];
    sug_path[0] = '\0';
    strcat(sug_path, driver_state.staging_path);
    strcat(sug_path, "/PSPLFiles/tmp_");
    strcat(sug_path, path_hash_str);
    
    // Allocate and add
    ++ctx->stubs_count;
    if (ctx->stubs_count >= ctx->stubs_cap) {
        ctx->stubs_cap *= 2;
        ctx->stubs_array = realloc(ctx->stubs_array, sizeof(pspl_indexer_entry_t)*ctx->stubs_cap);
    }
    pspl_indexer_entry_t* new_entry = malloc(sizeof(pspl_indexer_entry_t));
    ctx->stubs_array[ctx->stubs_count-1] = new_entry;
    
    // Ensure platforms are added
    if (plats) {
        new_entry->platform_availability_bits = 0;
        --plats;
        while (*(++plats)) {
            for (i=0 ; i<ctx->plat_count ; ++i)
                if (ctx->plat_array[i] == *plats) {
                    new_entry->platform_availability_bits |= 1<<i;
                    break;
                }
            if (i == ctx->plat_count) {
                ctx->plat_array[ctx->plat_count++] = *plats;
                new_entry->platform_availability_bits |= 1<<i;
            }
        }
    } else
        new_entry->platform_availability_bits = ~0;
    
    if (is_source_newer_than_output(path_in)) {
        
        // Convert data
        const char* final_path;
        if (converter_hook) {
            char conv_path_buf[MAXPATHLEN];
            conv_path_buf[0] = '\0';
            int err;
            pspl_converter_progress_update(0.0);
            if((err = converter_hook(conv_path_buf, path_in, sug_path))) {
                fprintf(stderr, "\n");
                pspl_error(-1, "Error converting file", "converter hook returned error '%d' for `%s`",
                           err, path_in);
            }
            pspl_converter_progress_update(1.0);
            fprintf(stderr, "\n");
            final_path = conv_path_buf;
        } else {
            if(copy_file(sug_path, path_in))
                pspl_error(-1, "Unable to copy file",
                           "unable to copy `%s` during conversion", path_in);
            final_path = sug_path;
        }
        
        // Hash converted data
        if (hash_file(&new_entry->object_hash, final_path))
            pspl_error(-1, "Unable to hash file",
                       "error while hashing `%s`", final_path);
        *hash_out = &new_entry->object_hash;
        
        // Final Hash path
        char final_hash_str[PSPL_HASH_STRING_LEN];
        pspl_hash_fmt(final_hash_str, &new_entry->object_hash);
        char final_hash_path_str[MAXPATHLEN];
        final_hash_path_str[0] = '\0';
        strcat(final_hash_path_str, driver_state.staging_path);
        strcat(final_hash_path_str, "/PSPLFiles/");
        strcat(final_hash_path_str, final_hash_str);
        
        // Copy (or move)
        if (converter_hook && !move_output) {
            if(copy_file(final_hash_path_str, final_path))
                pspl_error(-1, "Unable to copy file",
                           "unable to copy `%s` during conversion", final_path);
        } else
            if(rename(final_path, final_hash_path_str))
                pspl_error(-1, "Unable to move file",
                           "unable to move `%s` during conversion", final_path);
        
        // Message
        if (xterm_colour)
            fprintf(stderr, "\033[1mHash: \E[47;36m\E[47;49m%s\E[m\017\n", final_hash_str);
        else
            fprintf(stderr, "Hash: %s\n", final_hash_str);
        
    }
    
    // Populate structure
    new_entry->definer = definer->file_path;
    size_t cpy_len = strlen(path_in)+1;
    char* cpy_path = malloc(cpy_len);
    strncpy(cpy_path, path_in, cpy_len);
    new_entry->stub_source_path = cpy_path;
    if (driver_state.gather_ctx)
        pspl_gather_add_file(driver_state.gather_ctx, cpy_path);
    
    converter_state.path = NULL;
    
}
void pspl_indexer_stub_membuf_augment(pspl_indexer_context_t* ctx,
                                      const pspl_runtime_platform_t** plats, const char* path_in,
                                      pspl_converter_membuf_hook converter_hook,
                                      pspl_hash** hash_out,
                                      pspl_toolchain_driver_source_t* definer) {
    if (!converter_hook)
        return;
    
    converter_state.path = path_in;
    converter_state.last_prog = 1;
    char abs_path[MAXPATHLEN];
    abs_path[0] = '\0';
    
    // Ensure staging directory exists
    strcat(abs_path, driver_state.staging_path);
    strcat(abs_path, "/PSPLFiles");
    if(mkdir(abs_path, 0755))
        if (errno != EEXIST)
            pspl_error(-1, "Error creating staging directory",
                       "unable to create `%s` as staging directory",
                       abs_path);
    
    // Make path absolute
    if (*path_in != '/') {
        abs_path[0] = '\0';
        strcat(abs_path, definer->file_enclosing_dir);
        strcat(abs_path, path_in);
        path_in = abs_path;
    }
    
    int i;
    
    // Ensure object doesn't already exist
    for (i=0 ; i<ctx->stubs_count ; ++i)
        if (!strcmp(ctx->stubs_array[i]->stub_source_path, path_in))
            return;
    
    // Allocate and add
    ++ctx->stubs_count;
    if (ctx->stubs_count >= ctx->stubs_cap) {
        ctx->stubs_cap *= 2;
        ctx->stubs_array = realloc(ctx->stubs_array, sizeof(pspl_indexer_entry_t)*ctx->stubs_cap);
    }
    pspl_indexer_entry_t* new_entry = malloc(sizeof(pspl_indexer_entry_t));
    ctx->stubs_array[ctx->stubs_count-1] = new_entry;
    
    // Ensure platforms are added
    if (plats) {
        new_entry->platform_availability_bits = 0;
        --plats;
        while (*(++plats)) {
            for (i=0 ; i<ctx->plat_count ; ++i)
                if (ctx->plat_array[i] == *plats) {
                    new_entry->platform_availability_bits |= 1<<i;
                    break;
                }
            if (i == ctx->plat_count) {
                ctx->plat_array[ctx->plat_count++] = *plats;
                new_entry->platform_availability_bits |= 1<<i;
            }
        }
    } else
        new_entry->platform_availability_bits = ~0;
    
    if (is_source_newer_than_output(path_in)) {
        
        // Convert data
        void* conv_buf = NULL;
        size_t conv_len = 0;
        int err;
        pspl_converter_progress_update(0.0);
        if((err = converter_hook(&conv_buf, &conv_len, path_in))) {
            fprintf(stderr, "\n");
            pspl_error(-1, "Error converting file", "converter hook returned error '%d' for `%s`",
                       err, path_in);
        }
        pspl_converter_progress_update(1.0);
        fprintf(stderr, "\n");
        if (!conv_buf || !conv_len)
            pspl_error(-1, "Empty conversion buffer returned",
                       "conversion hook returned empty buffer for `%s`", path_in);
        
        // Hash converted data
        pspl_hash_ctx_t hash_ctx;
        pspl_hash_init(&hash_ctx);
        pspl_hash_write(&hash_ctx, conv_buf, conv_len);
        pspl_hash* hash_result;
        pspl_hash_result(&hash_ctx, hash_result);
        pspl_hash_cpy(&new_entry->object_hash, hash_result);
        *hash_out = &new_entry->object_hash;
        
        // Final Hash path
        char final_hash_str[PSPL_HASH_STRING_LEN];
        pspl_hash_fmt(final_hash_str, &new_entry->object_hash);
        char final_hash_path_str[MAXPATHLEN];
        final_hash_path_str[0] = '\0';
        strcat(final_hash_path_str, driver_state.staging_path);
        strcat(final_hash_path_str, "/PSPLFiles/");
        strcat(final_hash_path_str, final_hash_str);
        
        // Write to staging area
        FILE* file = fopen(final_hash_path_str, "w");
        if (!file)
            pspl_error(-1, "Unable to open conversion file for writing", "Unable to write to `%s`",
                       final_hash_path_str);
        fwrite(conv_buf, 1, conv_len, file);
        fclose(file);
        
        // Message
        if (xterm_colour)
            fprintf(stderr, "\033[1mHash: \E[47;36m\E[47;49m%s\E[m\017\n", final_hash_str);
        else
            fprintf(stderr, "Hash: %s\n", final_hash_str);
        
    }
    
    // Populate structure
    new_entry->definer = definer->file_path;
    size_t cpy_len = strlen(path_in)+1;
    char* cpy_path = malloc(cpy_len);
    strncpy(cpy_path, path_in, cpy_len);
    new_entry->stub_source_path = cpy_path;
    if (driver_state.gather_ctx)
        pspl_gather_add_file(driver_state.gather_ctx, cpy_path);
    
    converter_state.path = NULL;
    
}
