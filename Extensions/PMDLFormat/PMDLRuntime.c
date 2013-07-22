//
//  PMDLRuntime.c
//  PSPL
//
//  Created by Jack Andersen on 7/1/13.
//
//

#include <stdio.h>
#include <stdlib.h>
#include <PSPLExtension.h>
#include <PSPLRuntime.h>
#include "PMDLCommon.h"

struct file_array {
    // Byte-order-native array of PMDL files
    unsigned count;
    const pspl_runtime_arc_file_t** files;
};

/* Currently-bound PMDL array */
static struct file_array* bound_array;

static uint8_t hash_cached = 0;
static pspl_hash pmdl_ref_key_hash_cache;
static void load_object_hook(pspl_runtime_psplc_t* object) {
    
    // Load PMDL Reference data
    pspl_data_object_t pmdl_ref_data;
    if (!hash_cached) {
        pspl_runtime_get_embedded_data_object_from_key(object, "PMDL_References", &pmdl_ref_key_hash_cache, &pmdl_ref_data);
        hash_cached = 1;
    } else
        pspl_runtime_get_embedded_data_object_from_hash(object, &pmdl_ref_key_hash_cache, &pmdl_ref_data);
    
    // Array to populate
    void* data_cur = pmdl_ref_data.object_data;
    struct file_array* files = malloc(*(uint32_t*)data_cur * sizeof(pspl_runtime_arc_file_t*) + sizeof(struct file_array));
    files->count = *(uint32_t*)data_cur;
    files->files = (const pspl_runtime_arc_file_t**)((uint8_t*)files + sizeof(struct file_array));
    data_cur += sizeof(uint32_t);
    
    int i;
    for (i=0 ; i<files->count ; ++i)
        files->files[i] = pspl_runtime_get_archived_file_from_hash(object->parent, (pspl_hash*)data_cur, 1);
    
    // Set user data pointer appropriately
    pspl_runtime_set_extension_user_data_pointer(object, files);
    
}

static void unload_object_hook(pspl_runtime_psplc_t* object) {
    
    // Set user data pointer appropriately
    struct file_array* files = pspl_runtime_get_extension_user_data_pointer(object);
    
    // Release all referenced files
    int i;
    for (i=0 ; i<files->count ; ++i)
        pspl_runtime_release_archived_file(files->files[i]);
    free(files);
    
}

static void bind_object_hook(pspl_runtime_psplc_t* object) {
    
    // Set user data pointer appropriately
    bound_array = pspl_runtime_get_extension_user_data_pointer(object);
    
}

pspl_runtime_extension_t PMDL_runext = {
    .load_object_hook = load_object_hook,
    .unload_object_hook = unload_object_hook,
    .bind_object_hook = bind_object_hook
};

