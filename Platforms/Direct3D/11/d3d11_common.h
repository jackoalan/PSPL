//
//  d3d11_common.h
//  PSPL
//
//  Created by Jack Andersen on 6/28/13.
//
//

#ifndef PSPL_d3d_common_h
#define PSPL_d3d_common_h

#include <stdint.h>

#define MAX_TEX_MAPS 16

enum d3d11_object {
    D3D11_CONFIG_STRUCT   = 1,
    D3D11_VERTEX_SOURCE   = 2,
    D3D11_VERTEX_BINARY   = 3,
    D3D11_PIXEL_SOURCE    = 4,
    D3D11_PIXEL_BINARY    = 5
};

typedef struct __attribute__ ((__packed__)) {
    uint8_t uv_attr_count, texmap_count, texgen_count;
    uint8_t depth_write, depth_test;
    uint8_t blending, source_factor, dest_factor;
} d3d11_config_t;

#endif
