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
#ifndef _WIN32
#include <sys/uio.h>
#endif
#include <unistd.h>
#include <errno.h>
#include <dirent.h>

#include <PSPLInternal.h>
#include <PSPLHash.h>

#include "ObjectIndexer.h"
#include "Driver.h"
#include "ReferenceGatherer.h"

#ifdef _WIN32
#define STAT_A_NEWER_B(a,b) (a.st_mtime > b.st_mtime)
#define TIME_A_NEWER_B(a,b) (a > b)
#else
#define STAT_A_NEWER_B(a,b) ((a.st_mtimespec.tv_sec > b.st_mtimespec.tv_sec) || \
                             (a.st_mtimespec.tv_sec == b.st_mtimespec.tv_sec && \
                              a.st_mtimespec.tv_nsec > b.st_mtimespec.tv_nsec))
#define TIME_A_NEWER_B(a,b) ((a.tv_sec > b.tv_sec) || \
                             (a.tv_sec == b.tv_sec && \
                              a.tv_nsec > b.tv_nsec))
#endif

#define PSPL_INDEXER_INITIAL_CAP 50

/* Round up to nearest 4 multiple */
#define ROUND_UP_4(val) ((val)%4)?((((val)>>2)<<2)+4):(val)

/* Round up to nearest 32 multiple */
#define ROUND_UP_32(val) ((val)%32)?((((val)>>5)<<5)+32):(val)

#pragma mark File Utilities

/* Determine if referenced file is newer than last-known hashed binary object (For PSPLCs) */
static int is_psplc_ref_newer_than_staged_output(const char* abs_ref_path,
                                                 const char* abs_ref_path_ext,
                                                 const pspl_hash* hash) {
    // Stat source
    struct stat src_stat;
    if (stat(abs_ref_path, &src_stat))
        return 1;
    
    // Hash path
    char ext_path_str[MAXPATHLEN];
    ext_path_str[0] = '\0';
    strlcat(ext_path_str, abs_ref_path, MAXPATHLEN);
    if (abs_ref_path_ext) {
        strlcat(ext_path_str, ":", MAXPATHLEN);
        strlcat(ext_path_str, abs_ref_path_ext, MAXPATHLEN);
    }
    pspl_hash_ctx_t hash_ctx;
    pspl_hash_init(&hash_ctx);
    pspl_hash_write(&hash_ctx, ext_path_str, strlen(ext_path_str));
    pspl_hash* path_hash;
    pspl_hash_result(&hash_ctx, path_hash);
    char abs_ref_path_hash_str[PSPL_HASH_STRING_LEN];
    pspl_hash_fmt(abs_ref_path_hash_str, path_hash);
    
    // Stat object
    char obj_path[MAXPATHLEN];
    obj_path[0] = '\0';
    strlcat(obj_path, driver_state.staging_path, MAXPATHLEN);
    strlcat(obj_path, abs_ref_path_hash_str, MAXPATHLEN);
    strlcat(obj_path, "_", MAXPATHLEN);
    char data_hash_str[PSPL_HASH_STRING_LEN];
    pspl_hash_fmt(data_hash_str, hash);
    strlcat(obj_path, data_hash_str, MAXPATHLEN);
    struct stat obj_stat;
    if (stat(obj_path, &obj_stat))
        return 1;
    
    // Compare modtimes
    if (STAT_A_NEWER_B(src_stat, obj_stat))
        return 1;
    
    return 0;
}

/* Determine if referenced file is newer than its corresponding staged output 
 * Also copies path hash string out */
#ifdef _WIN32
#define NEWEST_MATCH_MTIME(record) record
#define MTIME(record) record.st_mtime
#else
#define NEWEST_MATCH_MTIME(record) record.tv_sec
#define MTIME(record) record.st_mtimespec
#endif
static int is_source_ref_newer_than_staged_output(const char* abs_ref_path,
                                                  const char* abs_ref_path_ext,
                                                  char* abs_ref_path_hash_str_out,
                                                  char* newest_ref_data_hash_str_out,
                                                  int delete_if_newer) {

    // First stat ref path
    struct stat ref_stat;
    if (stat(abs_ref_path, &ref_stat))
        pspl_error(-1, "Unable to stat referenced file",
                   "file `%s` couldn't be staged; errno: %d (%s)",
                   abs_ref_path, errno, strerror(errno));
    
    // Hash path
    char ext_path_str[MAXPATHLEN];
    ext_path_str[0] = '\0';
    strlcat(ext_path_str, abs_ref_path, MAXPATHLEN);
    if (abs_ref_path_ext) {
        strlcat(ext_path_str, ":", MAXPATHLEN);
        strlcat(ext_path_str, abs_ref_path_ext, MAXPATHLEN);
    }
    pspl_hash_ctx_t hash_ctx;
    pspl_hash_init(&hash_ctx);
    pspl_hash_write(&hash_ctx, ext_path_str, strlen(ext_path_str));
    pspl_hash* path_hash;
    pspl_hash_result(&hash_ctx, path_hash);
    pspl_hash_fmt(abs_ref_path_hash_str_out, path_hash);
    
    // Use hash path to find newest existing staged output
#   ifdef _WIN32
    time_t newest_match_mtime = 0;
#   else
    struct timespec newest_match_mtime = {0,0};
#   endif
    char matched_path[MAXPATHLEN];
    struct stat matched_stat;
    DIR* staging_dir = opendir(driver_state.staging_path);
    if (!staging_dir)
        pspl_error(-1, "Unable to open staging directory",
                   "error opening `%s`", driver_state.staging_path);
    
    struct dirent* dent;
    while ((dent = readdir(staging_dir))) {
#       ifdef _WIN32
        if (dent->d_namlen >= PSPL_HASH_STRING_LEN-1)
#       else
        if (dent->d_type == DT_REG && dent->d_namlen >= PSPL_HASH_STRING_LEN-1)
#       endif
            if (!strncmp(abs_ref_path_hash_str_out, dent->d_name, PSPL_HASH_STRING_LEN-1)) {
                // Found a match, stat it and set newest
                matched_path[0] = '\0';
                strlcat(matched_path, driver_state.staging_path, MAXPATHLEN);
                strlcat(matched_path, dent->d_name, MAXPATHLEN);
                if (stat(matched_path, &matched_stat))
                    pspl_error(-1, "Unable to stat matched staged output",
                               "while staging `%s`, unable to stat matched output `%s`; "
                               "errno: %d (%s)",
                               abs_ref_path, matched_path, errno, strerror(errno));
                if (TIME_A_NEWER_B(MTIME(matched_stat), newest_match_mtime))
                    newest_match_mtime = MTIME(matched_stat);
            }
    }
    
    // Write out newest data hash (if needed)
    if (newest_ref_data_hash_str_out && NEWEST_MATCH_MTIME(newest_match_mtime)) {
        char* d_hash = strrchr(matched_path, '_')+1;
        strncpy(newest_ref_data_hash_str_out, d_hash, PSPL_HASH_STRING_LEN-1);
        newest_ref_data_hash_str_out[PSPL_HASH_STRING_LEN-1] = '\0';
    }
    
    // Now see if ref is newer
    int is_newer = 0;
    if (TIME_A_NEWER_B(MTIME(ref_stat), newest_match_mtime)) {
        // It's newer; delete matched files in staging area (they're invalidated)
        is_newer = 1;
        if (delete_if_newer) {
            rewinddir(staging_dir);
            while ((dent = readdir(staging_dir))) {
#               ifdef _WIN32
                if (dent->d_namlen >= PSPL_HASH_STRING_LEN-1)
#               else
                if (dent->d_type == DT_REG && dent->d_namlen >= PSPL_HASH_STRING_LEN-1)
#               endif
                    if (!strncmp(abs_ref_path_hash_str_out, dent->d_name, PSPL_HASH_STRING_LEN-1)) {
                        // Found a match, delete it
                        matched_path[0] = '\0';
                        strlcat(matched_path, driver_state.staging_path, MAXPATHLEN);
                        strlcat(matched_path, dent->d_name, MAXPATHLEN);
                        unlink(matched_path);
                    }
            }
        }
    }
    
    closedir(staging_dir);
    
    return is_newer;
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
static struct {
    uint8_t last_prog;
    const char* path;
} copy_state;
static void pspl_copy_progress_update(double progress) {
    uint8_t prog_int = progress*100;
    if (prog_int == copy_state.last_prog)
        return; // Ease load on terminal if nothing is textually changing
    if (xterm_colour)
        fprintf(stderr, "\r"BOLD"[");
    else
        fprintf(stderr, "\r[");
    if (prog_int >= 100)
        fprintf(stderr, "100");
    else if (prog_int >= 10)
        fprintf(stderr, " %u", prog_int);
    else
        fprintf(stderr, "  %u", prog_int);
    if (xterm_colour)
        fprintf(stderr, "%c]"NORMAL" "GREEN"Copying "BOLD"%s"SGR0, '%', copy_state.path);
    else
        fprintf(stderr, "%c] Copying `%s`", '%', copy_state.path);
    copy_state.last_prog = prog_int;
}
static int copy_file(const char* dest_path, const char* src_path) {
    // Ensure the file is a regular file
    struct stat file_stat;
    if (stat(src_path, &file_stat))
        return -1;
    if (!(file_stat.st_mode & S_IFREG))
        return -1;
    
    copy_state.last_prog = 1;
    pspl_copy_progress_update(0);
    int in_fd = open(src_path, O_RDONLY);
    if (in_fd < 0)
        return -1;
    off_t len = lseek(in_fd, 0, SEEK_END);
    off_t cur = 0;
    lseek(in_fd, 0, SEEK_SET);
    
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
        cur += result;
        pspl_copy_progress_update((double)cur/(double)len);
    }
    
    close(in_fd);
    close(out_fd);
    pspl_copy_progress_update(1);
    fprintf(stderr, "\n");
    return 0;
}


#pragma mark Indexer API

