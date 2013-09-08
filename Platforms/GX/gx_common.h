//
//  gx_common.h
//  PSPL
//
//  Created by Jack Andersen on 8/14/13.
//
//

#ifndef PSPL_gx_common_h
#define PSPL_gx_common_h

#include <PSPL/PSPL_IR.h>

enum gl_object {
    GX_SHADER_CONFIG = 1,
    GX_SHADER_DL     = 2
};

#pragma pack(1)
typedef struct __attribute__ ((__packed__)) {
    uint8_t texgen_count, using_texcoord_normal;
} gx_config_t;
#pragma pack()

#endif
