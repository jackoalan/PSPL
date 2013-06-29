//
//  D3D11_PSPLTypes.h
//  PSPL
//
//  Created by Jack Andersen on 5/25/13.
//
//

#ifndef PSPL_D3D11_PSPLTypes_h
#define PSPL_D3D11_PSPLTypes_h

#include <d3d11.h>

/* Direct3D-specific type for shader object */
typedef struct {
    
    // Shader object
    
    
} D3D11_shader_object_t;

/* Runtime method to set the Direct3D device being accessed by all 
 * runtime extensions */
extern ID3D11Device* pspl_d3d_device;
void pspl_d3d_set_device(ID3D11Device* device);

extern ID3D11DeviceContext* pspl_d3d_device_context;
void pspl_d3d_set_device_context(ID3D11DeviceContext* context);

#endif