/* Initialise indexer context */
void pspl_indexer_init(pspl_indexer_context_t* ctx,
                       unsigned int max_extension_count,
                       unsigned int max_platform_count) {
    
    ctx->ext_count = 0;
    ctx->ext_array = calloc(max_extension_count, sizeof(pspl_extension_t*));
    
    ctx->plat_count = 0;
    ctx->plat_array = calloc(max_platform_count, sizeof(pspl_platform_t*));
    
    ctx->h_objects_count = 0;
    ctx->h_objects_cap = PSPL_INDEXER_INITIAL_CAP;
    ctx->h_objects_array = calloc(PSPL_INDEXER_INITIAL_CAP, sizeof(pspl_indexer_entry_t*));
    
    ctx->i_objects_count = 0;
    ctx->i_objects_cap = PSPL_INDEXER_INITIAL_CAP;
    ctx->i_objects_array = calloc(PSPL_INDEXER_INITIAL_CAP, sizeof(pspl_indexer_entry_t*));
    
    ctx->ph_objects_count = 0;
    ctx->ph_objects_cap = PSPL_INDEXER_INITIAL_CAP;
    ctx->ph_objects_array = calloc(PSPL_INDEXER_INITIAL_CAP, sizeof(pspl_indexer_entry_t*));
    
    ctx->pi_objects_count = 0;
    ctx->pi_objects_cap = PSPL_INDEXER_INITIAL_CAP;
    ctx->pi_objects_array = calloc(PSPL_INDEXER_INITIAL_CAP, sizeof(pspl_indexer_entry_t*));
    
    ctx->stubs_count = 0;
    ctx->stubs_cap = PSPL_INDEXER_INITIAL_CAP;
    ctx->stubs_array = calloc(PSPL_INDEXER_INITIAL_CAP, sizeof(pspl_indexer_entry_t*));
    
}

/* Platform array from PSPLC availability bits */
static void __plat_array_from_bits(const pspl_platform_t** plats_out,
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
                                                    const pspl_platform_t** plats, pspl_hash* key_hash,
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
                      hash_fmt, ctx->h_objects_array[i]->parent->definer, definer->file_path, owner->extension_name);
        }
    
    // Allocate and add
    ++ctx->h_objects_count;
    if (ctx->h_objects_count >= ctx->h_objects_cap) {
        ctx->h_objects_cap *= 2;
        ctx->h_objects_array = realloc(ctx->h_objects_array, sizeof(pspl_indexer_entry_t*)*ctx->h_objects_cap);
    }
    pspl_indexer_entry_t* new_entry = malloc(sizeof(pspl_indexer_entry_t));
    new_entry->parent = ctx;
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
                if (ctx->plat_array[i] == *plats)
                    break;
            if (i == ctx->plat_count)
                ctx->plat_array[ctx->plat_count++] = *plats;
            new_entry->platform_availability_bits |= 1<<i;
        }
    } else
        new_entry->platform_availability_bits = ~0;
    
    // Populate structure
    new_entry->parent->definer = definer->file_path;
    new_entry->owner_ext = owner;
    pspl_hash_cpy(&new_entry->object_hash, key_hash);
    new_entry->object_len = data_len;
    new_entry->object_little_data = little_data;
    new_entry->object_big_data = big_data;
    
}

/* Augment indexer context with embedded integer-indexed object (from PSPLC) */
static void __pspl_indexer_integer_object_post_augment(pspl_indexer_context_t* ctx, const pspl_extension_t* owner,
                                                       const pspl_platform_t** plats, uint32_t key,
                                                       const void* little_data, const void* big_data, size_t data_len,
                                                       pspl_toolchain_driver_psplc_t* definer) {
    int i;
    
    // Ensure object doesn't already exist
    for (i=0 ; i<ctx->i_objects_count ; ++i)
        if (ctx->i_objects_array[i]->object_index == key &&
            ctx->i_objects_array[i]->owner_ext == owner) {
            pspl_error(-1, "PSPLC Embedded object multiply defined",
                       "indexed object `%u` previously defined in `%s` as well as `%s` for extension `%s`",
                       key, ctx->i_objects_array[i]->parent->definer, definer->file_path, owner->extension_name);
        }
    
    // Allocate and add
    ++ctx->i_objects_count;
    if (ctx->i_objects_count >= ctx->i_objects_cap) {
        ctx->i_objects_cap *= 2;
        ctx->i_objects_array = realloc(ctx->i_objects_array, sizeof(pspl_indexer_entry_t*)*ctx->i_objects_cap);
    }
    pspl_indexer_entry_t* new_entry = malloc(sizeof(pspl_indexer_entry_t));
    new_entry->parent = ctx;
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
    new_entry->parent->definer = definer->file_path;
    new_entry->owner_ext = owner;
    new_entry->object_index = key;
    new_entry->object_len = data_len;
    new_entry->object_little_data = little_data;
    new_entry->object_big_data = big_data;
    
}

/* Augment indexer context with embedded hash-indexed object (from PSPLC) */
static void __pspl_indexer_platform_hash_object_post_augment(pspl_indexer_context_t* ctx, const pspl_platform_t* owner,
                                                             pspl_hash* key_hash, const void* little_data,
                                                             const void* big_data, size_t data_len,
                                                             pspl_toolchain_driver_psplc_t* definer) {
    int i;
    
    // Ensure object doesn't already exist
    for (i=0 ; i<ctx->ph_objects_count ; ++i)
        if (!pspl_hash_cmp(&ctx->ph_objects_array[i]->object_hash, key_hash) &&
            ctx->ph_objects_array[i]->owner_plat == owner) {
            char hash_fmt[PSPL_HASH_STRING_LEN];
            pspl_hash_fmt(hash_fmt, key_hash);
            pspl_error(-1, "PSPLC Embedded platform object multiply defined",
                       "object `%s` previously defined in `%s` as well as `%s` for platform `%s`",
                       hash_fmt, ctx->ph_objects_array[i]->parent->definer, definer->file_path, owner->platform_name);
        }
    
    // Allocate and add
    ++ctx->ph_objects_count;
    if (ctx->ph_objects_count >= ctx->ph_objects_cap) {
        ctx->ph_objects_cap *= 2;
        ctx->ph_objects_array = realloc(ctx->ph_objects_array, sizeof(pspl_indexer_entry_t*)*ctx->ph_objects_cap);
    }
    pspl_indexer_entry_t* new_entry = malloc(sizeof(pspl_indexer_entry_t));
    new_entry->parent = ctx;
    ctx->ph_objects_array[ctx->ph_objects_count-1] = new_entry;
    
    // Ensure platform is added
    for (i=0 ; i<ctx->plat_count ; ++i)
        if (ctx->plat_array[i] == owner)
            break;
    if (i == ctx->plat_count)
        ctx->plat_array[ctx->plat_count++] = owner;
    
    // Populate structure
    new_entry->parent->definer = definer->file_path;
    new_entry->owner_plat = owner;
    pspl_hash_cpy(&new_entry->object_hash, key_hash);
    new_entry->object_len = data_len;
    new_entry->object_little_data = little_data;
    new_entry->object_big_data = big_data;
    
}

/* Augment indexer context with embedded integer-indexed object (from PSPLC) */
static void __pspl_indexer_platform_integer_object_post_augment(pspl_indexer_context_t* ctx, const pspl_platform_t* owner,
                                                                uint32_t key, const void* little_data,
                                                                const void* big_data, size_t data_len,
                                                                pspl_toolchain_driver_psplc_t* definer) {
    int i;
    
    // Ensure object doesn't already exist
    for (i=0 ; i<ctx->pi_objects_count ; ++i)
        if (ctx->pi_objects_array[i]->object_index == key &&
            ctx->pi_objects_array[i]->owner_plat == owner) {
            pspl_error(-1, "PSPLC Embedded platform object multiply defined",
                       "object `%u` previously defined in `%s` as well as `%s` for platform `%s`",
                       key, ctx->pi_objects_array[i]->parent->definer, definer->file_path, owner->platform_name);
        }
    
    // Allocate and add
    ++ctx->pi_objects_count;
    if (ctx->pi_objects_count >= ctx->pi_objects_cap) {
        ctx->pi_objects_cap *= 2;
        ctx->pi_objects_array = realloc(ctx->pi_objects_array, sizeof(pspl_indexer_entry_t*)*ctx->pi_objects_cap);
    }
    pspl_indexer_entry_t* new_entry = malloc(sizeof(pspl_indexer_entry_t));
    new_entry->parent = ctx;
    ctx->pi_objects_array[ctx->pi_objects_count-1] = new_entry;
    
    // Ensure platform is added
    for (i=0 ; i<ctx->plat_count ; ++i)
        if (ctx->plat_array[i] == owner)
            break;
    if (i == ctx->plat_count)
        ctx->plat_array[ctx->plat_count++] = owner;
    
    // Populate structure
    new_entry->parent->definer = definer->file_path;
    new_entry->owner_plat = owner;
    new_entry->object_index = key;
    new_entry->object_len = data_len;
    new_entry->object_little_data = little_data;
    new_entry->object_big_data = big_data;
    
}

/* Augment indexer context with file stub (from PSPLC) */
static void __pspl_indexer_stub_file_post_augment(pspl_indexer_context_t* ctx,
                                                  const pspl_platform_t** plats, const char* path_in,
                                                  const char* path_ext_in,
                                                  pspl_hash* hash_in, pspl_toolchain_driver_psplc_t* definer) {
    int i;
    
    // Ensure object doesn't already exist
    for (i=0 ; i<ctx->stubs_count ; ++i)
        if (!strcmp(ctx->stubs_array[i]->stub_source_path, path_in))
            return;
    
    // Warn user if the source ref is newer than the newest object (necessitating recompile)
    if (is_psplc_ref_newer_than_staged_output(path_in, path_ext_in, hash_in))
        pspl_warn("Newer data source detected",
                  "PSPL detected that `%s` has changed since `%s` was last compiled. "
                  "Old converted object will be used. Recompile if this is not desired.",
                  path_in, definer->file_path);
    
    // Allocate and add
    ++ctx->stubs_count;
    if (ctx->stubs_count >= ctx->stubs_cap) {
        ctx->stubs_cap *= 2;
        ctx->stubs_array = realloc(ctx->stubs_array, sizeof(pspl_indexer_entry_t*)*ctx->stubs_cap);
    }
    pspl_indexer_entry_t* new_entry = malloc(sizeof(pspl_indexer_entry_t));
    new_entry->parent = ctx;
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
    new_entry->parent->definer = definer->file_path;
    pspl_hash_cpy(&new_entry->object_hash, hash_in);
    size_t cpy_len = strlen(path_in)+1;
    char* cpy_path = malloc(cpy_len);
    strncpy(cpy_path, path_in, cpy_len);
    new_entry->stub_source_path = cpy_path;
    new_entry->stub_source_path_ext = NULL;
    if (path_ext_in) {
        cpy_len = strlen(path_ext_in)+1;
        cpy_path = malloc(cpy_len);
        strncpy(cpy_path, path_ext_in, cpy_len);
        new_entry->stub_source_path_ext = cpy_path;
    }
}

