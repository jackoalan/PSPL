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
#include <unistd.h>

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
static const pspl_runtime_arc_file_t* monkey_model = NULL;

/* GX external framebuffer */
static void* xfb = NULL;

/* Reset button pressed */
static uint8_t reset_pressed = 0;

/* Ready to render */
static uint8_t ready_to_render = 0;

/* Frame Count */
u32 cur_frame = 0;

/* Texcoord scale matrix */
/*
static pspl_matrix34_t TC_SCALE = {
    0.5, 0.0, 0.0, 0.5,
    0.0, 0.5, 0.0, 0.5,
    0.0, 0.0, 1.0, 0.0
};
 */

static void renderfunc() {
    
    // Current time
    float time = (float)cur_frame / 60.0f;
    
    // Rotate camera around monkey
    monkey_ctx.camera_view.pos[0] = sinf(time) * 5;
    monkey_ctx.camera_view.pos[2] = cosf(time) * 5;
    pmdl_update_context(&monkey_ctx, PMDL_INVALIDATE_VIEW);
    
    // Update Texcoord 1
    monkey_ctx.texcoord_mtx[1][0][0] = 0.5;
    monkey_ctx.texcoord_mtx[1][1][1] = 1.0;
    monkey_ctx.texcoord_mtx[1][0][3] = fmodf(-time, 1.0);
    monkey_ctx.texcoord_mtx[1][1][3] = 1.0;
    
    // Draw monkey
    GX_SetCullMode(GX_CULL_NONE);
    GX_SetZMode(GX_TRUE, GX_LESS, GX_TRUE);
    pmdl_draw(&monkey_ctx, monkey_model);
    
    // Swap buffers
    GX_DrawDone();
    GX_CopyDisp(xfb, GX_TRUE);
    
    //printf("Console active\n");
    
}

static int enumerate_psplc_hook(pspl_runtime_psplc_t* psplc_object) {
    pspl_runtime_retain_psplc(psplc_object);
    return 0;
}

static void reset_press_cb() {
    reset_pressed = 1;
}

static void post_retrace_cb(uint32_t rcnt) {
    cur_frame = rcnt;
    ready_to_render = 1;
}

#define PSPL_HASHING_BUILTIN 1
#include <PSPL/PSPLHash.h>
int main(int argc, char* argv[]) {
    
    // Setup video
    VIDEO_Init();
    GXRModeObj* pref_vid_mode = VIDEO_GetPreferredMode(NULL);
    VIDEO_Configure(pref_vid_mode);
    xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(pref_vid_mode));
    VIDEO_SetNextFramebuffer(xfb);

    // Console
    console_init(xfb,20,20,pref_vid_mode->fbWidth,pref_vid_mode->xfbHeight,pref_vid_mode->fbWidth*VI_DISPLAY_PIX_SZ);
    
    // Make display visible
    VIDEO_SetBlack(FALSE);
    VIDEO_Flush();
    VIDEO_WaitVSync();
    if(pref_vid_mode->viTVMode&VI_NON_INTERLACE) VIDEO_WaitVSync();
    
    printf("Console active\n");


    // Setup GX
    void *gp_fifo = NULL;
    gp_fifo = memalign(32,DEFAULT_FIFO_SIZE);
    memset(gp_fifo,0,DEFAULT_FIFO_SIZE);
    GX_Init(gp_fifo, DEFAULT_FIFO_SIZE);
    
    GXColor background = {0x0,0x0,0x0,0xff};
    GX_SetCopyClear(background, 0x00ffffff);
    
    f32 yscale;
	u32 xfbHeight;
    yscale = GX_GetYScaleFactor(pref_vid_mode->efbHeight,pref_vid_mode->xfbHeight);
	xfbHeight = GX_SetDispCopyYScale(yscale);
	GX_SetScissor(0,0,pref_vid_mode->fbWidth,pref_vid_mode->efbHeight);
	GX_SetDispCopySrc(0,0,pref_vid_mode->fbWidth,pref_vid_mode->efbHeight);
	GX_SetDispCopyDst(pref_vid_mode->fbWidth,xfbHeight);
	GX_SetCopyFilter(pref_vid_mode->aa,pref_vid_mode->sample_pattern,GX_TRUE,pref_vid_mode->vfilter);
	GX_SetFieldMode(pref_vid_mode->field_rendering,((pref_vid_mode->viHeight==2*pref_vid_mode->xfbHeight)?GX_ENABLE:GX_DISABLE));
    
	if (pref_vid_mode->aa)
		GX_SetPixelFmt(GX_PF_RGB565_Z16, GX_ZC_LINEAR);
	else
		GX_SetPixelFmt(GX_PF_RGB8_Z24, GX_ZC_LINEAR);
    
	GX_SetDispCopyGamma(GX_GM_1_0);
    
    
    // Setup SD Card
    fatInitDefault();
    fatMountSimple("sd", &__io_wiisd);
    
    printf("SD mounted\n");

    
    // Setup PSPL
    const pspl_platform_t* plat;
    pspl_runtime_init(&plat);
    const pspl_runtime_package_t* package = NULL;
    pspl_runtime_load_package_file("sd:/monkey.psplp", &package);
    pspl_runtime_enumerate_psplcs(package, enumerate_psplc_hook);
    
    printf("PSPL package read\n");
    
    // Setup monkey rendering context
    memset(monkey_ctx.texcoord_mtx, 0, 8*sizeof(pspl_matrix34_t));
    monkey_ctx.texcoord_mtx[0][0][0] = 1.0;
    monkey_ctx.texcoord_mtx[0][1][1] = -1.0;
    monkey_ctx.texcoord_mtx[0][0][3] = 1.0;
    monkey_ctx.texcoord_mtx[0][1][3] = 1.0;
    
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
    monkey_ctx.projection.perspective.far = 10;
    monkey_ctx.projection.perspective.near = 1;
    monkey_ctx.projection.perspective.aspect = 1.77;
    monkey_ctx.projection.perspective.post_translate_x = 0;
    monkey_ctx.projection.perspective.post_translate_y = 0;
    pmdl_update_context(&monkey_ctx, PMDL_INVALIDATE_ALL);
    
    // Load monkey
    const pspl_runtime_psplc_t* monkey_obj = pspl_runtime_get_psplc_from_key(package, "monkey", 1);
    monkey_ctx.default_shader = monkey_obj;
    monkey_model = pmdl_lookup(monkey_obj, "monkey");
    
    printf("Monkey loaded\n");
    
    // Start rendering
    VIDEO_SetPostRetraceCallback(post_retrace_cb);
    
    
    // Loop until reset button pressed
    SYS_SetResetCallback(reset_press_cb);
    while (!reset_pressed) {
        if (ready_to_render) {
            ready_to_render = 0;
            renderfunc();
        }
    }
    
    return 0;
    
}

