//
//  d3d11_runtime_interface.cpp
//  PSPL
//
//  Created by Jack Andersen on 6/28/13.
//
//

#include "d3d11_runtime_interface.h"
#include <d3d11.h>
#include <d3dcompiler.h>

int pspl_d3d11_compile_vertex_shader(const void* shader_source, size_t length,
                                     ID3DBlob** blob_out, void** out, size_t* out_len){
    
    ID3DBlob* shader;
    ID3DBlob* error;
    
    HRESULT result =
    D3DCompile(shader_source, length, NULL, NULL, NULL, NULL, "vs_5_0", 0, 0, &shader, &error);
    
    if (FAILED(result)) {
        *blob_out = error;
        *out = error->GetBufferPointer();
        *out_len = error->GetBufferSize();
        return -1;
    }
    
    *blob_out = shader;
    *out = shader->GetBufferPointer();
    *out_len = shader->GetBufferSize();
    return 0;
    
}

int pspl_d3d11_compile_pixel_shader(const void* shader_source, size_t length,
                                    ID3DBlob** blob_out, void** out, size_t* out_len) {
    
    ID3DBlob* shader;
    ID3DBlob* error;
    
    HRESULT result =
    D3DCompile(shader_source, length, NULL, NULL, NULL, NULL, "ps_5_0", 0, 0, &shader, &error);
    
    if (FAILED(result)) {
        *blob_out = error;
        *out = error->GetBufferPointer();
        *out_len = error->GetBufferSize();
        return -1;
    }
    
    *blob_out = shader;
    *out = shader->GetBufferPointer();
    *out_len = shader->GetBufferSize();
    return 0;
    
}


ID3D11VertexShader* pspl_d3d11_create_vertex_shader(const void* shader_binary, size_t length) {
    
    ID3D11VertexShader* shader = NULL;
    HRESULT result =
    pspl_d3d11_device->CreateVertexShader(shader_binary, length, NULL, &shader);
    
    return shader;
    
}

ID3D11PixelShader* pspl_d3d11_create_pixel_shader(const void* shader_binary, size_t length) {
    
    ID3D11PixelShader* shader = NULL;
    HRESULT result =
    pspl_d3d11_device->CreatePixelShader(shader_binary, length, NULL, &shader);
    
    return shader;
    
}


ID3D11Buffer* pspl_d3d11_create_constant_buffer(size_t size) {
    D3D11_BUFFER_DESC desc = {
        .ByteWidth = size,
        .Usage = D3D11_USAGE_DYNAMIC,
        .BindFlags = D3D11_BIND_CONSTANT_BUFFER,
        .CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
        .MiscFlags = 0,
        .StructureByteStride = 0
    };
    ID3D11Buffer* output = NULL;
    pspl_d3d11_device->CreateBuffer(&desc, NULL, &output);
    return output;
}


static ID3D11Buffer* cur_const_buffer = NULL;
void pspl_d3d11_use_vertex_shader(ID3D11VertexShader* shader, ID3D11Buffer* constant_buffer) {
    pspl_d3d11_device_context->VSSetShader(shader, NULL, 0);
    cur_const_buffer = constant_buffer;
}

void pspl_d3d11_use_pixel_shader(ID3D11PixelShader* shader) {
    pspl_d3d11_device_context->PSSetShader(shader, NULL, 0);
}


/* PSPL-IR routines */
static int cur_mtx = 0;
static D3D11_MAPPED_SUBRESOURCE rsrc;
void pspl_ir_load_pos_mtx(pspl_matrix34_t* mtx) {
    pspl_d3d11_device_context->Map(cur_const_buffer, 0, D3D11_MAP_WRITE, 0, &rsrc);
    memcpy(rsrc.pData, mtx, sizeof(pspl_matrix34_t));
}
void pspl_ir_load_norm_mtx(pspl_matrix34_t* mtx) {
    memcpy((uint8_t*)rsrc.pData + sizeof(pspl_matrix34_t), mtx, sizeof(pspl_matrix34_t));
}
void pspl_ir_load_uv_mtx(pspl_matrix34_t* mtx) {
    memcpy((uint8_t*)rsrc.pData + (sizeof(pspl_matrix34_t)*(2+cur_mtx)), mtx, sizeof(pspl_matrix34_t));
    ++cur_mtx;
}
void pspl_ir_load_finish() {
    cur_mtx = 0;
    pspl_d3d11_device_context->Unmap(cur_const_buffer, 0);
    pspl_d3d11_device_context->VSSetConstantBuffers(0, 1, &cur_const_buffer);
}


void pspl_d3d11_destroy_something(IUnknown* something) {
    something->Release();
}

