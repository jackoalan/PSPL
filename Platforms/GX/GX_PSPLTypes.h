//
//  GX_PSPLTypes.h
//  PSPL
//
//  Created by Jack Andersen on 5/8/13.
//
//

#ifndef PSPL_GX_PSPLTypes_h
#define PSPL_GX_PSPLTypes_h

#include "gctypes.h"

/* GX-specific type for shader object */
typedef struct {
    
    // Idempotent display-list buffer for `GX_CallDisplayList`
    const void* disp_list;
    u32 disp_list_len;
    
} GX_shader_object_t;

#endif
