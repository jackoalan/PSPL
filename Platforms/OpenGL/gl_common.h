//
//  gl_common.h
//  PSPL
//
//  Created by Jack Andersen on 6/17/13.
//
//

#ifndef PSPL_gl_common_h
#define PSPL_gl_common_h

enum gl_object {
    GL_CONFIG_STRUCT   = 1,
    GL_VERTEX_SOURCE   = 2,
    GL_FRAGMENT_SOURCE = 3
};

typedef struct __attribute__ ((__packed__)) {
    uint8_t uv_attr_count, texmap_count;
    uint8_t depth_write, depth_test;
    uint8_t blending, source_factor, dest_factor;
} gl_config_t;
//typedef DEF_BI_OBJ_TYPE(gl_config_t) gl_config_bi_t;

#endif
