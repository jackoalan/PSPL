//
//  d3d11_runtime_interface.h
//  PSPL
//
//  Created by Jack Andersen on 6/28/13.
//
//

#ifndef __PSPL__d3d11_runtime_interface__
#define __PSPL__d3d11_runtime_interface__

#include <d3d11.h>
#include <PSPLExtension.h>


void pspl_d3d11_init(const pspl_platform_t* platform);

void pspl_d3d11_shutdown(const pspl_platform_t* platform);


int pspl_d3d11_compile_vertex_shader(const void* shader_source, size_t length,
                                     ID3DBlob** blob_out, void** out, size_t* out_len);

int pspl_d3d11_compile_pixel_shader(const void* shader_source, size_t length,
                                    ID3DBlob** blob_out, void** out, size_t* out_len);


ID3D11VertexShader* pspl_d3d11_create_vertex_shader(LPVOID shader_binary, size_t length);

ID3D11PixelShader* pspl_d3d11_create_pixel_shader(LPVOID shader_binary, size_t length);


ID3D11Buffer* pspl_d3d11_create_constant_buffer(size_t size);


void pspl_d3d11_use_vertex_shader(ID3D11VertexShader* shader, ID3D11Buffer* constant_buffer);

void pspl_d3d11_use_pixel_shader(ID3D11PixelShader* shader);


void pspl_d3d11_destroy_something(IUnknown* something);


#endif /* defined(__PSPL__d3d11_runtime_interface__) */
