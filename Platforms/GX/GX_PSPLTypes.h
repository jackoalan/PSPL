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
#include "gx_common.h"

/* GX-specific type for shader object */
typedef struct {
    
    // Idempotent PSPL-IR buffer for playback into GX
    void* ir_stream_buf;
    u32 ir_stream_len;
    
    // Select values loaded from IR-buffer immediately
    uint8_t texgen_count, using_texcoord_normal;
    
} GX_shader_object_t;

#endif
