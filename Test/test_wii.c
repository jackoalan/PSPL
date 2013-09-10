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


static pmdl_draw_context_t* monkey_ctx;
static const pmdl_t* monkey_model = NULL;
static pmdl_action_ctx* rotate_action_ctx;
static pmdl_action_ctx* haha_action_ctx;
static pmdl_animation_ctx* anim_ctx;

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
    monkey_ctx->camera_view.pos.f[0] = sinf(time) * 5;
    monkey_ctx->camera_view.pos.f[1] = cosf(time) * 5;
    pmdl_update_context(monkey_ctx, PMDL_INVALIDATE_VIEW);
    
    // Update action contexts
    pmdl_action_advance(rotate_action_ctx, 0.1/60.0);
    pmdl_action_advance(haha_action_ctx, 1/60.0);
    pmdl_animation_evaluate(anim_ctx);
    
    // Draw monkey
    pmdl_draw_rigged(monkey_ctx, monkey_model, NULL);
    
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
    monkey_ctx = pmdl_new_draw_context();
    monkey_ctx->texcoord_mtx[0].m[0][0] = 0.5;
    monkey_ctx->texcoord_mtx[0].m[1][1] = -0.5;
    monkey_ctx->texcoord_mtx[0].m[0][3] = 0.5;
    monkey_ctx->texcoord_mtx[0].m[1][3] = 0.5;
    
    monkey_ctx->camera_view.pos.f[0] = 0;
    monkey_ctx->camera_view.pos.f[1] = 3;
    monkey_ctx->camera_view.pos.f[2] = 0;
    monkey_ctx->camera_view.look.f[0] = 0;
    monkey_ctx->camera_view.look.f[1] = 0;
    monkey_ctx->camera_view.look.f[2] = 0;
    monkey_ctx->camera_view.up.f[0] = 0;
    monkey_ctx->camera_view.up.f[1] = 0;
    monkey_ctx->camera_view.up.f[2] = 1;
    monkey_ctx->projection_type = PMDL_PERSPECTIVE;
    monkey_ctx->projection.perspective.fov = 55;
    monkey_ctx->projection.perspective.far = 5;
    monkey_ctx->projection.perspective.near = 1;
    monkey_ctx->projection.perspective.aspect = 1.3333;
    monkey_ctx->projection.perspective.post_translate_x = 0;
    monkey_ctx->projection.perspective.post_translate_y = 0;
    pmdl_update_context(monkey_ctx, PMDL_INVALIDATE_ALL);
    
    // Load monkey
    const pspl_runtime_psplc_t* monkey_obj = pspl_runtime_get_psplc_from_key(package, "MonkeyMaterial", 1);
    monkey_ctx->default_shader = monkey_obj;
    monkey_model = pmdl_lookup(monkey_obj, "monkey");
    
    printf("Monkey loaded\n");
    
    // Setup animation context
    haha_action_ctx = pmdl_action_init(pmdl_action_lookup(monkey_model, "haha"));
    haha_action_ctx->loop_flag = 1;
    rotate_action_ctx = pmdl_action_init(pmdl_action_lookup(monkey_model, "rotate"));
    rotate_action_ctx->loop_flag = 1;
    
    anim_ctx = pmdl_animation_initv(rotate_action_ctx, haha_action_ctx, NULL);
    
    printf("Animation Context Setup\n");
    
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

