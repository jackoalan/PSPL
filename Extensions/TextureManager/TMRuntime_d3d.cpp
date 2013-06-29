//
//  TMRuntime_d3d.cpp
//  PSPL
//
//  Created by Jack Andersen on 6/28/13.
//
//

#include "TMRuntime_d3d.h"
#include <PSPLExtension.h>
    
#define MAX_MIPS 13

ID3D11ShaderResourceView* pspl_d3d_create_texture(D3D11_TEXTURE2D_DESC* desc, BYTE* data_buf) {
    
    if (desc->MipLevels > 13)
        pspl_error(-1, "Too many mip levels",
                   "a maximum of 13 mip levels are supported");
    
    
    size_t fmt_size_num = 1;
    size_t fmt_size_denom = 1;
    
    if (desc->Format == DXGI_FORMAT_R8G8B8A8_UINT) {
        fmt_size_num = 4;
    } else if (desc->Format == DXGI_FORMAT_R8_UINT) {
        fmt_size_num = 1;
    } else if (desc->Format == DXGI_FORMAT_R8G8_UINT) {
        fmt_size_num = 2;
    } else if (desc->Format == DXGI_FORMAT_BC1_TYPELESS) {
        fmt_size_denom = 2;
    } else if (desc->Format == DXGI_FORMAT_BC2_TYPELESS) {
        fmt_size_denom = 1;
    } else if (desc->Format == DXGI_FORMAT_BC3_TYPELESS) {
        fmt_size_denom = 1;
    } else
        pspl_error(-1, "Unsupported texture format",
                   "an unsupported texture about to be loaded into Direct3D");
    
    
    D3D11_SUBRESOURCE_DATA srdata[MAX_MIPS];
    
    unsigned width = desc->Width;
    unsigned height = desc->Height;
    BYTE* data_cur = data_buf;
    int i;
    for (i=0 ; i<desc->MipLevels ; ++i) {
        srdata[i].pSysMem = data_cur;
        srdata[i].SysMemPitch = width * fmt_size_num / fmt_size_denom;
        data_cur += width * height * fmt_size_num / fmt_size_denom;
        width /= 2;
        if (!width)
            width = 1;
        height /=2;
        if (!height)
            height = 1;
        if (desc->Format == DXGI_FORMAT_BC1_TYPELESS ||
            desc->Format == DXGI_FORMAT_BC2_TYPELESS ||
            desc->Format == DXGI_FORMAT_BC3_TYPELESS) {
            if (width < 4)
                width = 4;
            if (height < 4)
                height = 4;
        }
    }
    
    
    ID3D11Texture2D* tex = NULL;
    pspl_d3d_device->CreateTexture2D(desc, srdata, &tex);
    ID3D11ShaderResourceView* rsrc_out = NULL;
    pspl_d3d_device->CreateShaderResourceView(tex, NULL, &rsrc_out);
    tex->Release();
    
    return rsrc_out;
    
}

void pspl_d3d_bind_texture_array(ID3D11ShaderResourceView** array, unsigned count) {
    pspl_d3d_device_context->PSSetShaderResources(0, count, array);
}

void pspl_d3d_destroy_texture(ID3D11ShaderResourceView* texture) {
    texture->Release();
}