/* Augment indexer context with stub entries
 * from an existing PSPLC file */
#define IS_PSPLC_BI pspl_head->endian_flags == PSPL_BI_ENDIAN
#if __LITTLE_ENDIAN__
#define IS_PSPLC_SWAPPED pspl_head->endian_flags == PSPL_BIG_ENDIAN
#elif __BIG_ENDIAN__
#define IS_PSPLC_SWAPPED pspl_head->endian_flags == PSPL_LITTLE_ENDIAN
#endif
void pspl_indexer_psplc_stub_augment(pspl_indexer_context_t* ctx,
                                     pspl_toolchain_driver_psplc_t* existing_psplc) {
    
    int i,j;
    void* psplc_cur = (void*)existing_psplc->psplc_data;
    
    // Main header
    pspl_header_t* pspl_head = (pspl_header_t*)psplc_cur;
    psplc_cur += sizeof(pspl_header_t);
    check_psplc_underflow(existing_psplc, psplc_cur);
    
    // Offset header
    pspl_off_header_t* pspl_off_head = NULL;
    if (IS_PSPLC_BI) {
        pspl_off_header_bi_t* bi_head = psplc_cur;
        check_psplc_underflow(existing_psplc, bi_head+sizeof(pspl_off_header_bi_t));
        pspl_off_head = &bi_head->native;
        psplc_cur += sizeof(pspl_off_header_bi_t);
    } else {
        // Driver already swapped this (if non-native)
        pspl_off_head = psplc_cur;
        check_psplc_underflow(existing_psplc, pspl_off_head+sizeof(pspl_off_header_t));
        psplc_cur += sizeof(pspl_off_header_t);
    }
    
    // PSPLC name hash
    check_psplc_underflow(existing_psplc, psplc_cur+sizeof(pspl_hash));
    pspl_hash_cpy(&ctx->psplc_hash, psplc_cur);
    psplc_cur += sizeof(pspl_hash);
    
    // PSPLC header
    pspl_psplc_header_t* psplc_head = NULL;
    if (IS_PSPLC_BI) {
        pspl_psplc_header_bi_t* psplc_head_bi = psplc_cur;
        check_psplc_underflow(existing_psplc, psplc_head_bi+sizeof(pspl_psplc_header_bi_t));
        psplc_head = &psplc_head_bi->native;
        psplc_cur += sizeof(pspl_psplc_header_bi_t);
    } else {
        psplc_head = psplc_cur;
        check_psplc_underflow(existing_psplc, psplc_head+sizeof(pspl_psplc_header_t));
        if (IS_PSPLC_SWAPPED)
            SWAP_PSPL_PSPLC_HEADER_T(psplc_head);
        psplc_cur += sizeof(pspl_psplc_header_t);
    }
    
    // First, embedded extension objects
    for (i=0 ; i<psplc_head->extension_count ; ++i) {
        
        pspl_object_array_extension_t* ext = NULL;
        if (IS_PSPLC_BI) {
            pspl_object_array_extension_bi_t* bi_ext = psplc_cur;
            check_psplc_underflow(existing_psplc, bi_ext+sizeof(pspl_object_array_extension_bi_t));
            ext = &bi_ext->native;
            psplc_cur += sizeof(pspl_object_array_extension_bi_t);
        } else {
            ext = psplc_cur;
            check_psplc_underflow(existing_psplc, ext+sizeof(pspl_object_array_extension_t));
            if (IS_PSPLC_SWAPPED)
                SWAP_PSPL_OBJECT_ARRAY_EXTENSION_T(ext);
            psplc_cur += sizeof(pspl_object_array_extension_t);
        }
        
        // Resolve extension
        const pspl_extension_t* extension =
        existing_psplc->required_extension_set[ext->extension_index];
        
        // Hash-indexed objects
        void* h_obj_cur = existing_psplc->psplc_data + ext->ext_hash_indexed_object_array_off;
        for (j=0 ; j<ext->ext_hash_indexed_object_count ; ++j) {
            
            // Object Hash
            pspl_hash* hash = h_obj_cur;
            check_psplc_underflow(existing_psplc, hash+sizeof(pspl_hash));
            h_obj_cur += sizeof(pspl_hash);
            
            // Object
            pspl_object_hash_record_t* obj = NULL;
            if (IS_PSPLC_BI) {
                pspl_object_hash_record_bi_t* bi_obj = h_obj_cur;
                check_psplc_underflow(existing_psplc, bi_obj+sizeof(pspl_object_hash_record_bi_t));
                obj = &bi_obj->native;
                h_obj_cur += sizeof(pspl_object_hash_record_bi_t);
            } else {
                obj = h_obj_cur;
                check_psplc_underflow(existing_psplc, obj+sizeof(pspl_object_hash_record_t));
                if (IS_PSPLC_SWAPPED)
                    SWAP_PSPL_OBJECT_HASH_RECORD_T(obj);
                h_obj_cur += sizeof(pspl_object_hash_record_t);
            }
            
            // Integrate object
            const pspl_platform_t* plat_arr[PSPL_MAX_PLATFORMS+1];
            __plat_array_from_bits(plat_arr, existing_psplc, obj->platform_availability_bits);
            void* little_data = (existing_psplc->psplc_data + obj->object_off);
            void* big_data = (little_data + ((obj->object_bi)?ROUND_UP_4(obj->object_len):0));
            check_psplc_underflow(existing_psplc, big_data+obj->object_len);
            __pspl_indexer_hash_object_post_augment(ctx, extension, plat_arr, hash,
                                                    little_data, big_data, obj->object_len,
                                                    existing_psplc);
            
        }
        
        // Integer-indexed objects
        void* i_obj_cur = existing_psplc->psplc_data + ext->ext_int_indexed_object_array_off;
        for (j=0 ; j<ext->ext_int_indexed_object_count ; ++j) {
            
            pspl_object_int_record_t* obj = NULL;
            if (IS_PSPLC_BI) {
                pspl_object_int_record_bi_t* bi_obj = i_obj_cur;
                check_psplc_underflow(existing_psplc, bi_obj+sizeof(pspl_object_int_record_bi_t));
                obj = &bi_obj->native;
                i_obj_cur += sizeof(pspl_object_int_record_bi_t);
            } else {
                obj = i_obj_cur;
                check_psplc_underflow(existing_psplc, obj+sizeof(pspl_object_int_record_t));
                if (IS_PSPLC_SWAPPED)
                    SWAP_PSPL_OBJECT_HASH_RECORD_T(obj);
                i_obj_cur += sizeof(pspl_object_int_record_t);
            }
            
            // Integrate object
            const pspl_platform_t* plat_arr[PSPL_MAX_PLATFORMS+1];
            __plat_array_from_bits(plat_arr, existing_psplc, obj->platform_availability_bits);
            void* little_data = (existing_psplc->psplc_data + obj->object_off);
            void* big_data = (little_data + ((obj->object_bi)?ROUND_UP_4(obj->object_len):0));
            check_psplc_underflow(existing_psplc, big_data+obj->object_len);
            __pspl_indexer_integer_object_post_augment(ctx, extension, plat_arr, obj->object_index,
                                                       little_data, big_data, obj->object_len,
                                                       existing_psplc);
        }
        
    }
    
    // Next, embedded platform objects
    for (i=0 ; i<psplc_head->platform_count ; ++i) {
        
        pspl_object_array_extension_t* ext = NULL;
        if (IS_PSPLC_BI) {
            pspl_object_array_extension_bi_t* bi_ext = psplc_cur;
            check_psplc_underflow(existing_psplc, bi_ext+sizeof(pspl_object_array_extension_bi_t));
            ext = &bi_ext->native;
            psplc_cur += sizeof(pspl_object_array_extension_bi_t);
        } else {
            ext = psplc_cur;
            check_psplc_underflow(existing_psplc, ext+sizeof(pspl_object_array_extension_t));
            if (IS_PSPLC_SWAPPED)
                SWAP_PSPL_OBJECT_ARRAY_EXTENSION_T(ext);
            psplc_cur += sizeof(pspl_object_array_extension_t);
        }
        
        // Resolve platform
        const pspl_platform_t* platform =
        existing_psplc->required_platform_set[ext->platform_index];
        
        // Hash-indexed objects
        void* h_obj_cur = existing_psplc->psplc_data + ext->ext_hash_indexed_object_array_off;
        for (j=0 ; j<ext->ext_hash_indexed_object_count ; ++j) {
            
            // Object Hash
            pspl_hash* hash = h_obj_cur;
            check_psplc_underflow(existing_psplc, hash+sizeof(pspl_hash));
            h_obj_cur += sizeof(pspl_hash);
            
            // Object
            pspl_object_hash_record_t* obj = NULL;
            if (IS_PSPLC_BI) {
                pspl_object_hash_record_bi_t* bi_obj = h_obj_cur;
                check_psplc_underflow(existing_psplc, bi_obj+sizeof(pspl_object_hash_record_bi_t));
                obj = &bi_obj->native;
                h_obj_cur += sizeof(pspl_object_hash_record_bi_t);
            } else {
                obj = h_obj_cur;
                check_psplc_underflow(existing_psplc, obj+sizeof(pspl_object_hash_record_t));
                if (IS_PSPLC_SWAPPED)
                    SWAP_PSPL_OBJECT_HASH_RECORD_T(obj);
                h_obj_cur += sizeof(pspl_object_hash_record_t);
            }
            
            // Integrate object
            const pspl_platform_t* plat_arr[PSPL_MAX_PLATFORMS+1];
            __plat_array_from_bits(plat_arr, existing_psplc, obj->platform_availability_bits);
            void* little_data = (existing_psplc->psplc_data + obj->object_off);
            void* big_data = (little_data + ((obj->object_bi)?ROUND_UP_4(obj->object_len):0));
            check_psplc_underflow(existing_psplc, big_data+obj->object_len);
            __pspl_indexer_platform_hash_object_post_augment(ctx, platform, hash,
                                                             little_data, big_data, obj->object_len,
                                                             existing_psplc);
            
        }
        
        // Integer-indexed objects
        void* i_obj_cur = existing_psplc->psplc_data + ext->ext_int_indexed_object_array_off;
        for (j=0 ; j<ext->ext_int_indexed_object_count ; ++j) {
            
            pspl_object_int_record_t* obj = NULL;
            if (IS_PSPLC_BI) {
                pspl_object_int_record_bi_t* bi_obj = i_obj_cur;
                check_psplc_underflow(existing_psplc, bi_obj+sizeof(pspl_object_int_record_bi_t));
                obj = &bi_obj->native;
                i_obj_cur += sizeof(pspl_object_int_record_bi_t);
            } else {
                obj = i_obj_cur;
                check_psplc_underflow(existing_psplc, obj+sizeof(pspl_object_int_record_t));
                if (IS_PSPLC_SWAPPED)
                    SWAP_PSPL_OBJECT_HASH_RECORD_T(obj);
                i_obj_cur += sizeof(pspl_object_int_record_t);
            }
            
            // Integrate object
            const pspl_platform_t* plat_arr[PSPL_MAX_PLATFORMS+1];
            __plat_array_from_bits(plat_arr, existing_psplc, obj->platform_availability_bits);
            void* little_data = (existing_psplc->psplc_data + obj->object_off);
            void* big_data = (little_data + ((obj->object_bi)?ROUND_UP_4(obj->object_len):0));
            check_psplc_underflow(existing_psplc, big_data+obj->object_len);
            __pspl_indexer_platform_integer_object_post_augment(ctx, platform, obj->object_index,
                                                                little_data, big_data, obj->object_len,
                                                                existing_psplc);
        }
        
    }
    
    // Now file-stub arrays
    psplc_cur = existing_psplc->psplc_data + pspl_off_head->file_table_off;
    for (i=0 ; i<pspl_off_head->file_table_c ; ++i) {
        
        // Object Hash
        pspl_hash* hash = psplc_cur;
        check_psplc_underflow(existing_psplc, hash+sizeof(pspl_hash));
        psplc_cur += sizeof(pspl_hash);
        
        // Object
        pspl_file_stub_t* stub = NULL;
        if (IS_PSPLC_BI) {
            pspl_file_stub_bi_t* bi_stub = psplc_cur;
            check_psplc_underflow(existing_psplc, bi_stub+sizeof(pspl_file_stub_bi_t));
            stub = &bi_stub->native;
            psplc_cur += sizeof(pspl_file_stub_bi_t);
        } else {
            stub = psplc_cur;
            check_psplc_underflow(existing_psplc, stub+sizeof(pspl_file_stub_t));
            if (IS_PSPLC_SWAPPED)
                SWAP_PSPL_FILE_STUB_T(stub);
            psplc_cur += sizeof(pspl_file_stub_t);
        }
        
        // Integrate stub
        const pspl_platform_t* plat_arr[PSPL_MAX_PLATFORMS+1];
        __plat_array_from_bits(plat_arr, existing_psplc, stub->platform_availability_bits);
        char* path = (char*)(existing_psplc->psplc_data + stub->file_path_off);
        check_psplc_underflow(existing_psplc, path);
        char* path_ext = NULL;
        if (stub->file_path_ext_off) {
            path_ext = (char*)(existing_psplc->psplc_data + stub->file_path_ext_off);
            check_psplc_underflow(existing_psplc, path_ext);
        }
        __pspl_indexer_stub_file_post_augment(ctx, plat_arr, path, path_ext, hash, existing_psplc);
        
    }
    
}

