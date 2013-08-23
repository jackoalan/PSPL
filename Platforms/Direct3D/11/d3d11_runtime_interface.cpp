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

/* GUID stuff for Microsoft ABI compatibility */
#ifdef DEFINE_GUID
#undef DEFINE_GUID
#endif
#define DEFINE_GUID(id, a, b, c, d, e, f, g, h, i, j, k) const GUID id = { a,b,c, { d,e,f,g,h,i,j,k } };

DEFINE_GUID(IID_ID3D11DeviceChild,0x1841e5c8,0x16b0,0x489b,0xbc,0xc8,0x44,0xcf,0xb0,0xd5,0xde,0xae);
DEFINE_GUID(IID_ID3D11DepthStencilState,0x03823efb,0x8d8f,0x4e1c,0x9a,0xa2,0xf6,0x4b,0xb2,0xcb,0xfd,0xf1);
DEFINE_GUID(IID_ID3D11BlendState,0x75b68faa,0x347d,0x4159,0x8f,0x45,0xa0,0x64,0x0f,0x01,0xcd,0x9a);
DEFINE_GUID(IID_ID3D11RasterizerState,0x9bb4ab81,0xab1a,0x4d8f,0xb5,0x06,0xfc,0x04,0x20,0x0b,0x6e,0xe7);
DEFINE_GUID(IID_ID3D11Resource,0xdc8e63f3,0xd12b,0x4952,0xb4,0x7b,0x5e,0x45,0x02,0x6a,0x86,0x2d);
DEFINE_GUID(IID_ID3D11Buffer,0x48570b85,0xd1ee,0x4fcd,0xa2,0x50,0xeb,0x35,0x07,0x22,0xb0,0x37);
DEFINE_GUID(IID_ID3D11Texture1D,0xf8fb5c27,0xc6b3,0x4f75,0xa4,0xc8,0x43,0x9a,0xf2,0xef,0x56,0x4c);
DEFINE_GUID(IID_ID3D11Texture2D,0x6f15aaf2,0xd208,0x4e89,0x9a,0xb4,0x48,0x95,0x35,0xd3,0x4f,0x9c);
DEFINE_GUID(IID_ID3D11Texture3D,0x037e866e,0xf56d,0x4357,0xa8,0xaf,0x9d,0xab,0xbe,0x6e,0x25,0x0e);
DEFINE_GUID(IID_ID3D11View,0x839d1216,0xbb2e,0x412b,0xb7,0xf4,0xa9,0xdb,0xeb,0xe0,0x8e,0xd1);
DEFINE_GUID(IID_ID3D11ShaderResourceView,0xb0e06fe0,0x8192,0x4e1a,0xb1,0xca,0x36,0xd7,0x41,0x47,0x10,0xb2);
DEFINE_GUID(IID_ID3D11RenderTargetView,0xdfdba067,0x0b8d,0x4865,0x87,0x5b,0xd7,0xb4,0x51,0x6c,0xc1,0x64);
DEFINE_GUID(IID_ID3D11DepthStencilView,0x9fdac92a,0x1876,0x48c3,0xaf,0xad,0x25,0xb9,0x4f,0x84,0xa9,0xb6);
DEFINE_GUID(IID_ID3D11UnorderedAccessView,0x28acf509,0x7f5c,0x48f6,0x86,0x11,0xf3,0x16,0x01,0x0a,0x63,0x80);
DEFINE_GUID(IID_ID3D11VertexShader,0x3b301d64,0xd678,0x4289,0x88,0x97,0x22,0xf8,0x92,0x8b,0x72,0xf3);
DEFINE_GUID(IID_ID3D11HullShader,0x8e5c6061,0x628a,0x4c8e,0x82,0x64,0xbb,0xe4,0x5c,0xb3,0xd5,0xdd);
DEFINE_GUID(IID_ID3D11DomainShader,0xf582c508,0x0f36,0x490c,0x99,0x77,0x31,0xee,0xce,0x26,0x8c,0xfa);
DEFINE_GUID(IID_ID3D11GeometryShader,0x38325b96,0xeffb,0x4022,0xba,0x02,0x2e,0x79,0x5b,0x70,0x27,0x5c);
DEFINE_GUID(IID_ID3D11PixelShader,0xea82e40d,0x51dc,0x4f33,0x93,0xd4,0xdb,0x7c,0x91,0x25,0xae,0x8c);
DEFINE_GUID(IID_ID3D11ComputeShader,0x4f5b196e,0xc2bd,0x495e,0xbd,0x01,0x1f,0xde,0xd3,0x8e,0x49,0x69);
DEFINE_GUID(IID_ID3D11InputLayout,0xe4819ddc,0x4cf0,0x4025,0xbd,0x26,0x5d,0xe8,0x2a,0x3e,0x07,0xb7);
DEFINE_GUID(IID_ID3D11SamplerState,0xda6fea51,0x564c,0x4487,0x98,0x10,0xf0,0xd0,0xf9,0xb4,0xe3,0xa5);
DEFINE_GUID(IID_ID3D11Asynchronous,0x4b35d0cd,0x1e15,0x4258,0x9c,0x98,0x1b,0x13,0x33,0xf6,0xdd,0x3b);
DEFINE_GUID(IID_ID3D11Query,0xd6c00747,0x87b7,0x425e,0xb8,0x4d,0x44,0xd1,0x08,0x56,0x0a,0xfd);
DEFINE_GUID(IID_ID3D11Predicate,0x9eb576dd,0x9f77,0x4d86,0x81,0xaa,0x8b,0xab,0x5f,0xe4,0x90,0xe2);
DEFINE_GUID(IID_ID3D11Counter,0x6e8c49fb,0xa371,0x4770,0xb4,0x40,0x29,0x08,0x60,0x22,0xb7,0x41);
DEFINE_GUID(IID_ID3D11ClassInstance,0xa6cd7faa,0xb0b7,0x4a2f,0x94,0x36,0x86,0x62,0xa6,0x57,0x97,0xcb);
DEFINE_GUID(IID_ID3D11ClassLinkage,0xddf57cba,0x9543,0x46e4,0xa1,0x2b,0xf2,0x07,0xa0,0xfe,0x7f,0xed);
DEFINE_GUID(IID_ID3D11CommandList,0xa24bc4d1,0x769e,0x43f7,0x80,0x13,0x98,0xff,0x56,0x6c,0x18,0xe2);
DEFINE_GUID(IID_ID3D11DeviceContext,0xc0bfa96c,0xe089,0x44fb,0x8e,0xaf,0x26,0xf8,0x79,0x61,0x90,0xda);
DEFINE_GUID(IID_ID3D11Device,0xdb6f6ddb,0xac77,0x4e88,0x82,0x53,0x81,0x9d,0xf9,0xbb,0xf1,0x40);

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




extern "C" void pspl_d3d11_destroy_something(IUnknown* something) {
    something->Release();
}

