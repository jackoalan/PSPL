//
//  IRRuntime.c
//  PSPL
//
//  Created by Jack Andersen on 6/9/13.
//
//

#include <stdio.h>
#include <PSPLExtension.h>

static int init(const pspl_extension_t* extension) {
    return 0;
}

static void shutdown() {
    
}

static void load_object(pspl_runtime_psplc_t* object) {
    
}

static void unload_object(pspl_runtime_psplc_t* object) {
    
}

static void bind_object(pspl_runtime_psplc_t* object) {
    
}

pspl_runtime_extension_t PSPL_IR_runext = {
    .init_hook = init,
    .shutdown_hook = shutdown,
    .load_object_hook = load_object,
    .unload_object_hook = unload_object,
    .bind_object_hook = bind_object
};