/* Augment indexer context with embedded hash-indexed object */
void pspl_indexer_hash_object_augment(pspl_indexer_context_t* ctx, const pspl_extension_t* owner,
                                      const pspl_platform_t** plats, const char* key,
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
                       hash_fmt, ctx->h_objects_array[i]->parent->definer, definer->file_path, owner->extension_name);
        }
    
    // Allocate and add
    ++ctx->h_objects_count;
    if (ctx->h_objects_count >= ctx->h_objects_cap) {
        ctx->h_objects_cap *= 2;
        ctx->h_objects_array = realloc(ctx->h_objects_array, sizeof(pspl_indexer_entry_t*)*ctx->h_objects_cap);
    }
    pspl_indexer_entry_t* new_entry = malloc(sizeof(pspl_indexer_entry_t));
    new_entry->parent = ctx;
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
                if (ctx->plat_array[i] == *plats)
                    break;
            if (i == ctx->plat_count)
                ctx->plat_array[ctx->plat_count++] = *plats;
            new_entry->platform_availability_bits |= 1<<i;
            
            // Ensure platform has appropriate data available
            if ((*plats)->byte_order == PSPL_LITTLE_ENDIAN && !little_data)
                pspl_error(-1, "Required embedded object data not provided",
                           "extension embed request specified little-endian platform, "
                           "but didn't provide little-endian data");
            else if ((*plats)->byte_order == PSPL_BIG_ENDIAN && !big_data)
                pspl_error(-1, "Required embedded object data not provided",
                           "extension embed request specified big-endian platform, "
                           "but didn't provide big-endian data");
        }
    } else
        new_entry->platform_availability_bits = ~0;
    
    // Populate structure
    new_entry->parent->definer = definer->file_path;
    new_entry->owner_ext = owner;
    pspl_hash_cpy(&new_entry->object_hash, key_hash);
    new_entry->object_len = data_len;
    void* little_buf = NULL;
    if (data_len && little_data) {
        void* little_buf = malloc(data_len);
        memcpy(little_buf, little_data, data_len);
    }
    void* big_buf = little_buf;
    if (data_len && little_data != big_data) {
        big_buf = malloc(data_len);
        memcpy(big_buf, big_data, data_len);
    }
    new_entry->object_little_data = little_buf;
    new_entry->object_big_data = big_buf;
}

/* Augment indexer context with embedded integer-indexed object */
void pspl_indexer_integer_object_augment(pspl_indexer_context_t* ctx, const pspl_extension_t* owner,
                                         const pspl_platform_t** plats, uint32_t key,
                                         const void* little_data, const void* big_data, size_t data_len,
                                         pspl_toolchain_driver_source_t* definer) {
    int i;
    
    // Ensure object doesn't already exist
    for (i=0 ; i<ctx->i_objects_count ; ++i)
        if (ctx->i_objects_array[i]->object_index == key &&
            ctx->i_objects_array[i]->owner_ext == owner) {
            pspl_error(-1, "PSPLC Embedded object multiply defined",
                       "indexed object `%u` previously defined in `%s` as well as `%s` for extension `%s`",
                       key, ctx->i_objects_array[i]->parent->definer, definer->file_path, owner->extension_name);
        }
    
    // Allocate and add
    ++ctx->i_objects_count;
    if (ctx->i_objects_count >= ctx->i_objects_cap) {
        ctx->i_objects_cap *= 2;
        ctx->i_objects_array = realloc(ctx->i_objects_array, sizeof(pspl_indexer_entry_t*)*ctx->i_objects_cap);
    }
    pspl_indexer_entry_t* new_entry = malloc(sizeof(pspl_indexer_entry_t));
    new_entry->parent = ctx;
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
            
            // Ensure platform has appropriate data available
            if ((*plats)->byte_order == PSPL_LITTLE_ENDIAN && !little_data)
                pspl_error(-1, "Required embedded object data not provided",
                           "extension embed request specified little-endian platform, "
                           "but didn't provide little-endian data");
            else if ((*plats)->byte_order == PSPL_BIG_ENDIAN && !big_data)
                pspl_error(-1, "Required embedded object data not provided",
                           "extension embed request specified big-endian platform, "
                           "but didn't provide big-endian data");
        }
    } else
        new_entry->platform_availability_bits = ~0;
    
    // Populate structure
    new_entry->parent->definer = definer->file_path;
    new_entry->owner_ext = owner;
    new_entry->object_index = key;
    new_entry->object_len = data_len;
    void* little_buf = NULL;
    if (data_len && little_data) {
        void* little_buf = malloc(data_len);
        memcpy(little_buf, little_data, data_len);
    }
    void* big_buf = little_buf;
    if (data_len && little_data != big_data) {
        big_buf = malloc(data_len);
        memcpy(big_buf, big_data, data_len);
    }
    new_entry->object_little_data = little_buf;
    new_entry->object_big_data = big_buf;
}

/* Augment indexer context with embedded hash-indexed platform object */
void pspl_indexer_platform_hash_object_augment(pspl_indexer_context_t* ctx, const pspl_platform_t* owner,
                                               const char* key, const void* little_data,
                                               const void* big_data, size_t data_len,
                                               pspl_toolchain_driver_source_t* definer) {
    int i;
    
    // Make key hash
    pspl_hash_ctx_t hash_ctx;
    pspl_hash_init(&hash_ctx);
    pspl_hash_write(&hash_ctx, key, strlen(key));
    pspl_hash* key_hash;
    pspl_hash_result(&hash_ctx, key_hash);
    
    // Ensure object doesn't already exist
    for (i=0 ; i<ctx->ph_objects_count ; ++i)
        if (!pspl_hash_cmp(&ctx->ph_objects_array[i]->object_hash, key_hash) &&
            ctx->ph_objects_array[i]->owner_plat == owner) {
            char hash_fmt[PSPL_HASH_STRING_LEN];
            pspl_hash_fmt(hash_fmt, key_hash);
            pspl_error(-1, "PSPLC Embedded platform object multiply defined",
                       "object `%s` previously defined in `%s` as well as `%s` for platform `%s`",
                       hash_fmt, ctx->ph_objects_array[i]->parent->definer, definer->file_path, owner->platform_name);
        }
    
    // Allocate and add
    ++ctx->ph_objects_count;
    if (ctx->ph_objects_count >= ctx->ph_objects_cap) {
        ctx->ph_objects_cap *= 2;
        ctx->ph_objects_array = realloc(ctx->ph_objects_array, sizeof(pspl_indexer_entry_t*)*ctx->ph_objects_cap);
    }
    pspl_indexer_entry_t* new_entry = malloc(sizeof(pspl_indexer_entry_t));
    new_entry->parent = ctx;
    ctx->ph_objects_array[ctx->ph_objects_count-1] = new_entry;
    
    // Ensure platform is added
    for (i=0 ; i<ctx->plat_count ; ++i)
        if (ctx->plat_array[i] == owner)
            break;
    if (i == ctx->plat_count)
        ctx->plat_array[ctx->plat_count++] = owner;
    
    // Populate structure
    new_entry->parent->definer = definer->file_path;
    new_entry->owner_plat = owner;
    pspl_hash_cpy(&new_entry->object_hash, key_hash);
    new_entry->object_len = data_len;
    void* little_buf = NULL;
    if (data_len && little_data) {
        void* little_buf = malloc(data_len);
        memcpy(little_buf, little_data, data_len);
    }
    void* big_buf = little_buf;
    if (data_len && little_data != big_data) {
        big_buf = malloc(data_len);
        memcpy(big_buf, big_data, data_len);
    }
    new_entry->object_little_data = little_buf;
    new_entry->object_big_data = big_buf;
    
}

