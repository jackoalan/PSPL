//
//  PMDLToolchain.c
//  PSPL
//
//  Created by Jack Andersen on 7/1/13.
//
//

#include <stdio.h>
#include <errno.h>
#include <PSPLExtension.h>
#include "PMDLCommon.h"
#include <PSPLHash.h>

#if __APPLE__
#define BLENDER_DEFAULT "/Applications/blender.app/Contents/MacOS/blender"
#elif _WIN32
#define BLENDER_DEFAULT "C:/Program Files/Blender Foundation/Blender/blender.exe"
#else
#define BLENDER_DEFAULT "blender"
#endif
#include <sys/stat.h>
#include <unistd.h>

/* General PMDL platforms */
extern pspl_platform_t GL2_platform;
extern pspl_platform_t D3D11_platform;
static const pspl_platform_t* general_plats[] = {
    &GL2_platform,
    &D3D11_platform,
    NULL};
static pspl_malloc_context_t gen_refs;

/* GX PMDL platforms */
extern pspl_platform_t GX_platform;
static const pspl_platform_t* gx_plats[] = {
    &GX_platform,
    NULL};
static pspl_malloc_context_t gx_refs;

static void copyright_hook() {
    
    pspl_toolchain_provide_copyright("PMDL (PSPL-native 3D model format)",
                                     "Copyright (c) 2013 Jack Andersen <jackoalan@gmail.com>",
                                     "[See licence for \"PSPL\" above]");
    
}

static int init_hook(const pspl_toolchain_context_t* driver_context) {
    pspl_malloc_context_init(&gen_refs);
    pspl_malloc_context_init(&gx_refs);
    return 0;
}

static void finish_hook(const pspl_toolchain_context_t* driver_context) {
    int i;
    
    if (gen_refs.object_num) {
        
        size_t buf_sz = sizeof(pmdl_ref_entry)*gen_refs.object_num;
        pmdl_ref_entry* entries = malloc(buf_sz);
        for (i=0 ; i<gen_refs.object_num ; ++i)
            entries[i] = *((pmdl_ref_entry*)gen_refs.object_arr[i]);
        pspl_embed_hash_keyed_object(general_plats, "PMDL_References", entries, entries, buf_sz);
        free(entries);

    }
    
    if (gx_refs.object_num) {
        
        size_t buf_sz = sizeof(pmdl_ref_entry)*gx_refs.object_num;
        pmdl_ref_entry* entries = malloc(buf_sz);
        for (i=0 ; i<gx_refs.object_num ; ++i)
            entries[i] = *((pmdl_ref_entry*)gx_refs.object_arr[i]);
        pspl_embed_hash_keyed_object(gx_plats, "PMDL_References", entries, entries, buf_sz);
        free(entries);
        
    }
    
    pspl_malloc_context_destroy(&gen_refs);
    pspl_malloc_context_destroy(&gx_refs);
}


