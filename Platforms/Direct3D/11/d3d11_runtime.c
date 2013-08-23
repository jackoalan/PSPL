//
//  d3d11_runtime.c
//  PSPL
//
//  Created by Jack Andersen on 5/25/13.
//
//

#include <stdio.h>
#include <PSPLExtension.h>
#include "d3d11_common.h"
#include "d3d11_runtime_interface.h"



/* Runtime method to set the Direct3D device being accessed by all
 * runtime extensions */
ID3D11Device* pspl_d3d11_device = NULL;
void pspl_d3d11_set_device(ID3D11Device* device) {
    pspl_d3d11_device = device;
}

ID3D11DeviceContext* pspl_d3d11_device_context = NULL;
void pspl_d3d11_set_device_context(ID3D11DeviceContext* context) {
    pspl_d3d11_device_context = context;
}



static void load_object(pspl_runtime_psplc_t* object) {
    int i;
    
    // Config structure
    pspl_data_object_t config_struct;
    pspl_runtime_get_embedded_data_object_from_integer(object, D3D11_CONFIG_STRUCT, &config_struct);
    object->native_shader.config = config_struct.object_data;
    
    
    // Vertex constant buffer
    object->native_shader.vertex_constant_buffer =
    pspl_d3d11_create_constant_buffer(sizeof(pspl_matrix34_t)*(2+object->native_shader.config->uv_attr_count));
    
    
    // Vertex Shader
    ID3DBlob* compiled_blob = NULL;
    pspl_data_object_t vert_shader_data;
    if (pspl_runtime_get_embedded_data_object_from_integer(object, D3D11_VERTEX_BINARY, &vert_shader_data) < 0) {
        pspl_data_object_t vert_shader_source;
        pspl_runtime_get_embedded_data_object_from_integer(object, D3D11_VERTEX_SOURCE, &vert_shader_source);
        void* data_out = NULL;
        size_t out_len = 0;
        if (pspl_d3d11_compile_vertex_shader(vert_shader_source.object_data, vert_shader_source.object_len,
                                             &compiled_blob, &data_out, &out_len) < 0)
            pspl_error(-1, "HLSL Vertex Shader Compile Failure", "%s", (char*)data_out);
        vert_shader_data.object_data = data_out;
        vert_shader_data.object_len = out_len;
    }
    
    object->native_shader.vertex_shader =
    pspl_d3d11_create_vertex_shader(vert_shader_data.object_data, vert_shader_data.object_len);
    
    if (compiled_blob)
        pspl_d3d11_destroy_something((IUnknown*)compiled_blob);
    
    
    // Pixel
    compiled_blob = NULL;
    pspl_data_object_t pix_shader_data;
    if (pspl_runtime_get_embedded_data_object_from_integer(object, D3D11_PIXEL_BINARY, &pix_shader_data) < 0) {
        pspl_data_object_t pix_shader_source;
        pspl_runtime_get_embedded_data_object_from_integer(object, D3D11_PIXEL_SOURCE, &pix_shader_source);
        void* data_out = NULL;
        size_t out_len = 0;
        if (pspl_d3d11_compile_pixel_shader(pix_shader_source.object_data, pix_shader_source.object_len,
                                            &compiled_blob, &data_out, &out_len) < 0)
            pspl_error(-1, "HLSL Pixel Shader Compile Failure", "%s", (char*)data_out);
        pix_shader_data.object_data = data_out;
        pix_shader_data.object_len = out_len;
    }
    
    object->native_shader.pixel_shader =
    pspl_d3d11_create_pixel_shader(pix_shader_data.object_data, pix_shader_data.object_len);
    
    if (compiled_blob)
        pspl_d3d11_destroy_something((IUnknown*)compiled_blob);
    
    
    // Matrix uniforms
    
    
    // Texture map uniforms
    
    
}

static void unload_object(pspl_runtime_psplc_t* object) {
    
    pspl_d3d11_destroy_something((IUnknown*)object->native_shader.vertex_shader);
    pspl_d3d11_destroy_something((IUnknown*)object->native_shader.pixel_shader);
    pspl_d3d11_destroy_something((IUnknown*)object->native_shader.vertex_constant_buffer);
    
}

static pspl_platform_shader_object_t* bound_shader = NULL;
static void bind_object(pspl_runtime_psplc_t* object) {
    
    pspl_d3d11_use_vertex_shader(object->native_shader.vertex_shader, object->native_shader.vertex_constant_buffer);
    pspl_d3d11_use_pixel_shader(object->native_shader.pixel_shader);
    
}

pspl_runtime_platform_t D3D11_runplat = {
    .load_object_hook = load_object,
    .unload_object_hook = unload_object,
    .bind_object_hook = bind_object
};
