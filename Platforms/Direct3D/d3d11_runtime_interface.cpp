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


/* Matrix uniform buffers */
#define BUF_COUNT 10
ID3D11Buffer* constant_buffers[BUF_COUNT];

void pspl_d3d11_init(const pspl_platform_t* platform) {
    int i;
    
    for (i=0 ; i<BUF_COUNT ; ++i) {
        
        D3D11_BUFFER_DESC desc = {
            .ByteWidth = sizeof(pspl_matrix34_t),
            .Usage = D3D11_USAGE_DYNAMIC,
            .BindFlags = D3D11_BIND_CONSTANT_BUFFER,
            .CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
            .MiscFlags = 0,
            .StructureByteStride = 0
        };
        pspl_d3d_device->CreateBuffer(&desc, NULL, &constant_buffers[i]);
        
    }
    
}

void pspl_d3d11_shutdown(const pspl_platform_t* platform) {
    int i;
    
    for (i=0 ; i<BUF_COUNT ; ++i)
        constant_buffers[i]->Release();
    
}

int pspl_d3d11_compile_pixel_shader(const void* shader_source, size_t length,
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
    pspl_d3d_device->CreateVertexShader(shader_binary, length, NULL, &shader);
    
    return shader;
    
}

ID3D11PixelShader* pspl_d3d11_create_pixel_shader(const void* shader_binary, size_t length) {
    
    ID3D11PixelShader* shader = NULL;
    HRESULT result =
    pspl_d3d_device->CreatePixelShader(shader_binary, length, NULL, &shader);
    
    return shader;
    
}



void pspl_d3d11_use_vertex_shader(ID3D11VertexShader* shader) {
    pspl_d3d_device_context->VSSetShader(shader, NULL, 0);
}

void pspl_d3d11_use_pixel_shader(ID3D11PixelShader* shader) {
    pspl_d3d_device_context->PSSetShader(shader, NULL, 0);
}


/* PSPL-IR routines */
static int cur_mtx = 0;
void pspl_ir_load_pos_mtx(pspl_matrix34_t* mtx) {
    D3D11_MAPPED_SUBRESOURCE rsrc;
    pspl_d3d_device_context->Map(constant_buffers[0], 0, D3D11_MAP_WRITE, 0, &rsrc);
    memcpy(rsrc.pData, mtx, sizeof(pspl_matrix34_t));
    pspl_d3d_device_context->Unmap(constant_buffers[0], 0);
}
void pspl_ir_load_norm_mtx(pspl_matrix34_t* mtx) {
    D3D11_MAPPED_SUBRESOURCE rsrc;
    pspl_d3d_device_context->Map(constant_buffers[1], 0, D3D11_MAP_WRITE, 0, &rsrc);
    memcpy(rsrc.pData, mtx, sizeof(pspl_matrix34_t));
    pspl_d3d_device_context->Unmap(constant_buffers[1], 0);
}
void pspl_ir_load_uv_mtx(pspl_matrix34_t* mtx) {
    if (cur_mtx >= 8)
        pspl_error(-1, "UV Mtx overflow", "only 8 UV XF matrices may be defined");
    D3D11_MAPPED_SUBRESOURCE rsrc;
    pspl_d3d_device_context->Map(constant_buffers[2+cur_mtx], 0, D3D11_MAP_WRITE, 0, &rsrc);
    memcpy(rsrc.pData, mtx, sizeof(pspl_matrix34_t));
    pspl_d3d_device_context->Unmap(constant_buffers[2+cur_mtx], 0);
    ++cur_mtx;
}
void pspl_ir_load_finish() {
    cur_mtx = 0;
    pspl_d3d_device_context->VSSetConstantBuffers(0, BUF_COUNT, constant_buffers);
}


void pspl_d3d11_destroy_something(IUnknown* something) {
    something->Release();
}