/* Augment indexer context with embedded integer-indexed platform object */
void pspl_indexer_platform_integer_object_augment(pspl_indexer_context_t* ctx, const pspl_platform_t* owner,
                                                  uint32_t key, const void* little_data,
                                                  const void* big_data, size_t data_len,
                                                  pspl_toolchain_driver_source_t* definer) {
    int i;
    
    // Ensure object doesn't already exist
    for (i=0 ; i<ctx->pi_objects_count ; ++i)
        if (ctx->pi_objects_array[i]->object_index == key &&
            ctx->pi_objects_array[i]->owner_plat == owner) {
            pspl_error(-1, "PSPLC Embedded platform object multiply defined",
                       "object `%u` previously defined in `%s` as well as `%s` for platform `%s`",
                       key, ctx->pi_objects_array[i]->parent->definer, definer->file_path, owner->platform_name);
        }
    
    // Allocate and add
    ++ctx->pi_objects_count;
    if (ctx->pi_objects_count >= ctx->pi_objects_cap) {
        ctx->pi_objects_cap *= 2;
        ctx->pi_objects_array = realloc(ctx->pi_objects_array, sizeof(pspl_indexer_entry_t*)*ctx->pi_objects_cap);
    }
    pspl_indexer_entry_t* new_entry = malloc(sizeof(pspl_indexer_entry_t));
    new_entry->parent = ctx;
    ctx->pi_objects_array[ctx->pi_objects_count-1] = new_entry;
    
    // Ensure platform is added
    for (i=0 ; i<ctx->plat_count ; ++i)
        if (ctx->plat_array[i] == owner)
            break;
    if (i == ctx->plat_count)
        ctx->plat_array[ctx->plat_count++] = owner;
    
    // Populate structure
    new_entry->parent->definer = definer->file_path;
    new_entry->owner_plat = owner;
    new_entry->object_index = key;
    new_entry->object_len = data_len;
    void* little_buf = NULL;
    if (data_len && little_data) {
        void* little_buf = malloc(data_len);
        memcpy(little_buf, little_data, data_len);
    }
    void* big_buf = little_buf;
    if (data_len && little_data != big_data) {
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
    const char* path_ext;
} converter_state;
void pspl_converter_progress_update(double progress) {
    uint8_t prog_int = progress*100;
    if (prog_int == converter_state.last_prog)
        return; // Ease load on terminal if nothing is textually changing
    if (xterm_colour)
        fprintf(stderr, "\r"BOLD"[");
    else
        fprintf(stderr, "\r[");
    if (prog_int >= 100)
        fprintf(stderr, "100");
    else if (prog_int >= 10)
        fprintf(stderr, " %u", prog_int);
    else
        fprintf(stderr, "  %u", prog_int);
    if (converter_state.path_ext) {
        if (xterm_colour)
            fprintf(stderr, "%c]"SGR0" "GREEN"Converting "
                    BOLD"%s"NORMAL BOLD":"MAGENTA"%s"SGR0, '%',
                    converter_state.path, converter_state.path_ext);
        else
            fprintf(stderr, "%c] Converting %s:%s", '%',
                    converter_state.path, converter_state.path_ext);
    } else {
        if (xterm_colour)
            fprintf(stderr, "%c]"SGR0" "GREEN"Converting "BOLD"%s"SGR0, '%',
                    converter_state.path);
        else
            fprintf(stderr, "%c] Converting %s", '%', converter_state.path);
    }
    converter_state.last_prog = prog_int;
}
void pspl_indexer_stub_file_augment(pspl_indexer_context_t* ctx,
                                    const pspl_platform_t** plats, const char* path_in,
                                    const char* path_ext_in,
                                    pspl_converter_file_hook converter_hook, uint8_t move_output,
                                    pspl_hash** hash_out,
                                    pspl_toolchain_driver_source_t* definer) {
    converter_state.path = path_in;
    converter_state.path_ext = path_ext_in;
    converter_state.last_prog = 1;
    char abs_path[MAXPATHLEN];
    abs_path[0] = '\0';
    
    // Ensure staging directory exists
    strlcat(abs_path, driver_state.staging_path, MAXPATHLEN);
#   ifdef _WIN32
    if(mkdir(abs_path))
#   else
    if(mkdir(abs_path, 0755))
#   endif
        if (errno != EEXIST)
            pspl_error(-1, "Error creating staging directory",
                       "unable to create `%s` as staging directory; "
                       "errno: %d (%s)",
                       abs_path, errno, strerror(errno));
    
    
    // Make path absolute
    if (*path_in != '/') {
        abs_path[0] = '\0';
        strlcat(abs_path, definer->file_enclosing_dir, MAXPATHLEN);
        strlcat(abs_path, path_in, MAXPATHLEN);
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
        ctx->stubs_array = realloc(ctx->stubs_array, sizeof(pspl_indexer_entry_t*)*ctx->stubs_cap);
    }
    pspl_indexer_entry_t* new_entry = malloc(sizeof(pspl_indexer_entry_t));
    new_entry->parent = ctx;
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
    
    // Determine if reference is newer
    char path_hash_str[PSPL_HASH_STRING_LEN];
    char newest_data_hash_str[PSPL_HASH_STRING_LEN];
    int is_newer = is_source_ref_newer_than_staged_output(path_in, path_ext_in,
                                                          path_hash_str, newest_data_hash_str, 1);
    char sug_path[MAXPATHLEN];
    sug_path[0] = '\0';
    strlcat(sug_path, driver_state.staging_path, MAXPATHLEN);
    strlcat(sug_path, "tmp_", MAXPATHLEN);
    strlcat(sug_path, path_hash_str, MAXPATHLEN);
    
    if (is_newer) {
        
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
            copy_state.path = converter_state.path;
            if(copy_file(sug_path, path_in))
                pspl_error(-1, "Unable to copy file",
                           "unable to copy `%s` during conversion", path_in);
            final_path = sug_path;
        }
        
        // Message path
        if (xterm_colour)
            fprintf(stderr, BOLD"Path Hash: "CYAN"%s"SGR0"\n", path_hash_str);
        else
            fprintf(stderr, "Path Hash: %s\n", path_hash_str);
        
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
        strlcat(final_hash_path_str, driver_state.staging_path, MAXPATHLEN);
        strlcat(final_hash_path_str, path_hash_str, MAXPATHLEN);
        strlcat(final_hash_path_str, "_", MAXPATHLEN);
        strlcat(final_hash_path_str, final_hash_str, MAXPATHLEN);
        
        // Copy (or move)
        if (converter_hook && !move_output) {
            copy_state.path = converter_state.path;
            if(copy_file(final_hash_path_str, final_path))
                pspl_error(-1, "Unable to copy file",
                           "unable to copy `%s` during conversion", final_path);
        } else
            if(rename(final_path, final_hash_path_str))
                pspl_error(-1, "Unable to move file",
                           "unable to move `%s` during conversion", final_path);
        
        // Message data
        if (xterm_colour)
            fprintf(stderr, BOLD"Data Hash: "CYAN"%s"SGR0"\n", final_hash_str);
        else
            fprintf(stderr, "Data Hash: %s\n", final_hash_str);
        
    } else {
        
        // No conversion; just copy newest data hash
        pspl_hash_parse(&new_entry->object_hash, newest_data_hash_str);
        
    }
    
    // Populate structure
    new_entry->parent->definer = definer->file_path;
    size_t cpy_len = strlen(path_in)+1;
    char* cpy_path = malloc(cpy_len);
    strncpy(cpy_path, path_in, cpy_len);
    new_entry->stub_source_path = cpy_path;
    new_entry->stub_source_path_ext = NULL;
    if (path_ext_in) {
        cpy_len = strlen(path_ext_in)+1;
        cpy_path = malloc(cpy_len);
        strncpy(cpy_path, path_ext_in, cpy_len);
        new_entry->stub_source_path_ext = cpy_path;
    }
    if (driver_state.gather_ctx)
        pspl_gather_add_file(driver_state.gather_ctx, cpy_path);
    
    converter_state.path = NULL;
    
}
void pspl_indexer_stub_membuf_augment(pspl_indexer_context_t* ctx,
                                      const pspl_platform_t** plats, const char* path_in,
                                      const char* path_ext_in,
                                      pspl_converter_membuf_hook converter_hook,
                                      pspl_hash** hash_out,
                                      pspl_toolchain_driver_source_t* definer) {
    if (!converter_hook)
        return;
    
    converter_state.path = path_in;
    converter_state.path_ext = path_ext_in;
    converter_state.last_prog = 1;
    char abs_path[MAXPATHLEN];
    abs_path[0] = '\0';
    
    // Ensure staging directory exists
    strlcat(abs_path, driver_state.staging_path, MAXPATHLEN);
#   ifdef _WIN32
    if(mkdir(abs_path))
#   else
    if(mkdir(abs_path, 0755))
#   endif
        if (errno != EEXIST)
            pspl_error(-1, "Error creating staging directory",
                       "unable to create `%s` as staging directory",
                       abs_path);
    
    // Make path absolute
    if (*path_in != '/') {
        abs_path[0] = '\0';
        strlcat(abs_path, definer->file_enclosing_dir, MAXPATHLEN);
        strlcat(abs_path, path_in, MAXPATHLEN);
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
        ctx->stubs_array = realloc(ctx->stubs_array, sizeof(pspl_indexer_entry_t*)*ctx->stubs_cap);
    }
    pspl_indexer_entry_t* new_entry = malloc(sizeof(pspl_indexer_entry_t));
    new_entry->parent = ctx;
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
    
    // Determine if reference is newer
    char path_hash_str[PSPL_HASH_STRING_LEN];
    char newest_data_hash_str[PSPL_HASH_STRING_LEN];
    int is_newer = is_source_ref_newer_than_staged_output(path_in, path_ext_in,
                                                          path_hash_str, newest_data_hash_str, 1);
    
    if (is_newer) {
        
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
        
        // Message path
        if (xterm_colour)
            fprintf(stderr, BOLD"Path Hash: "CYAN"%s"SGR0"\n", path_hash_str);
        else
            fprintf(stderr, "Path Hash: %s\n", path_hash_str);
        
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
        strlcat(final_hash_path_str, driver_state.staging_path, MAXPATHLEN);
        strlcat(final_hash_path_str, final_hash_str, MAXPATHLEN);
        
        // Write to staging area
        FILE* file = fopen(final_hash_path_str, "w");
        if (!file)
            pspl_error(-1, "Unable to open conversion file for writing", "Unable to write to `%s`",
                       final_hash_path_str);
        fwrite(conv_buf, 1, conv_len, file);
        fclose(file);
        
        // Message data
        if (xterm_colour)
            fprintf(stderr, BOLD"Data Hash: "CYAN"%s"SGR0"\n", final_hash_str);
        else
            fprintf(stderr, "Data Hash: %s\n", final_hash_str);
        
    } else {
        
        // No conversion; just copy newest data hash
        pspl_hash_parse(&new_entry->object_hash, newest_data_hash_str);
        
    }
    
    // Populate structure
    new_entry->parent->definer = definer->file_path;
    size_t cpy_len = strlen(path_in)+1;
    char* cpy_path = malloc(cpy_len);
    strncpy(cpy_path, path_in, cpy_len);
    new_entry->stub_source_path = cpy_path;
    new_entry->stub_source_path_ext = NULL;
    if (path_ext_in) {
        cpy_len = strlen(path_ext_in)+1;
        cpy_path = malloc(cpy_len);
        strncpy(cpy_path, path_ext_in, cpy_len);
        new_entry->stub_source_path_ext = cpy_path;
    }
    if (driver_state.gather_ctx)
        pspl_gather_add_file(driver_state.gather_ctx, cpy_path);
    
    converter_state.path = NULL;
    
}