static void command_call_hook(const pspl_toolchain_context_t* driver_context,
                              const pspl_toolchain_heading_context_t* current_heading,
                              const char* command_name,
                              unsigned int command_argc,
                              const char** command_argv) {
    int i;
    
    if (!strcasecmp(command_name, "ADD_PMDL")) {
        
        if (command_argc < 2)
            pspl_error(-1, "Invalid ADD_PMDL usage",
                       "There must be *two* arguments: (<PMDL_NAME> <PMDL_PATH>)");
        
        
        // Open PMDL file
        FILE* pmdl = fopen(command_argv[1], "r");
        if (!pmdl)
            pspl_error(-1, "Unable to open PMDL", "'%s'-> `%s`: errno %d - %s",
                       command_argv[0], command_argv[1], errno, strerror(errno));
        
        // Read in header
        pmdl_header pmdl_header;
        fread(&pmdl_header, 1, sizeof(pmdl_header), pmdl);
        fclose(pmdl);
        
        // Name hash
        pspl_hash_ctx_t hash_ctx;
        pspl_hash_init(&hash_ctx);
        pspl_hash_write(&hash_ctx, command_argv[0], strlen(command_argv[0]));
        pspl_hash* name_hash = NULL;
        pspl_hash_result(&hash_ctx, name_hash);
        
        // Determine PMDL draw type and package according to target PSPL platform(s)
        pmdl_ref_entry* entry = NULL;
        pspl_hash* pmdl_hash = NULL;
        if (!memcmp(&pmdl_header.draw_format, "_GEN", 4)) {

            // Ensure there is no name collision
            for (i=0 ; i<gen_refs.object_num ; ++i) {
                pmdl_ref_entry* entry = (pmdl_ref_entry*)gen_refs.object_arr[i];
                if (!pspl_hash_cmp(&entry->name_hash, name_hash))
                    pspl_error(-1, "PMDL Name Collision", "a General PMDL with name '%s' has already been added to '%s'",
                               command_argv[0], driver_context->pspl_name);
            }
            
            // Package PMDL
            pspl_package_file_augment(general_plats, command_argv[1], NULL, NULL, 0, NULL, &pmdl_hash);
            entry = pspl_malloc_malloc(&gen_refs, sizeof(pmdl_ref_entry));
            
        } else if (!memcmp(&pmdl_header.draw_format, "__GX", 4)) {
            
            // Ensure there is no name collision
            for (i=0 ; i<gx_refs.object_num ; ++i) {
                pmdl_ref_entry* entry = (pmdl_ref_entry*)gx_refs.object_arr[i];
                if (!pspl_hash_cmp(&entry->name_hash, name_hash))
                    pspl_error(-1, "PMDL Name Collision", "a GX PMDL with name '%s' has already been added to '%s'",
                               command_argv[0], driver_context->pspl_name);
            }
            
            // Package PMDL
            pspl_package_file_augment(gx_plats, command_argv[1], NULL, NULL, 0, NULL, &pmdl_hash);
            entry = pspl_malloc_malloc(&gx_refs, sizeof(pmdl_ref_entry));
            
        } else
            pspl_error(-1, "Unknown PMDL Sub-Type",
                       "this build of PSPL doesn't recognise '%s' PMDL sub-types", pmdl_header.draw_format);
        
        
        // Hashes
        entry->pmdl_file_hash = *pmdl_hash;
        entry->name_hash = *name_hash;
        
        
        
    } else if (!strcasecmp(command_name, "ADD_BLENDER_OBJECT")) {
        
        if (command_argc < 3)
            pspl_error(-1, "Invalid ADD_BLENDER_OBJECT usage",
                       "There must be *three* arguments: (<PMDL_NAME> <BLEND_PATH> <BLEND_OBJ_NAME>)");
        
                
        // Name hash
        pspl_hash_ctx_t hash_ctx;
        pspl_hash_init(&hash_ctx);
        pspl_hash_write(&hash_ctx, command_argv[0], strlen(command_argv[0]));
        pspl_hash* name_hash = NULL;
        pspl_hash_result(&hash_ctx, name_hash);
        
        // BLEND absolute path
        char* blend_abs_path = NULL;
        // TODO: Do
        
        // Target PMDL
        
        
        // Get Blender environment variable
        char* blender_path = getenv("BLENDER_BIN");
        if (!blender_path)
            blender_path = BLENDER_DEFAULT;
        
        // Verify Blender binary
        struct stat test_bin;
        if (stat(blender_path, &test_bin))
            pspl_error(-1, "Unable to locate Blender", "`blender` executable not found at `%s`; the path may be"
                       " set by exporting the 'BLENDER_BIN' environment variable to the Blender binary's absolute path",
                       blender_path);
        if (!(S_ISREG(test_bin.st_mode) && (test_bin.st_mode & 0111)))
            pspl_error(-1, "Provided Blender binary not executable",
                       "`stat` indicates that `%s` is not executable", blender_path);
        
        // Export BLEND objects for each target platform
        uint8_t added_general = 0;
        uint8_t added_gx = 0;
        for (i=0 ; i<driver_context->target_runtime_platforms_c ; ++i) {
            if ((driver_context->target_runtime_platforms[i] == &GL2_platform ||
                 driver_context->target_runtime_platforms[i] == &D3D11_platform) &&
                !added_general) {
                
                // TODO: Move to conversion hook
                pid_t blender_pid;
                if ((blender_pid = fork()) < 0)
                    pspl_error(-1, "Unable to Fork to Blender", "errno %d - %s", errno, strerror(errno));
                
                if (!blender_pid) { // Child process
                    execlp(blender_path, "--background", blend_abs_path, "--addons", "io_pmdl_export",
                           "--python-text", "bpy.ops.io_pmdl_export.export_pmdl()", "--", "GENERAL");
                }
                
                added_general = 1;
                
            } else if (driver_context->target_runtime_platforms[i] == &GX_platform && !added_gx) {
                
                added_gx = 1;
                
            }
        }
        
        
    }
    
}


static const char* claimed_global_command_names[] = {
    "ADD_PMDL",
    "ADD_BLENDER_OBJECT",
    NULL};

pspl_toolchain_extension_t PMDL_toolext = {
    .init_hook = init_hook,
    .finish_hook = finish_hook,
    .copyright_hook = copyright_hook,
    .claimed_global_command_names = claimed_global_command_names,
    .command_call_hook = command_call_hook
};
