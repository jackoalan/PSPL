//
//  gx_runtime.c
//  PSPL
//
//  Created by Jack Andersen on 5/25/13.
//
//

#include <ogc/cache.h>
#include <ogc/gx.h>
#include <PSPLExtension.h>
#include "gx_common.h"

static void load_object(pspl_runtime_psplc_t* object) {
    
    pspl_data_object_t datac;
    pspl_runtime_get_embedded_data_object_from_integer(object, GX_SHADER_CONFIG, &datac);
    object->native_shader.config = datac.object_data;
    
    pspl_data_object_t datao;
    pspl_runtime_get_embedded_data_object_from_integer(object, GX_SHADER_DL, &datao);

    object->native_shader.disp_list = datao.object_data;
    object->native_shader.disp_list_len = (u32)datao.object_len;
    DCStoreRange(object->native_shader.disp_list, (u32)datao.object_len);
    
}

static void bind_object(pspl_runtime_psplc_t* object) {
    
    // This will asynchronously instruct the GPU to run the list
    GX_CallDispList((void*)object->native_shader.disp_list,
                    object->native_shader.disp_list_len);
    
}


pspl_runtime_platform_t GX_runplat = {
    .load_object_hook = load_object,
    .bind_object_hook = bind_object
};