#pragma mark File Generators

/* Translates local per-psplc bits to global per-psplp bits */
uint32_t union_plat_bits(pspl_indexer_globals_t* globals,
                         pspl_indexer_context_t* locals, uint32_t bits) {
    
    int i,j;
    uint32_t result = 0;
    
    for (i=0 ; i<32 ; ++i) {
        if (bits & 0x1) {
            const pspl_platform_t* plat = locals->plat_array[i];
            for (j=0 ; j<globals->plat_count ; ++j) {
                const pspl_platform_t* g_plat = globals->plat_array[j];
                if (plat == g_plat) {
                    result |= (0x1 << j);
                    break;
                }
            }
        }
        
        bits >>= 1;
    }
    
    return result;
    
}


/* Write out bare PSPLC object (separate for direct PSPLP writing) */
void pspl_indexer_write_psplc_bare(pspl_indexer_context_t* ctx,
                                   uint8_t psplc_endianness,
                                   pspl_indexer_globals_t* globals,
                                   FILE* psplc_file_out) {
    
    int i,j,k;
    
    // Write hash
    fwrite(&ctx->psplc_hash, 1, sizeof(pspl_hash), psplc_file_out);
    
    // Populate PSPLC object header
    pspl_psplc_header_bi_t psplc_header;
    SET_BI_U32(psplc_header, extension_count, ctx->ext_count);
    SET_BI_U32(psplc_header, platform_count, ctx->plat_count);
    
    // Write PSPLC object header
    switch (psplc_endianness) {
        case PSPL_LITTLE_ENDIAN:
            fwrite(&psplc_header.little, 1, sizeof(pspl_psplc_header_t), psplc_file_out);
            break;
        case PSPL_BIG_ENDIAN:
            fwrite(&psplc_header.big, 1, sizeof(pspl_psplc_header_t), psplc_file_out);
            break;
        case PSPL_BI_ENDIAN:
            fwrite(&psplc_header, 1, sizeof(pspl_psplc_header_bi_t), psplc_file_out);
            break;
        default:
            break;
    }
    
    
    // Populate and write all per-extension array heading structures
    for (i=0 ; i<ctx->ext_count ; ++i) {
        const pspl_extension_t* cur_ext = ctx->ext_array[i];
        uint32_t hash_count = 0, int_count = 0;
        
        // Hash object count
        uint32_t hash_off = ctx->extension_obj_array_off;
        for (j=0 ; j<ctx->h_objects_count ; ++j)
            if (ctx->h_objects_array[j]->owner_ext == cur_ext) {
                ctx->extension_obj_array_off += sizeof(pspl_hash);
                ctx->extension_obj_array_off += (psplc_endianness==PSPL_BI_ENDIAN) ? sizeof(pspl_object_hash_record_bi_t) : sizeof(pspl_object_hash_record_t);
                ++hash_count;
            }
        
        // Int object count
        uint32_t int_off = ctx->extension_obj_array_off;
        for (j=0 ; j<ctx->i_objects_count ; ++j)
            if (ctx->i_objects_array[j]->owner_ext == cur_ext) {
                ctx->extension_obj_array_off += (psplc_endianness==PSPL_BI_ENDIAN) ? sizeof(pspl_object_int_record_bi_t) : sizeof(pspl_object_int_record_t);
                ++int_count;
            }
        
        // Populate structure
        pspl_object_array_extension_bi_t object_array_tier2;
        
        // Determine extension index
        if (globals == (pspl_indexer_globals_t*)ctx) {
            SET_BI_U32(object_array_tier2, extension_index, i);
        } else {
            for (j=0 ; j<globals->ext_count ; ++j) {
                const pspl_extension_t* global_ext = globals->ext_array[j];
                if (global_ext == cur_ext) {
                    SET_BI_U32(object_array_tier2, extension_index, j);
                    break;
                }
            }
        }
        SET_BI_U32(object_array_tier2, ext_hash_indexed_object_count, hash_count);
        SET_BI_U32(object_array_tier2, ext_hash_indexed_object_array_off, hash_off);
        SET_BI_U32(object_array_tier2, ext_int_indexed_object_count, int_count);
        SET_BI_U32(object_array_tier2, ext_int_indexed_object_array_off, int_off);
        
        // Write structure
        switch (psplc_endianness) {
            case PSPL_LITTLE_ENDIAN:
                fwrite(&object_array_tier2.little, 1, sizeof(pspl_object_array_extension_t), psplc_file_out);
                break;
            case PSPL_BIG_ENDIAN:
                fwrite(&object_array_tier2.big, 1, sizeof(pspl_object_array_extension_t), psplc_file_out);
                break;
            case PSPL_BI_ENDIAN:
                fwrite(&object_array_tier2, 1, sizeof(pspl_object_array_extension_bi_t), psplc_file_out);
                break;
            default:
                break;
        }
    }
    
    // Populate and write all per-platform array heading structures
    for (i=0 ; i<ctx->plat_count ; ++i) {
        const pspl_platform_t* cur_plat = ctx->plat_array[i];
        uint32_t hash_count = 0, int_count = 0;
        
        // Hash object count
        uint32_t hash_off = ctx->extension_obj_array_off;
        for (j=0 ; j<ctx->ph_objects_count ; ++j)
            if (ctx->ph_objects_array[j]->owner_plat == cur_plat) {
                ctx->extension_obj_array_off += sizeof(pspl_hash);
                ctx->extension_obj_array_off += (psplc_endianness==PSPL_BI_ENDIAN) ? sizeof(pspl_object_hash_record_bi_t) : sizeof(pspl_object_hash_record_t);
                ++hash_count;
            }
        
        // Int object count
        uint32_t int_off = ctx->extension_obj_array_off;
        for (j=0 ; j<ctx->pi_objects_count ; ++j)
            if (ctx->pi_objects_array[j]->owner_plat == cur_plat) {
                ctx->extension_obj_array_off += (psplc_endianness==PSPL_BI_ENDIAN) ? sizeof(pspl_object_int_record_bi_t) : sizeof(pspl_object_int_record_t);
                ++int_count;
            }
        
        // Populate structure
        pspl_object_array_extension_bi_t object_array_tier2;
        
        // Determine platform index
        if (globals == (pspl_indexer_globals_t*)ctx) {
            SET_BI_U32(object_array_tier2, platform_index, i);
        } else {
            for (j=0 ; j<globals->ext_count ; ++j) {
                const pspl_platform_t* global_plat = globals->plat_array[j];
                if (global_plat == cur_plat) {
                    SET_BI_U32(object_array_tier2, platform_index, j);
                    break;
                }
            }
        }
        SET_BI_U32(object_array_tier2, ext_hash_indexed_object_count, hash_count);
        SET_BI_U32(object_array_tier2, ext_hash_indexed_object_array_off, hash_off);
        SET_BI_U32(object_array_tier2, ext_int_indexed_object_count, int_count);
        SET_BI_U32(object_array_tier2, ext_int_indexed_object_array_off, int_off);
        
        // Write structure
        switch (psplc_endianness) {
            case PSPL_LITTLE_ENDIAN:
                fwrite(&object_array_tier2.little, 1, sizeof(pspl_object_array_extension_t), psplc_file_out);
                break;
            case PSPL_BIG_ENDIAN:
                fwrite(&object_array_tier2.big, 1, sizeof(pspl_object_array_extension_t), psplc_file_out);
                break;
            case PSPL_BI_ENDIAN:
                fwrite(&object_array_tier2, 1, sizeof(pspl_object_array_extension_bi_t), psplc_file_out);
                break;
            default:
                break;
        }
    }
    
    // Populate and write all per-extension array objects
    for (i=0 ; i<ctx->ext_count ; ++i) {
        const pspl_extension_t* cur_ext = ctx->ext_array[i];
        
        // Hash objects
        for (j=0 ; j<ctx->h_objects_count ; ++j)
            if (ctx->h_objects_array[j]->owner_ext == cur_ext) {
                pspl_indexer_entry_t* ent = ctx->h_objects_array[j];
                
                // Populate structure
                pspl_object_hash_record_bi_t hash_record;
                SET_BI_U32(hash_record, platform_availability_bits,
                           (globals == (pspl_indexer_globals_t*)ctx)?
                           ent->platform_availability_bits:
                           union_plat_bits(globals, ctx, ent->platform_availability_bits));
                SET_BI_U16(hash_record, object_bi, (ent->object_little_data && ent->object_big_data &&
                                                    ent->object_little_data != ent->object_big_data));
                SET_BI_U32(hash_record, object_off, ctx->extension_obj_data_off);
                if (ent->object_little_data && ent->object_big_data &&
                    ent->object_little_data != ent->object_big_data) {
#                   ifdef __LITTLE_ENDIAN__
                    hash_record.big.object_off = swap_uint32(ctx->extension_obj_data_off + (uint32_t)ent->object_len);
#                   else
                    hash_record.big.object_off = extension_obj_data_off + (uint32_t)ent->object_len;
#                   endif
                }
                SET_BI_U32(hash_record, object_len, (uint32_t)ent->object_len);
                
                // Write hash
                fwrite(&ent->object_hash, 1, sizeof(pspl_hash), psplc_file_out);
                
                // Write structure
                switch (psplc_endianness) {
                    case PSPL_LITTLE_ENDIAN:
                        fwrite(&hash_record.little, 1, sizeof(pspl_object_hash_record_t), psplc_file_out);
                        break;
                    case PSPL_BIG_ENDIAN:
                        fwrite(&hash_record.big, 1, sizeof(pspl_object_hash_record_t), psplc_file_out);
                        break;
                    case PSPL_BI_ENDIAN:
                        fwrite(&hash_record, 1, sizeof(pspl_object_hash_record_bi_t), psplc_file_out);
                        break;
                    default:
                        break;
                }
                
            }
        
        
        // Int objects
        for (j=0 ; j<ctx->i_objects_count ; ++j)
            if (ctx->i_objects_array[j]->owner_ext == cur_ext) {
                pspl_indexer_entry_t* ent = ctx->i_objects_array[j];
                
                // Populate structure
                pspl_object_int_record_bi_t int_record;
                SET_BI_U32(int_record, object_index, ent->object_index);
                SET_BI_U32(int_record, platform_availability_bits,
                           (globals == (pspl_indexer_globals_t*)ctx)?
                           ent->platform_availability_bits:
                           union_plat_bits(globals, ctx, ent->platform_availability_bits));
                SET_BI_U16(int_record, object_bi, (ent->object_little_data && ent->object_big_data &&
                                                   ent->object_little_data != ent->object_big_data));
                SET_BI_U32(int_record, object_off, ctx->extension_obj_data_off);
                if (ent->object_little_data && ent->object_big_data &&
                    ent->object_little_data != ent->object_big_data) {
#                   ifdef __LITTLE_ENDIAN__
                    int_record.big.object_off = swap_uint32(ctx->extension_obj_data_off + (uint32_t)ent->object_len);
#                   else
                    int_record.big.object_off = extension_obj_data_off + (uint32_t)ent->object_len;
#                   endif
                }
                SET_BI_U32(int_record, object_len, (uint32_t)ent->object_len);
                
                // Write structure
                switch (psplc_endianness) {
                    case PSPL_LITTLE_ENDIAN:
                        fwrite(&int_record.little, 1, sizeof(pspl_object_int_record_t), psplc_file_out);
                        break;
                    case PSPL_BIG_ENDIAN:
                        fwrite(&int_record.big, 1, sizeof(pspl_object_int_record_t), psplc_file_out);
                        break;
                    case PSPL_BI_ENDIAN:
                        fwrite(&int_record, 1, sizeof(pspl_object_int_record_bi_t), psplc_file_out);
                        break;
                    default:
                        break;
                }
                
            }
        
    }
    
    // Populate and write all per-platform array objects
    for (i=0 ; i<ctx->plat_count ; ++i) {
        const pspl_platform_t* cur_plat = ctx->plat_array[i];
        
        // Hash objects
        for (j=0 ; j<ctx->ph_objects_count ; ++j)
            if (ctx->ph_objects_array[j]->owner_plat == cur_plat) {
                pspl_indexer_entry_t* ent = ctx->ph_objects_array[j];
                
                // Populate structure
                pspl_object_hash_record_bi_t hash_record;
                SET_BI_U32(hash_record, platform_availability_bits,
                           (globals == (pspl_indexer_globals_t*)ctx)?
                           ent->platform_availability_bits:
                           union_plat_bits(globals, ctx, ent->platform_availability_bits));
                SET_BI_U16(hash_record, object_bi, (ent->object_little_data && ent->object_big_data &&
                                                    ent->object_little_data != ent->object_big_data));
                SET_BI_U32(hash_record, object_off, ctx->extension_obj_data_off);
                if (ent->object_little_data && ent->object_big_data &&
                    ent->object_little_data != ent->object_big_data) {
#                   ifdef __LITTLE_ENDIAN__
                    hash_record.big.object_off = swap_uint32(ctx->extension_obj_data_off + (uint32_t)ent->object_len);
#                   else
                    hash_record.big.object_off = extension_obj_data_off + (uint32_t)ent->object_len;
#                   endif
                }
                SET_BI_U32(hash_record, object_len, (uint32_t)ent->object_len);
                
                // Write hash
                fwrite(&ent->object_hash, 1, sizeof(pspl_hash), psplc_file_out);
                
                // Write structure
                switch (psplc_endianness) {
                    case PSPL_LITTLE_ENDIAN:
                        fwrite(&hash_record.little, 1, sizeof(pspl_object_hash_record_t), psplc_file_out);
                        break;
                    case PSPL_BIG_ENDIAN:
                        fwrite(&hash_record.big, 1, sizeof(pspl_object_hash_record_t), psplc_file_out);
                        break;
                    case PSPL_BI_ENDIAN:
                        fwrite(&hash_record, 1, sizeof(pspl_object_hash_record_bi_t), psplc_file_out);
                        break;
                    default:
                        break;
                }
                
            }
        
        
        // Int objects
        for (j=0 ; j<ctx->pi_objects_count ; ++j)
            if (ctx->pi_objects_array[j]->owner_plat == cur_plat) {
                pspl_indexer_entry_t* ent = ctx->pi_objects_array[j];
                
                // Populate structure
                pspl_object_int_record_bi_t int_record;
                SET_BI_U32(int_record, object_index, ent->object_index);
                SET_BI_U32(int_record, platform_availability_bits,
                           (globals == (pspl_indexer_globals_t*)ctx)?
                           ent->platform_availability_bits:
                           union_plat_bits(globals, ctx, ent->platform_availability_bits));
                SET_BI_U16(int_record, object_bi, (ent->object_little_data && ent->object_big_data &&
                                                   ent->object_little_data != ent->object_big_data));
                SET_BI_U32(int_record, object_off, ctx->extension_obj_data_off);
                if (ent->object_little_data && ent->object_big_data &&
                    ent->object_little_data != ent->object_big_data) {
#                   ifdef __LITTLE_ENDIAN__
                    int_record.big.object_off = swap_uint32(ctx->extension_obj_data_off + (uint32_t)ent->object_len);
#                   else
                    int_record.big.object_off = extension_obj_data_off + (uint32_t)ent->object_len;
#                   endif
                }
                SET_BI_U32(int_record, object_len, (uint32_t)ent->object_len);
                
                // Write structure
                switch (psplc_endianness) {
                    case PSPL_LITTLE_ENDIAN:
                        fwrite(&int_record.little, 1, sizeof(pspl_object_int_record_t), psplc_file_out);
                        break;
                    case PSPL_BIG_ENDIAN:
                        fwrite(&int_record.big, 1, sizeof(pspl_object_int_record_t), psplc_file_out);
                        break;
                    case PSPL_BI_ENDIAN:
                        fwrite(&int_record, 1, sizeof(pspl_object_int_record_bi_t), psplc_file_out);
                        break;
                    default:
                        break;
                }
                
            }
        
    }
    
    // Write all per-extension array object data blobs
    for (i=0 ; i<ctx->ext_count ; ++i) {
        const pspl_extension_t* cur_ext = ctx->ext_array[i];
        
        // Hash objects
        for (j=0 ; j<ctx->h_objects_count ; ++j)
            if (ctx->h_objects_array[j]->owner_ext == cur_ext) {
                pspl_indexer_entry_t* ent = ctx->h_objects_array[j];
                
                if (ent->object_little_data && ent->object_big_data &&
                    ent->object_little_data != ent->object_big_data) {
                    fwrite(ent->object_little_data, 1, ent->object_len, psplc_file_out);
                    for (k=0 ; k<ent->object_padding ; ++k)
                        fwrite("", 1, 1, psplc_file_out);
                    fwrite(ent->object_big_data, 1, ent->object_len, psplc_file_out);
                } else if (ent->object_little_data)
                    fwrite(ent->object_little_data, 1, ent->object_len, psplc_file_out);
                else
                    fwrite(ent->object_big_data, 1, ent->object_len, psplc_file_out);
                for (k=0 ; k<ent->object_padding ; ++k)
                    fwrite("", 1, 1, psplc_file_out);
                
            }
        
        
        // Int objects
        for (j=0 ; j<ctx->i_objects_count ; ++j)
            if (ctx->i_objects_array[j]->owner_ext == cur_ext) {
                pspl_indexer_entry_t* ent = ctx->i_objects_array[j];
                
                if (ent->object_little_data && ent->object_big_data &&
                    ent->object_little_data != ent->object_big_data) {
                    fwrite(ent->object_little_data, 1, ent->object_len, psplc_file_out);
                    for (k=0 ; k<ent->object_padding ; ++k)
                        fwrite("", 1, 1, psplc_file_out);
                    fwrite(ent->object_big_data, 1, ent->object_len, psplc_file_out);
                } else if (ent->object_little_data)
                    fwrite(ent->object_little_data, 1, ent->object_len, psplc_file_out);
                else
                    fwrite(ent->object_big_data, 1, ent->object_len, psplc_file_out);
                for (k=0 ; k<ent->object_padding ; ++k)
                    fwrite("", 1, 1, psplc_file_out);
                
            }
        
    }
    
    // Write all per-platform array object data blobs
    for (i=0 ; i<ctx->plat_count ; ++i) {
        const pspl_platform_t* cur_plat = ctx->plat_array[i];
        
        // Hash objects
        for (j=0 ; j<ctx->ph_objects_count ; ++j)
            if (ctx->ph_objects_array[j]->owner_plat == cur_plat) {
                pspl_indexer_entry_t* ent = ctx->ph_objects_array[j];
                
                if (ent->object_little_data && ent->object_big_data &&
                    ent->object_little_data != ent->object_big_data) {
                    fwrite(ent->object_little_data, 1, ent->object_len, psplc_file_out);
                    for (k=0 ; k<ent->object_padding ; ++k)
                        fwrite("", 1, 1, psplc_file_out);
                    fwrite(ent->object_big_data, 1, ent->object_len, psplc_file_out);
                } else if (ent->object_little_data)
                    fwrite(ent->object_little_data, 1, ent->object_len, psplc_file_out);
                else
                    fwrite(ent->object_big_data, 1, ent->object_len, psplc_file_out);
                for (k=0 ; k<ent->object_padding ; ++k)
                    fwrite("", 1, 1, psplc_file_out);
                
            }
        
        
        // Int objects
        for (j=0 ; j<ctx->pi_objects_count ; ++j)
            if (ctx->pi_objects_array[j]->owner_plat == cur_plat) {
                pspl_indexer_entry_t* ent = ctx->pi_objects_array[j];
                
                if (ent->object_little_data && ent->object_big_data &&
                    ent->object_little_data != ent->object_big_data) {
                    fwrite(ent->object_little_data, 1, ent->object_len, psplc_file_out);
                    for (k=0 ; k<ent->object_padding ; ++k)
                        fwrite("", 1, 1, psplc_file_out);
                    fwrite(ent->object_big_data, 1, ent->object_len, psplc_file_out);
                } else if (ent->object_little_data)
                    fwrite(ent->object_little_data, 1, ent->object_len, psplc_file_out);
                else
                    fwrite(ent->object_big_data, 1, ent->object_len, psplc_file_out);
                for (k=0 ; k<ent->object_padding ; ++k)
                    fwrite("", 1, 1, psplc_file_out);
                
            }
        
    }
    
}

