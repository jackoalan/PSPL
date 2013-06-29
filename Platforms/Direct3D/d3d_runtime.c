//
//  d3d_runtime.c
//  PSPL
//
//  Created by Jack Andersen on 5/25/13.
//
//

#include <stdio.h>
#include <d3d11.h>

/* Runtime method to set the Direct3D device being accessed by all
 * runtime extensions */
ID3D11Device* pspl_d3d_device = NULL;
void pspl_d3d_set_device(ID3D11Device* device) {
    pspl_d3d_device = device;
}

ID3D11DeviceContext* pspl_d3d_device_context = NULL;
void pspl_d3d_set_device_context(ID3D11DeviceContext* context) {
    pspl_d3d_device_context = context;
}
