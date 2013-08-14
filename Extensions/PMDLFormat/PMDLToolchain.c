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
#include <PSPL/PSPLHash.h>
#include <sys/param.h>

#if __APPLE__
#define BLENDER_DEFAULT "/Applications/blender.app/Contents/MacOS/blender"
#define BLENDER_BOOT_SCRIPT "/tmp/pmdl_blender_boot.py"
#elif _WIN32
#define BLENDER_DEFAULT "C:/Program Files/Blender Foundation/Blender/blender.exe"
#define BLENDER_BOOT_SCRIPT "C:/Windows/Temp/pmdl_blender_boot.py"
#else
#define BLENDER_DEFAULT "blender"
#define BLENDER_BOOT_SCRIPT "/tmp/pmdl_blender_boot.py"
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
    
    pspl_toolchain_provide_copyright("io_pmdl_export (Exporter Addon for Blender)",
                                     "Copyright (c) 2013 Jack Andersen <jackoalan@gmail.com>",
                                     "[See licence for \"PSPL\" above]");
    
    pspl_toolchain_provide_copyright("libogc (GU Matrix Math C-reference-implementations)",
                                     "Copyright (C) 2004 - 2009\n"
                                     "    Michael Wiedenbauer (shagkur)\n"
                                     "    Dave Murphy (WinterMute)",
                                     "[See licence for \"libogc\" in \"GX Platform\"]");
    
}

static void subext_hook() {
    
    pspl_toolchain_provide_subext("Blender Integration", "Converts named object from .blend file to PMDL", 0);
    
}

static const char* BOOT_SCRIPT_TEXT = "import bpy\nbpy.ops.io_export_scene.pmdl_cli()\n";
static int init_hook(const pspl_toolchain_context_t* driver_context) {
    
    // Write boot script
    FILE* boot_script_file = fopen(BLENDER_BOOT_SCRIPT, "w");
    if (!boot_script_file)
        pspl_error(-1, "Blender boot script error", "unable to write temporary python script; errno %d - %s", errno, strerror(errno));
    fwrite(BOOT_SCRIPT_TEXT, 1, strlen(BOOT_SCRIPT_TEXT), boot_script_file);
    fclose(boot_script_file);
    
    pspl_malloc_context_init(&gen_refs);
    pspl_malloc_context_init(&gx_refs);
    return 0;
    
}

static void finish_hook(const pspl_toolchain_context_t* driver_context) {
    int i;
    
    if (gen_refs.object_num) {
        
        size_t buf_sz = sizeof(pmdl_ref_entry)*gen_refs.object_num + sizeof(pmdl_bi_integer);
        void* entries_buf = malloc(buf_sz);
        pmdl_bi_integer* count_integer = (pmdl_bi_integer*)entries_buf;
        SET_BI_U32((*count_integer), integer, gen_refs.object_num);
        pmdl_ref_entry* entries = entries_buf + sizeof(pmdl_bi_integer);
        for (i=0 ; i<gen_refs.object_num ; ++i)
            entries[i] = *((pmdl_ref_entry*)gen_refs.object_arr[i]);
        pspl_embed_hash_keyed_object(general_plats, "PMDL_References", entries_buf, entries_buf, buf_sz);
        free(entries_buf);

    }
    
    if (gx_refs.object_num) {
        
        size_t buf_sz = sizeof(pmdl_ref_entry)*gx_refs.object_num + sizeof(pmdl_bi_integer);
        void* entries_buf = malloc(buf_sz);
        pmdl_bi_integer* count_integer = (pmdl_bi_integer*)entries_buf;
        SET_BI_U32((*count_integer), integer, gx_refs.object_num);
        pmdl_ref_entry* entries = entries_buf + sizeof(pmdl_bi_integer);
        for (i=0 ; i<gx_refs.object_num ; ++i)
            entries[i] = *((pmdl_ref_entry*)gx_refs.object_arr[i]);
        pspl_embed_hash_keyed_object(gx_plats, "PMDL_References", entries_buf, entries_buf, buf_sz);
        free(entries_buf);
        
    }
    
    pspl_malloc_context_destroy(&gen_refs);
    pspl_malloc_context_destroy(&gx_refs);
}


/* Conversion hook to run Blender instance for auto-export of PMDL */
static int blender_convert(char* path_out, const char* path_in, const char* path_ext_in,
                           const char* suggested_path, void* user_ptr) {
    strcpy(path_out, suggested_path);
    fprintf(stderr, "\n");
    
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
    
    // Target Draw Format
    const char* target_draw_fmt = NULL;
    if (user_ptr == general_plats)
        target_draw_fmt = "GENERAL";
    else if (user_ptr == gx_plats)
        target_draw_fmt = "GX";
    
    // Endianness
    const char* endianness = NULL;
    if (user_ptr == gx_plats)
        endianness = "BIG";
    else
        endianness = "LITTLE";
    
    // Pointer size
    const char* psize = NULL;
    if (user_ptr == gx_plats)
        psize = "32-BIT";
    else
        psize = "64-BIT";
    
    
    // Perform conversion
    pid_t blender_pid;
    if ((blender_pid = fork()) < 0)
        pspl_error(-1, "Unable to Fork to Blender", "errno %d - %s", errno, strerror(errno));
    
    if (!blender_pid) { // Child process
        execlp(blender_path, blender_path, "--background", path_in,
               "-P", BLENDER_BOOT_SCRIPT, "--", suggested_path,
               path_ext_in, target_draw_fmt, endianness, psize, NULL);
        exit(0);
    }
    
    // Parent process
    int blender_return = 0;
    waitpid(blender_pid, &blender_return, 0);
    if (!WIFEXITED(blender_return) || WEXITSTATUS(blender_return))
        pspl_error(-1, "Blender did not complete PMDL conversion",
                   "error code %d from Blender", WEXITSTATUS(blender_return));
    
    return 0;
    
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
        
        
        // Export BLEND objects for each target platform
        uint8_t added_general = 0;
        uint8_t added_gx = 0;
        pmdl_ref_entry* entry = NULL;
        pspl_hash* pmdl_hash = NULL;
        for (i=0 ; i<driver_context->target_runtime_platforms_c ; ++i) {
            if ((driver_context->target_runtime_platforms[i] == &GL2_platform ||
                 driver_context->target_runtime_platforms[i] == &D3D11_platform) &&
                !added_general) {
                
                pspl_package_file_augment(general_plats, command_argv[1], command_argv[2],
                                          blender_convert, 1, general_plats, &pmdl_hash);
                entry = pspl_malloc_malloc(&gen_refs, sizeof(pmdl_ref_entry));
                added_general = 1;
                
            } else if (driver_context->target_runtime_platforms[i] == &GX_platform && !added_gx) {
                
                pspl_package_file_augment(gx_plats, command_argv[1], command_argv[2],
                                          blender_convert, 1, gx_plats, &pmdl_hash);
                entry = pspl_malloc_malloc(&gx_refs, sizeof(pmdl_ref_entry));
                added_gx = 1;
                
            }
        }
        
        // Hashes
        entry->pmdl_file_hash = *pmdl_hash;
        entry->name_hash = *name_hash;
        
        
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
    .subext_hook = subext_hook,
    .claimed_global_command_names = claimed_global_command_names,
    .command_call_hook = command_call_hook
};
