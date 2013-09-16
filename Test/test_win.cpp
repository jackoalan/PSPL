//
//  test_win.c
//  PSPL
//
//  Created by Jack Andersen on 5/30/13.
//
//

#include <PSPL/PSPLMinGWD3D11Support.h>
#include <stdio.h>
#include <windows.h>
#include <d3d11.h>

#include <PSPLRuntime.h>
#include <PMDLRuntime.h>


static unsigned frame_count = 0;
static double last_render_time = 0;
static double fps = 0;
#define USEC_PER_SEC 1000000

static pmdl_draw_ctx monkey_ctx;
static const pspl_runtime_arc_file_t* monkey_model;


static int enumerate_psplc_hook(pspl_runtime_psplc_t* psplc_object) {
    pspl_runtime_retain_psplc(psplc_object);
    return 0;
}

int main(int argc, char* argv[]) {
    
    // Initialise Direct3D 11 API
    ID3D11Device* d3d11_device = NULL;
    ID3D11DeviceContext* d3d11_context = NULL;
    
    const D3D_FEATURE_LEVEL feature_levels[] = {
        D3D_FEATURE_LEVEL_11_0
    };
    D3D_FEATURE_LEVEL actual_level;
    if (FAILED(D3D11CreateDevice(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, feature_levels, 1,
                                 D3D11_SDK_VERSION, &d3d11_device, &actual_level, &d3d11_context))) {
        pspl_error(-1, "Unable to initialise Direct3D", "`D3D11CreateDevice` returned FAILED");
        return -1;
    }
    
    
    // Provide Direct3D device objects to PSPL
    pspl_d3d11_set_device(d3d11_device);
    pspl_d3d11_set_device_context(d3d11_context);
    
    
    if(pspl_runtime_init(NULL) < 0) {
        pspl_error(-1, "Unable to initialise PSPL", "something failed");
        return -1;
    }
    
    const pspl_runtime_package_t* package = NULL;
    pspl_runtime_load_package_file("monkey.psplp", &package);
    pspl_runtime_enumerate_psplcs(package, enumerate_psplc_hook);
    
    // Setup monkey rendering context
    memset(monkey_ctx.texcoord_mtx, 0, 8*sizeof(pspl_matrix34_t));
    monkey_ctx.texcoord_mtx[0][0][0] = 0.5;
    monkey_ctx.texcoord_mtx[0][1][1] = -0.5;
    monkey_ctx.texcoord_mtx[0][0][3] = 0.5;
    monkey_ctx.texcoord_mtx[0][1][3] = 0.5;
    
    memset(monkey_ctx.model_mtx, 0, sizeof(pspl_matrix34_t));
    monkey_ctx.model_mtx[0][0] = 1;
    monkey_ctx.model_mtx[1][1] = 1;
    monkey_ctx.model_mtx[2][2] = 1;
    monkey_ctx.camera_view.pos[0] = 0;
    monkey_ctx.camera_view.pos[1] = 0;
    monkey_ctx.camera_view.pos[2] = 5;
    monkey_ctx.camera_view.look[0] = 0;
    monkey_ctx.camera_view.look[1] = 0;
    monkey_ctx.camera_view.look[2] = 0;
    monkey_ctx.camera_view.up[0] = 0;
    monkey_ctx.camera_view.up[1] = 1;
    monkey_ctx.camera_view.up[2] = 0;
    monkey_ctx.projection_type = PMDL_PERSPECTIVE;
    monkey_ctx.projection.perspective.fov = 55;
    monkey_ctx.projection.perspective.far = 5;
    monkey_ctx.projection.perspective.near = 1;
    monkey_ctx.projection.perspective.aspect = 1.3333;
    monkey_ctx.projection.perspective.post_translate_x = 0;
    monkey_ctx.projection.perspective.post_translate_y = 0;
    pmdl_update_context(&monkey_ctx, PMDL_INVALIDATE_ALL);
    
    // Load monkey
    const pspl_runtime_psplc_t* monkey_obj = pspl_runtime_get_psplc_from_key(package, "monkey", 1);
    monkey_ctx.default_shader = monkey_obj;
    monkey_model = pmdl_lookup(monkey_obj, "monkey");
    
    
    return 0;
}
