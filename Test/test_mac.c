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

static pmdl_draw_context_t monkey_ctx;
static const pspl_runtime_arc_file_t* monkey_model;

static void renderfunc() {
    
    
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    glEnable(GL_DEPTH_TEST);
    
    // Current time
    struct timeval tv;
    gettimeofday(&tv, NULL);
    double time = tv.tv_sec + ((double)tv.tv_usec / (double)USEC_PER_SEC);

    // Rotate camera around monkey
    monkey_ctx.camera_view.pos[0] = sin(time) * 5;
    monkey_ctx.camera_view.pos[2] = cos(time) * 5;
    pmdl_update_context(&monkey_ctx, PMDL_INVALIDATE_VIEW);
    
    // Update Texcoord 1
    monkey_ctx.texcoord_mtx[1][0][0] = 0.5;
    monkey_ctx.texcoord_mtx[1][1][1] = 1.0;
    monkey_ctx.texcoord_mtx[1][0][3] = fmod(-time, 1.0);
    monkey_ctx.texcoord_mtx[1][1][3] = 1.0;
    
    // Draw monkey
    glDisable(GL_CULL_FACE);
    pmdl_draw(&monkey_ctx, monkey_model);
    glHint(GL_LINE_SMOOTH_HINT, GL_DONT_CARE);
    glUseProgram(0);
    
    if (!((frame_count)%10)) {
        double diff = time - last_render_time;
        last_render_time = time;
        if (frame_count)
            fps = 10.0/diff;
    }
    ++frame_count;
    
    char fps_str[128];
    snprintf(fps_str, 128, "FPS: %.f", fps);
    size_t fps_str_len = strlen(fps_str);
    
    glDisable(GL_DEPTH_TEST);
    
    // Render
    /*
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, 800, 0.0, 600, -2.0, 500.0);
    
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(0, 585, 0);
    glScalef(0.1,0.1,0.1);
    
    int i;
    for (i=0 ; i<fps_str_len ; ++i)
        glutStrokeCharacter(GLUT_STROKE_MONO_ROMAN, fps_str[i]);
     */
    
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
    
    
    // Setup PSPL
    const pspl_platform_t* plat;
    pspl_runtime_init(&plat);
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
    
    // Start rendering
    glutMainLoop();
    
    return 0;
    
}

