//
//  Packager.c
//  PSPL
//
//  Created by Jack Andersen on 4/28/13.
//
//

#define PSPL_INTERNAL

#include <PSPLExtension.h>
#include <PSPLInternal.h>
#include <PSPL/PSPLHash.h>
#include <errno.h>
#include <dirent.h>

#include "Packager.h"
#include "ObjectIndexer.h"


#pragma mark Packager Implementation

/* Package indexer initialiser */
void pspl_packager_init(pspl_packager_context_t* ctx,
                        unsigned int max_extension_count,
                        unsigned int max_platform_count,
                        unsigned int psplc_count) {
    
    ctx->ext_count = 0;
    ctx->ext_array = calloc(max_extension_count, sizeof(pspl_extension_t*));
    
    ctx->plat_count = 0;
    ctx->plat_array = calloc(max_platform_count, sizeof(pspl_platform_t*));
    
    ctx->indexer_count = 0;
    ctx->indexer_cap = psplc_count;
    ctx->indexer_array = calloc(psplc_count, sizeof(pspl_indexer_context_t*));
    
    ctx->stubs_count = 0;
    ctx->stubs_cap = 50;
    ctx->stubs_array = calloc(50, sizeof(pspl_indexer_entry_t*));
    
}

/* Augment with index */
void pspl_packager_indexer_augment(pspl_packager_context_t* ctx,
                                   pspl_indexer_context_t* indexer) {
    
    int i,j;
    uint8_t needs_add;
    
    if (ctx->indexer_count >= ctx->indexer_cap)
        pspl_error(-1, "Internal error",
                   "PSPL Packager would have augmented too many PSPLCs; expected %u",
                   ctx->indexer_cap);
    
    // Ensure PSPLC isn't already staged for adding
    for (i=0 ; i<ctx->indexer_count ; ++i) {
        pspl_indexer_context_t* present_idxer = ctx->indexer_array[i];
        if (!pspl_hash_cmp(&present_idxer->psplc_hash, &indexer->psplc_hash))
            pspl_error(-1, "Duplicate-name PSPLCs added to package",
                       "PSPLC '%s' and '%s' have the same name hash; "
                       "this is disallowed by PSPL",
                       present_idxer->definer, indexer->definer);
    }
    
    // Unify extension array
    for (i=0 ; i<indexer->ext_count ; ++i) {
        needs_add = 1;
        const pspl_extension_t* index_ext = indexer->ext_array[i];
        for (j=0 ; j<ctx->ext_count ; ++j) {
            const pspl_extension_t* pack_ext = ctx->ext_array[j];
            if (index_ext == pack_ext) {
                needs_add = 0;
                break;
            }
        }
        if (needs_add)
            ctx->ext_array[ctx->ext_count++] = index_ext;
    }
    
    // Unify platform array
    for (i=0 ; i<indexer->plat_count ; ++i) {
        needs_add = 1;
        const pspl_platform_t* index_plat = indexer->plat_array[i];
        for (j=0 ; j<ctx->plat_count ; ++j) {
            const pspl_platform_t* pack_plat = ctx->plat_array[j];
            if (index_plat == pack_plat) {
                needs_add = 0;
                break;
            }
        }
        if (needs_add)
            ctx->plat_array[ctx->plat_count++] = index_plat;
    }
    
    // Unify file stubs
    for (i=0 ; i<indexer->stubs_count ; ++i) {
        needs_add = 1;
        pspl_indexer_entry_t* index_stub = indexer->stubs_array[i];
        for (j=0 ; j<ctx->stubs_count ; ++j) {
            const pspl_indexer_entry_t* pack_stub = ctx->stubs_array[j];
            if (!pspl_hash_cmp(&index_stub->object_hash, &pack_stub->object_hash)) {
                needs_add = 0;
                break;
            }
        }
        if (needs_add) {
            if (ctx->stubs_count >= ctx->stubs_cap) {
                ctx->stubs_cap *= 2;
                ctx->stubs_array = realloc(ctx->stubs_array, ctx->stubs_cap * sizeof(pspl_indexer_entry_t*));
            }
            ctx->stubs_array[ctx->stubs_count++] = index_stub;
        }
    }
    
    // Add indexer
    ctx->indexer_array[ctx->indexer_count++] = indexer;
    
}

