//
//  TMRuntime_d3d.h
//  PSPL
//
//  Created by Jack Andersen on 6/28/13.
//
//

#ifndef __PSPL__TMRuntime_d3d__
#define __PSPL__TMRuntime_d3d__

#include <d3d11.h>


#ifdef __cplusplus
extern "C" {
#endif
    
    ID3D11ShaderResourceView* pspl_d3d_create_texture(D3D11_TEXTURE2D_DESC* desc, BYTE* data_buf);
    
    void pspl_d3d_bind_texture_array(ID3D11ShaderResourceView** array, unsigned count);
    
    void pspl_d3d_destroy_texture(ID3D11ShaderResourceView* texture);
    
#ifdef __cplusplus
}
#endif

#endif /* defined(__PSPL__TMRuntime_d3d__) */
