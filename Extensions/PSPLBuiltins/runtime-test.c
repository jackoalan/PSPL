//
//  runtime-test.c
//  PSPL
//
//  Created by Jack Andersen on 6/1/13.
//
//

#include <stdio.h>
#include <PSPLExtension.h>

static void BUILTINS_load_object(const pspl_runtime_psplc_t* object) {
    pspl_data_object_t obj;
    pspl_runtime_get_embedded_data_object_from_key(object, "test", &obj);
    fprintf(stderr, "Encoded: %.*s\n", (int)obj.object_len, obj.object_data);
}

pspl_runtime_extension_t BUILTINS_runext = {
    .load_object_hook = BUILTINS_load_object
};
