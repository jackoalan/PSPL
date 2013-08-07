//
//  test_wii.c
//  PSPL
//
//  Created by Jack Andersen on 5/30/13.
//
//

#include <stdio.h>
#include <malloc.h>
#include <sys/time.h>
#include <math.h>

#include <PSPLRuntime.h>
#include <PMDLRuntime.h>

#include <ogcsys.h>
#include <ogc/gx.h>
#include <ogc/system.h>
#define __wii__ 1
#include <fat.h>
#include <sdcard/wiisd_io.h>

#define USEC_PER_SEC 1000000

#define DEFAULT_FIFO_SIZE (256 * 1024)

static pmdl_draw_context_t monkey_ctx;
static const pspl_runtime_arc_file_t* monkey_model;

/* GX embedded framebuffer */
void* efb = NULL;

static void renderfunc(u32 r_cnt) {
    
        
    // Current time
    struct timeval tv;
    gettimeofday(&tv, NULL);
    double time = tv.tv_sec + ((double)tv.tv_usec / (double)USEC_PER_SEC);
    
    // Rotate camera around monkey
    monkey_ctx.camera_view.pos[0] = sin(time) * 5;
    monkey_ctx.camera_view.pos[2] = cos(time) * 5;
    pmdl_update_context(&monkey_ctx, PMDL_INVALIDATE_VIEW);
    
    // Draw monkey
    GX_SetCullMode(GX_CULL_NONE);
    GX_SetZMode(GX_TRUE, GX_LESS, GX_TRUE);
    pmdl_draw(&monkey_ctx, monkey_model);
    
    
    // Swap buffers
    GX_CopyDisp(efb, GX_TRUE);
    
}

static int enumerate_psplc_hook(pspl_runtime_psplc_t* psplc_object) {
    pspl_runtime_retain_psplc(psplc_object);
    return 0;
}

int main(int argc, char* argv[]) {
    
    // Framebuffer
    efb = SYS_AllocateFramebuffer(&TVNtsc480Prog);
    
    // Setup video
    VIDEO_Init();
    VIDEO_Configure(&TVNtsc480Prog);
    VIDEO_SetNextFramebuffer(efb);
    
    // Setup GX
    void *gp_fifo = NULL;
    gp_fifo = memalign(32,DEFAULT_FIFO_SIZE);
    memset(gp_fifo,0,DEFAULT_FIFO_SIZE);
    GX_Init(gp_fifo, DEFAULT_FIFO_SIZE);
    
    GXColor background = {0,0,0,0xff};
    GX_SetCopyClear(background, 0x00ffffff);
    
    // Setup SD Card
    fatInitDefault();
    fatMountSimple("sd", &__io_wiisd);
    
    // Setup PSPL
    const pspl_platform_t* plat;
    pspl_runtime_init(&plat);
    const pspl_runtime_package_t* package = NULL;
    pspl_runtime_load_package_file("sd:/rtest.psplp", &package);
    pspl_runtime_enumerate_psplcs(package, enumerate_psplc_hook);
    
    // Setup monkey rendering context
    memset(monkey_ctx.texcoord_mtx, 0, 8*sizeof(pspl_matrix34_t));
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
    const pspl_runtime_psplc_t* rtest = pspl_runtime_get_psplc_from_key(package, "rtest", 1);
    monkey_ctx.default_shader = rtest;
    monkey_model = pmdl_lookup(rtest, "monkey");
    
    // Start rendering
    VIDEO_SetPostRetraceCallback(renderfunc);
    
    return 0;
    
}