/* Write out to PSPLC file */
void pspl_indexer_write_psplc(pspl_indexer_context_t* ctx,
                              uint8_t psplc_endianness,
                              FILE* psplc_file_out) {
    
    int i;
    
    // Determine endianness
    for (i=0 ; i<ctx->plat_count ; ++i)
        psplc_endianness |= ctx->plat_array[i]->byte_order;
    psplc_endianness &= PSPL_BI_ENDIAN;
    if (!psplc_endianness) {
#       if __LITTLE_ENDIAN__
        psplc_endianness = PSPL_LITTLE_ENDIAN;
#       elif __BIG_ENDIAN__
        psplc_endianness = PSPL_BIG_ENDIAN;
#       endif
    }
    
    // Table offset accumulations
    uint32_t acc = sizeof(pspl_header_t);
    acc += (psplc_endianness==PSPL_BI_ENDIAN) ? sizeof(pspl_off_header_bi_t) : sizeof(pspl_off_header_t);
    acc += sizeof(pspl_hash);
    acc += (psplc_endianness==PSPL_BI_ENDIAN) ? sizeof(pspl_psplc_header_bi_t) : sizeof(pspl_psplc_header_t);
    acc += ((psplc_endianness==PSPL_BI_ENDIAN) ? sizeof(pspl_object_array_extension_bi_t) : sizeof(pspl_object_array_extension_t)) * ctx->ext_count;
    ctx->extension_obj_array_off = acc;
    acc += ((psplc_endianness==PSPL_BI_ENDIAN) ? sizeof(pspl_object_hash_record_bi_t) : sizeof(pspl_object_hash_record_t) + sizeof(pspl_hash)) * ctx->h_objects_count;
    acc += ((psplc_endianness==PSPL_BI_ENDIAN) ? sizeof(pspl_object_int_record_bi_t) : sizeof(pspl_object_int_record_t)) * ctx->i_objects_count;
    acc += ((psplc_endianness==PSPL_BI_ENDIAN) ? sizeof(pspl_object_hash_record_bi_t) : sizeof(pspl_object_hash_record_t) + sizeof(pspl_hash)) * ctx->ph_objects_count;
    acc += ((psplc_endianness==PSPL_BI_ENDIAN) ? sizeof(pspl_object_int_record_bi_t) : sizeof(pspl_object_int_record_t)) * ctx->pi_objects_count;
    ctx->extension_obj_data_off = acc;
    for (i=0 ; i<ctx->h_objects_count ; ++i) {
        pspl_indexer_entry_t* ent = ctx->h_objects_array[i];
        ent->object_padding = ROUND_UP_4(acc) - acc;
        if (ent->object_little_data && ent->object_big_data &&
            ent->object_little_data != ent->object_big_data)
            acc += (ent->object_len+ent->object_padding)*2;
        else
            acc += ent->object_len+ent->object_padding;
    }
    for (i=0 ; i<ctx->i_objects_count ; ++i) {
        pspl_indexer_entry_t* ent = ctx->i_objects_array[i];
        ent->object_padding = ROUND_UP_4(acc) - acc;
        if (ent->object_little_data && ent->object_big_data &&
            ent->object_little_data != ent->object_big_data)
            acc += (ent->object_len+ent->object_padding)*2;
        else
            acc += ent->object_len+ent->object_padding;
    }
    for (i=0 ; i<ctx->ph_objects_count ; ++i) {
        pspl_indexer_entry_t* ent = ctx->ph_objects_array[i];
        ent->object_padding = ROUND_UP_4(acc) - acc;
        if (ent->object_little_data && ent->object_big_data &&
            ent->object_little_data != ent->object_big_data)
            acc += (ent->object_len+ent->object_padding)*2;
        else
            acc += ent->object_len+ent->object_padding;
    }
    for (i=0 ; i<ctx->pi_objects_count ; ++i) {
        pspl_indexer_entry_t* ent = ctx->pi_objects_array[i];
        ent->object_padding = ROUND_UP_4(acc) - acc;
        if (ent->object_little_data && ent->object_big_data &&
            ent->object_little_data != ent->object_big_data)
            acc += (ent->object_len+ent->object_padding)*2;
        else
            acc += ent->object_len+ent->object_padding;
    }
    uint32_t file_stub_array_off = acc;
    acc += ((psplc_endianness==PSPL_BI_ENDIAN) ? sizeof(pspl_file_stub_bi_t) : sizeof(pspl_file_stub_t) + sizeof(pspl_hash)) * ctx->stubs_count;
    
    uint32_t stub_string_table_off = acc;
    uint32_t extension_name_table_off = acc;
    for (i=0 ; i<ctx->stubs_count ; ++i) {
        extension_name_table_off += strlen(ctx->stubs_array[i]->stub_source_path) + 1;
        if (ctx->stubs_array[i]->stub_source_path_ext)
            extension_name_table_off += strlen(ctx->stubs_array[i]->stub_source_path_ext) + 1;
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
        .package_flag = PSPL_PSPLC,
        .version = PSPL_VERSION,
        .endian_flags = psplc_endianness,
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
    fwrite(&pspl_header, 1, sizeof(pspl_header_t), psplc_file_out);
    
    // Write offset header
    switch (psplc_endianness) {
        case PSPL_LITTLE_ENDIAN:
            fwrite(&pspl_off_header.little, 1, sizeof(pspl_off_header_t), psplc_file_out);
            break;
        case PSPL_BIG_ENDIAN:
            fwrite(&pspl_off_header.big, 1, sizeof(pspl_off_header_t), psplc_file_out);
            break;
        case PSPL_BI_ENDIAN:
            fwrite(&pspl_off_header, 1, sizeof(pspl_off_header_bi_t), psplc_file_out);
            break;
        default:
            break;
    }
    
    // Perform bare file write
    pspl_indexer_write_psplc_bare(ctx, psplc_endianness, (pspl_indexer_globals_t*)ctx, psplc_file_out);
    
    
    // Populate and write all file-stub objects
    for (i=0 ; i<ctx->stubs_count ; ++i) {
        pspl_indexer_entry_t* ent = ctx->stubs_array[i];
        
        pspl_file_stub_bi_t stub_record;
        SET_BI_U32(stub_record, platform_availability_bits, ent->platform_availability_bits);
        SET_BI_U32(stub_record, file_path_off, stub_string_table_off);
        stub_string_table_off += strlen(ent->stub_source_path) + 1;
        if (ent->stub_source_path_ext) {
            SET_BI_U32(stub_record, file_path_ext_off, stub_string_table_off);
            stub_string_table_off += strlen(ent->stub_source_path_ext) + 1;
        } else
            SET_BI_U32(stub_record, file_path_ext_off, 0);
        
        // Write data hash
        fwrite(&ent->object_hash, 1, sizeof(pspl_hash), psplc_file_out);
        
        // Write structure
        switch (psplc_endianness) {
            case PSPL_LITTLE_ENDIAN:
                fwrite(&stub_record.little, 1, sizeof(pspl_file_stub_t), psplc_file_out);
                break;
            case PSPL_BIG_ENDIAN:
                fwrite(&stub_record.big, 1, sizeof(pspl_file_stub_t), psplc_file_out);
                break;
            case PSPL_BI_ENDIAN:
                fwrite(&stub_record, 1, sizeof(pspl_file_stub_bi_t), psplc_file_out);
                break;
            default:
                break;
        }

    }
    
    // Write all file-stub string blobs
    for (i=0 ; i<ctx->stubs_count ; ++i) {
        pspl_indexer_entry_t* ent = ctx->stubs_array[i];
        
        fwrite(ent->stub_source_path, 1, strlen(ent->stub_source_path)+1, psplc_file_out);
        if (ent->stub_source_path_ext)
            fwrite(ent->stub_source_path_ext, 1, strlen(ent->stub_source_path_ext)+1, psplc_file_out);
        
    }
    
    // Write all Extension name string blobs
    for (i=0 ; i<ctx->ext_count ; ++i) {
        const pspl_extension_t* ext = ctx->ext_array[i];
        fwrite(ext->extension_name, 1, strlen(ext->extension_name)+1, psplc_file_out);
    }
    
    // Write all Platform name string blobs
    for (i=0 ; i<ctx->plat_count ; ++i) {
        const pspl_platform_t* plat = ctx->plat_array[i];
        fwrite(plat->platform_name, 1, strlen(plat->platform_name)+1, psplc_file_out);
    }
    
    // Pad with 0xFF
    char ff = 0xff;
    for (i=0 ; i<pad_end ; ++i)
        fwrite(&ff, 1, 1, psplc_file_out);
    
}

