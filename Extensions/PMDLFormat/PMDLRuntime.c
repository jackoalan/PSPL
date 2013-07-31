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
#include <PSPL/PSPLHash.h>
#include "PMDLCommon.h"
#include "PMDLRuntimeProcessing.h"


struct file_array {
    // Byte-order-native array of PMDL files
    pmdl_bi_integer count;
    pmdl_ref_entry files[];
};

/* My own extension */
extern const pspl_extension_t PMDL_extension;

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
    struct file_array* files = pmdl_ref_data.object_data;
    
    // Initialise PMDLs
    int i;
    for (i=0 ; i<files->count.native.integer ; ++i) {
        files->files[i].file_ptr =
        pspl_runtime_get_archived_file_from_hash(object->parent, &files->files[i].pmdl_file_hash, 1);
        pmdl_init(files->files[i].file_ptr);
    }
    
    // Set user data pointer appropriately
    pspl_runtime_set_extension_user_data_pointer(&PMDL_extension, object, files);
    
}

static void unload_object_hook(pspl_runtime_psplc_t* object) {
    
    // Set user data pointer appropriately
    struct file_array* files = pspl_runtime_get_extension_user_data_pointer(&PMDL_extension, object);
    
    // Release all referenced files
    int i;
    for (i=0 ; i<files->count.native.integer ; ++i) {
        pmdl_destroy(files->files[i].file_ptr);
        pspl_runtime_release_archived_file(files->files[i].file_ptr);
    }
    
}


#pragma mark PMDL Lookup

const pspl_runtime_arc_file_t* pmdl_lookup(pspl_runtime_psplc_t* pspl_object, const char* pmdl_name) {
    
    struct file_array* files = pspl_runtime_get_extension_user_data_pointer(&PMDL_extension, pspl_object);
    
    // Hash name
    pspl_hash_ctx_t hash;
    pspl_hash_init(&hash);
    pspl_hash_write(&hash, pmdl_name, strlen(pmdl_name));
    pspl_hash* name_hash = NULL;
    pspl_hash_result(&hash, name_hash);
    
    // Lookup by hash
    int i;
    for (i=0 ; i<files->count.native.integer ; ++i) {
        if (!pspl_hash_cmp(name_hash, &files->files[i].name_hash))
            return files->files[i].file_ptr;
    }
    
    return NULL;
    
}


pspl_runtime_extension_t PMDL_runext = {
    .load_object_hook = load_object_hook,
    .unload_object_hook = unload_object_hook
};