/* Prepare staged file for packaging */
static void prepare_staged_file(pspl_indexer_entry_t* ent) {
    
    // Concatenate together path
    char* path = ent->file_path;
    path[0] = '\0';
    strlcat(path, ent->stub_source_path, MAXPATHLEN);
    if (ent->stub_source_path_ext) {
        strlcat(path, ":", MAXPATHLEN);
        strlcat(path, ent->stub_source_path_ext, MAXPATHLEN);
    }
    
    // Hash path
    pspl_hash_ctx_t hash_ctx;
    pspl_hash_init(&hash_ctx);
    pspl_hash_write(&hash_ctx, path, strlen(path));
    pspl_hash* path_hash;
    pspl_hash_result(&hash_ctx, path_hash);
    char path_hash_str[PSPL_HASH_STRING_LEN];
    pspl_hash_fmt(path_hash_str, path_hash);
    
    // Object hash
    char object_hash_str[PSPL_HASH_STRING_LEN];
    pspl_hash_fmt(object_hash_str, &ent->object_hash);
    
    // Use dirent to scan PSPLFiles for bitwise-matching file
    DIR* staging_dir = opendir(driver_state.staging_path);
    if (!staging_dir)
        pspl_error(-1, "Unable to open Staging Directory", "`%s` inaccessible", driver_state.staging_path);
    struct dirent* dent = NULL;
    while ((dent = readdir(staging_dir))) {
        if (memcmp(dent->d_name, path_hash_str, 40))
            continue;
        const char* file_obj_hash = strrchr(dent->d_name, '_') + 1;
        if (memcmp(file_obj_hash, object_hash_str, 40))
            continue;
        
        const char* file_plat_bits_str = strchr(dent->d_name, '_') + 1;
        long file_plat_bits = strtol(file_plat_bits_str, NULL, 16);
        if (file_plat_bits & ent->build_platform_availability_bits)
            break;
    }
    if (!dent)
        pspl_error(-1, "Staged file unavailable",
                   "there should be a file with path hash '%s' and data hash '%s'",
                   path_hash_str, object_hash_str);
    
    // Now copy staged path
    path[0] = '\0';
    strlcpy(path, driver_state.staging_path, MAXPATHLEN);
    strlcat(path, dent->d_name, MAXPATHLEN);
    
    // Open file, make sure it still exists
    FILE* file = fopen(path, "r");
    if (!file)
        pspl_error(-1, "Staged file unavailable",
                   "there should be an accessible file at `%s`; derived from `%s`; errno %d (%s)",
                   path, ent->stub_source_path, errno, strerror(errno));
    
    // Record length
    fseek(file, 0, SEEK_END);
    ent->object_len = ftell(file);
    fclose(file);
    
}

