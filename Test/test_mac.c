//
//  test_mac.c
//  PSPL
//
//  Created by Jack Andersen on 5/30/13.
//
//

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <math.h>

#include <OpenGL/OpenGL.h>
#include <GLUT/GLUT.h>

#include <PSPLRuntime.h>
#include <PMDLRuntime.h>

static unsigned frame_count = 0;
static double last_render_time = 0;
static double fps = 0;
#define USEC_PER_SEC 1000000

static pmdl_draw_context_t* monkey_ctx;
static const pmdl_t* monkey_model;
static pmdl_action_ctx* rotate_action_ctx;
static pmdl_action_ctx* haha_action_ctx;
static pmdl_animation_ctx* anim_ctx;

static void renderfunc() {
    
    
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    glEnable(GL_DEPTH_TEST);
    
    // Current time
    struct timeval tv;
    gettimeofday(&tv, NULL);
    double time = tv.tv_sec + ((double)tv.tv_usec / (double)USEC_PER_SEC);
    
    // Update action contexts
    pmdl_action_advance(rotate_action_ctx, 0.1/60.0);
    pmdl_action_advance(haha_action_ctx, 1/60.0);
    pmdl_animation_evaluate(anim_ctx);
    
    // Draw monkey
    glEnable(GL_CULL_FACE);
    pmdl_draw_rigged(monkey_ctx, monkey_model, anim_ctx);
    
    if (!((frame_count)%10)) {
        double diff = time - last_render_time;
        last_render_time = time;
        if (frame_count)
            fps = 10.0/diff;
    }
    ++frame_count;
    
    
    glutSwapBuffers();
    
    glutPostRedisplay();
    
}

static void kbfunc(unsigned char key, int x, int y) {
    if (key == 'q') {
        pspl_runtime_shutdown();
        exit(0);
    }
}

static int enumerate_psplc_hook(pspl_runtime_psplc_t* psplc_object) {
    pspl_runtime_retain_psplc(psplc_object);
    return 0;
}

static void reshape(int w, int h) {
    glViewport(0, 0, (GLsizei)w, (GLsizei)h);
    monkey_ctx->projection.perspective.aspect = (float)w / (float)h;
    pmdl_update_context(monkey_ctx, PMDL_INVALIDATE_PROJECTION);
}

#define PSPL_HASHING_BUILTIN 1
#include <PSPL/PSPLHash.h>
int main(int argc, char* argv[]) {
    
    // Setup GLUT
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA | GLUT_MULTISAMPLE);
	glutInitWindowPosition(100,100);
	glutInitWindowSize(800,600);
	glutCreateWindow("PSPL Test");
    glutDisplayFunc(renderfunc);
    glutKeyboardFunc(kbfunc);
    glutReshapeFunc(reshape);
    
    
    // Setup PSPL
    const pspl_platform_t* plat;
    pspl_runtime_init(&plat);
    const pspl_runtime_package_t* package = NULL;
    pspl_runtime_load_package_file("monkey.psplp", &package);
    pspl_runtime_enumerate_psplcs(package, enumerate_psplc_hook);
    
    // Setup monkey rendering context
    monkey_ctx = pmdl_new_draw_context();
    monkey_ctx->texcoord_mtx[1].m[0][0] = 0.5;
    monkey_ctx->texcoord_mtx[1].m[1][1] = -0.5;
    monkey_ctx->texcoord_mtx[1].m[0][3] = 0.5;
    monkey_ctx->texcoord_mtx[1].m[1][3] = 0.5;
    
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
    const pspl_runtime_psplc_t* monkey_obj = pspl_runtime_get_psplc_from_key(package, "Monkey", 1);
    monkey_ctx->default_shader = monkey_obj;
    monkey_model = pmdl_lookup(monkey_obj, "monkey");
    
    // Setup animation context
    haha_action_ctx = pmdl_action_init(pmdl_action_lookup(monkey_model, "haha"));
    haha_action_ctx->loop_flag = 1;
    rotate_action_ctx = pmdl_action_init(pmdl_action_lookup(monkey_model, "rotate"));
    rotate_action_ctx->loop_flag = 1;
    
    anim_ctx = pmdl_animation_initv(rotate_action_ctx, haha_action_ctx, NULL);
    
    // Start rendering
    glutMainLoop();
    
    return 0;
    
}