/* Write out to PSPLP file */
void pspl_packager_write_psplp(pspl_packager_context_t* ctx,
                               uint8_t psplp_endianness,
                               FILE* psplp_file_out) {
    
    int i,j,k;
    
    // Determine endianness
    for (i=0 ; i<ctx->plat_count ; ++i)
        psplp_endianness |= ctx->plat_array[i]->byte_order;
    psplp_endianness &= PSPL_BI_ENDIAN;
    if (!psplp_endianness) {
#       if __LITTLE_ENDIAN__
        psplp_endianness = PSPL_LITTLE_ENDIAN;
#       elif __BIG_ENDIAN__
        psplp_endianness = PSPL_BIG_ENDIAN;
#       endif
    }
    
    // Table offset accumulations
    uint32_t acc = sizeof(pspl_header_t);
    acc += (psplp_endianness==PSPL_BI_ENDIAN) ? sizeof(pspl_off_header_bi_t) : sizeof(pspl_off_header_t);
    acc += (psplp_endianness==PSPL_BI_ENDIAN) ? sizeof(pspl_psplp_header_bi_t) : sizeof(pspl_psplp_header_t);
    acc += ((psplp_endianness==PSPL_BI_ENDIAN) ? sizeof(pspl_psplp_psplc_index_bi_t) : sizeof(pspl_psplp_psplc_index_t) + sizeof(pspl_hash)) * ctx->indexer_count;
    
    for (i=0 ; i<ctx->indexer_count ; ++i) {
        pspl_indexer_context_t* indexer = ctx->indexer_array[i];
        
        //printf("Will Write Beginning: %zu\n", acc);
        indexer->extension_obj_base_off = acc;
        //acc += sizeof(pspl_hash);
        acc += (psplp_endianness==PSPL_BI_ENDIAN) ? sizeof(pspl_psplc_header_bi_t) : sizeof(pspl_psplc_header_t);
        //printf("Will Write p1: %zu\n", acc);
        //printf("Will AHH %u\n",indexer->ext_count);
        acc += ((psplp_endianness==PSPL_BI_ENDIAN) ? sizeof(pspl_object_array_extension_bi_t) : sizeof(pspl_object_array_extension_t)) * indexer->ext_count;
        //printf("Will Write p2: %zu\n", acc);
        acc += ((psplp_endianness==PSPL_BI_ENDIAN) ? sizeof(pspl_object_array_extension_bi_t) : sizeof(pspl_object_array_extension_t)) * indexer->plat_count;
        indexer->extension_obj_array_off = acc - indexer->extension_obj_base_off; // PACKAGER SPECIFIC!!! - BASE-RELATIVE OFFSETS!!!
        //printf("Will Write p3: %zu\n", acc);
        acc += ((psplp_endianness==PSPL_BI_ENDIAN) ? sizeof(pspl_object_hash_record_bi_t) : sizeof(pspl_object_hash_record_t) + sizeof(pspl_hash)) * indexer->h_objects_count;
        acc += ((psplp_endianness==PSPL_BI_ENDIAN) ? sizeof(pspl_object_int_record_bi_t) : sizeof(pspl_object_int_record_t)) * indexer->i_objects_count;
        acc += ((psplp_endianness==PSPL_BI_ENDIAN) ? sizeof(pspl_object_hash_record_bi_t) : sizeof(pspl_object_hash_record_t) + sizeof(pspl_hash)) * indexer->ph_objects_count;
        acc += ((psplp_endianness==PSPL_BI_ENDIAN) ? sizeof(pspl_object_int_record_bi_t) : sizeof(pspl_object_int_record_t)) * indexer->pi_objects_count;
        indexer->extension_obj_array_padding = ROUND_UP_32(acc) - acc;
        acc += indexer->extension_obj_array_padding;
        //printf("Will Write: %u\n", acc);
        indexer->extension_obj_data_off = acc;
        
        // Per extension object blobs
        for (k=0 ; k<indexer->ext_count ; ++k) {
            const pspl_extension_t* cur_ext = indexer->ext_array[k];
            
            for (j=0 ; j<indexer->h_objects_count ; ++j) {
                pspl_indexer_entry_t* ent = indexer->h_objects_array[j];
                if (ent->owner_ext == cur_ext) {
                    
                    ent->object_off = acc;
                    if (psplp_endianness == PSPL_LITTLE_ENDIAN)
                        ent->object_big_data = NULL;
                    else if (psplp_endianness == PSPL_BIG_ENDIAN)
                        ent->object_little_data = NULL;
                    ent->object_padding = ROUND_UP_32(ent->object_len) - ent->object_len;
                    if (ent->object_little_data && ent->object_big_data &&
                        ent->object_little_data != ent->object_big_data)
                        acc += (ent->object_len+ent->object_padding)*2;
                    else
                        acc += ent->object_len+ent->object_padding;
                    
                }
            }
            for (j=0 ; j<indexer->i_objects_count ; ++j) {
                pspl_indexer_entry_t* ent = indexer->i_objects_array[j];
                if (ent->owner_ext == cur_ext) {
                    
                    ent->object_off = acc;
                    if (psplp_endianness == PSPL_LITTLE_ENDIAN)
                        ent->object_big_data = NULL;
                    else if (psplp_endianness == PSPL_BIG_ENDIAN)
                        ent->object_little_data = NULL;
                    ent->object_padding = ROUND_UP_32(ent->object_len) - ent->object_len;
                    if (ent->object_little_data && ent->object_big_data &&
                        ent->object_little_data != ent->object_big_data)
                        acc += (ent->object_len+ent->object_padding)*2;
                    else
                        acc += ent->object_len+ent->object_padding;
                    
                }
            }
        }
        
        // Per platform object blobs
        for (k=0 ; k<indexer->plat_count ; ++k) {
            const pspl_platform_t* cur_plat = indexer->plat_array[k];
                
            for (j=0 ; j<indexer->ph_objects_count ; ++j) {
                pspl_indexer_entry_t* ent = indexer->ph_objects_array[j];
                if (ent->owner_plat == cur_plat) {
                
                    ent->object_off = acc;
                    if (psplp_endianness == PSPL_LITTLE_ENDIAN)
                        ent->object_big_data = NULL;
                    else if (psplp_endianness == PSPL_BIG_ENDIAN)
                        ent->object_little_data = NULL;
                    ent->object_padding = ROUND_UP_32(ent->object_len) - ent->object_len;
                    if (ent->object_little_data && ent->object_big_data &&
                        ent->object_little_data != ent->object_big_data)
                        acc += (ent->object_len+ent->object_padding)*2;
                    else
                        acc += ent->object_len+ent->object_padding;
                    
                }
            }
            for (j=0 ; j<indexer->pi_objects_count ; ++j) {
                pspl_indexer_entry_t* ent = indexer->pi_objects_array[j];
                if (ent->owner_plat == cur_plat) {

                    ent->object_off = acc;
                    if (psplp_endianness == PSPL_LITTLE_ENDIAN)
                        ent->object_big_data = NULL;
                    else if (psplp_endianness == PSPL_BIG_ENDIAN)
                        ent->object_little_data = NULL;
                    ent->object_padding = ROUND_UP_32(ent->object_len) - ent->object_len;
                    if (ent->object_little_data && ent->object_big_data &&
                        ent->object_little_data != ent->object_big_data)
                        acc += (ent->object_len+ent->object_padding)*2;
                    else
                        acc += ent->object_len+ent->object_padding;
                    
                }
            }
            
        }
        
        indexer->extension_obj_blobs_len = acc - indexer->extension_obj_data_off; // PACKAGER SPECIFIC!!!
        //printf("Will Write End: %u\n", acc);
    }

    uint32_t file_stub_array_off = acc;
    acc += ((psplp_endianness==PSPL_BI_ENDIAN) ? sizeof(pspl_file_stub_bi_t) : sizeof(pspl_file_stub_t) + sizeof(pspl_hash)) * ctx->stubs_count;
    
    uint32_t stub_data_table_pre_padding = acc;
    acc = ROUND_UP_32(acc);
    stub_data_table_pre_padding = acc - stub_data_table_pre_padding;
    uint32_t extension_name_table_off = acc;
    for (i=0 ; i<ctx->stubs_count ; ++i) {
        pspl_indexer_entry_t* file_ent = ctx->stubs_array[i];
        file_ent->file_off = extension_name_table_off;
        prepare_staged_file(file_ent);
        extension_name_table_off += file_ent->object_len;
        uint32_t padding_diff = extension_name_table_off;
        extension_name_table_off = ROUND_UP_32(extension_name_table_off);
        file_ent->file_padding = extension_name_table_off - padding_diff;
    }
    
    uint32_t platform_name_table_off = extension_name_table_off;
    for (i=0 ; i<ctx->ext_count ; ++i)
        platform_name_table_off += strlen(ctx->ext_array[i]->extension_name) + 1;
    
    acc = platform_name_table_off;
    for (i=0 ; i<ctx->plat_count ; ++i)
        acc += strlen(ctx->plat_array[i]->platform_name) + 1;
    
    // Determine padding length at end
    uint32_t pad_end = ROUND_UP_32(acc);
    pad_end -= acc;
    
    // Populate main header
    pspl_header_t pspl_header = {
        .magic = PSPL_MAGIC_DEF,
        .package_flag = PSPL_PSPLP,
        .version = PSPL_VERSION,
        .endian_flags = psplp_endianness,
    };
    
    // Populate offset header
    pspl_off_header_bi_t pspl_off_header;
    SET_BI_U32(pspl_off_header, extension_name_table_c, ctx->ext_count);
    SET_BI_U32(pspl_off_header, extension_name_table_off, extension_name_table_off);
    SET_BI_U32(pspl_off_header, platform_name_table_c, ctx->plat_count);
    SET_BI_U32(pspl_off_header, platform_name_table_off, platform_name_table_off);
    SET_BI_U32(pspl_off_header, file_table_c, ctx->stubs_count);
    SET_BI_U32(pspl_off_header, file_table_off, file_stub_array_off);
    
    // Write main header
    fwrite(&pspl_header, 1, sizeof(pspl_header_t), psplp_file_out);
    
    // Write offset header
    switch (psplp_endianness) {
        case PSPL_LITTLE_ENDIAN:
            fwrite(&pspl_off_header.little, 1, sizeof(pspl_off_header_t), psplp_file_out);
            break;
        case PSPL_BIG_ENDIAN:
            fwrite(&pspl_off_header.big, 1, sizeof(pspl_off_header_t), psplp_file_out);
            break;
        case PSPL_BI_ENDIAN:
            fwrite(&pspl_off_header, 1, sizeof(pspl_off_header_bi_t), psplp_file_out);
            break;
        default:
            break;
    }
    
    // Write PSPLP header
    pspl_psplp_header_bi_t psplp_head;
    SET_BI_U32(psplp_head, psplc_count, ctx->indexer_count);
    switch (psplp_endianness) {
        case PSPL_LITTLE_ENDIAN:
            fwrite(&psplp_head.little, 1, sizeof(pspl_psplp_header_t), psplp_file_out);
            break;
        case PSPL_BIG_ENDIAN:
            fwrite(&psplp_head.big, 1, sizeof(pspl_psplp_header_t), psplp_file_out);
            break;
        case PSPL_BI_ENDIAN:
            fwrite(&psplp_head, 1, sizeof(pspl_psplp_header_bi_t), psplp_file_out);
            break;
        default:
            break;
    }
    
    // Write PSPLC hash and base offsets for indexers
    for (i=0 ; i<ctx->indexer_count ; ++i) {
        pspl_indexer_context_t* indexer = ctx->indexer_array[i];
        fwrite(&indexer->psplc_hash, 1, sizeof(pspl_hash), psplp_file_out);
        pspl_psplp_psplc_index_bi_t psplc_record;
        SET_BI_U32(psplc_record, psplc_base, indexer->extension_obj_base_off);
        SET_BI_U32(psplc_record, psplc_tables_len,
                   indexer->extension_obj_data_off - indexer->extension_obj_base_off);
        SET_BI_U32(psplc_record, psplc_blobs_len, indexer->extension_obj_blobs_len);
        switch (psplp_endianness) {
            case PSPL_LITTLE_ENDIAN:
                fwrite(&psplc_record.little, 1, sizeof(pspl_psplp_psplc_index_t), psplp_file_out);
                break;
            case PSPL_BIG_ENDIAN:
                fwrite(&psplc_record.big, 1, sizeof(pspl_psplp_psplc_index_t), psplp_file_out);
                break;
            case PSPL_BI_ENDIAN:
                fwrite(&psplc_record, 1, sizeof(pspl_psplp_psplc_index_bi_t), psplp_file_out);
                break;
            default:
                break;
        }
    }
    
    // Perform bare file write
    for (i=0 ; i<ctx->indexer_count ; ++i) {
        pspl_indexer_context_t* indexer = ctx->indexer_array[i];
        pspl_indexer_write_psplc_bare(indexer, psplp_endianness, (pspl_indexer_globals_t*)ctx, psplp_file_out);
    }
    
    
    // Populate and write all file-stub objects
    for (i=0 ; i<ctx->stubs_count ; ++i) {
        const pspl_indexer_entry_t* ent = ctx->stubs_array[i];
        
        pspl_file_stub_bi_t stub_record;
        SET_BI_U32(stub_record, platform_availability_bits,
                   union_plat_bits((pspl_indexer_globals_t*)ctx, ent->parent, ent->platform_availability_bits));
        SET_BI_U32(stub_record, file_off, ent->file_off);
        SET_BI_U32(stub_record, file_len, (uint32_t)ent->object_len);
        
        // Write data hash
        fwrite(&ent->object_hash, 1, sizeof(pspl_hash), psplp_file_out);
        
        // Write structure
        switch (psplp_endianness) {
            case PSPL_LITTLE_ENDIAN:
                fwrite(&stub_record.little, 1, sizeof(pspl_file_stub_t), psplp_file_out);
                break;
            case PSPL_BIG_ENDIAN:
                fwrite(&stub_record.big, 1, sizeof(pspl_file_stub_t), psplp_file_out);
                break;
            case PSPL_BI_ENDIAN:
                fwrite(&stub_record, 1, sizeof(pspl_file_stub_bi_t), psplp_file_out);
                break;
            default:
                break;
        }
        
    }
    
    
    // Data blob pre-padding
    for (i=0 ; i<stub_data_table_pre_padding ; ++i)
        fwrite("", 1, 1, psplp_file_out);
    
    // Write all file data blobs
    for (i=0 ; i<ctx->stubs_count ; ++i) {
        const pspl_indexer_entry_t* ent = ctx->stubs_array[i];
        
        // Copy in file data
        uint8_t buf[8196];
        size_t rem_len = ent->object_len;
        size_t read_len;
        FILE* file = fopen(ent->file_path, "r");
        if (!file)
            pspl_error(-1, "File suddenly unavailable",
                       "`%s` is unable to be re-opened; errno %d (%s)",
                       ent->file_path, errno, strerror(errno));
        do {
            read_len = fread(buf, 1, (rem_len>8196)?8196:rem_len, file);
            if (!read_len)
                pspl_error(-1, "Unexpected end of staged file",
                           "`%s` ended with %zu bytes to go",
                           ent->file_path, rem_len);
            fwrite(buf, 1, read_len, psplp_file_out);
            rem_len -= read_len;
        } while (rem_len > 0);
        fclose(file);
        
        // Post padding
        for (j=0 ; j<ent->file_padding ; ++j)
            fwrite("", 1, 1, psplp_file_out);
        
    }
    
    // Write all Extension name string blobs
    for (i=0 ; i<ctx->ext_count ; ++i) {
        const pspl_extension_t* ext = ctx->ext_array[i];
        fwrite(ext->extension_name, 1, strlen(ext->extension_name)+1, psplp_file_out);
    }
    
    // Write all Platform name string blobs
    for (i=0 ; i<ctx->plat_count ; ++i) {
        const pspl_platform_t* plat = ctx->plat_array[i];
        fwrite(plat->platform_name, 1, strlen(plat->platform_name)+1, psplp_file_out);
    }
    
    // Pad with 0xFF
    char ff = 0xff;
    for (i=0 ; i<pad_end ; ++i)
        fwrite(&ff, 1, 1, psplp_file_out);
    
}

