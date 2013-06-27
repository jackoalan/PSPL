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

#include <OpenGL/OpenGL.h>
#include <GLUT/GLUT.h>

#include <PSPLRuntime.h>

static double last_render_time = 0;
static double fps_avg = 0;
#define USEC_PER_SEC 1000000

static void renderfunc() {
    
    glUseProgram(0);
    
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);
        
    struct timeval tv;
    gettimeofday(&tv, NULL);
    double time = tv.tv_sec + ((double)tv.tv_usec / (double)USEC_PER_SEC);
    double diff = time - last_render_time;
    
    fps_avg *= 14.0/15.0;
    fps_avg += 1.0/diff/15.0;
    
    char fps_str[128];
    snprintf(fps_str, 128, "FPS: %.f", fps_avg);
    size_t fps_str_len = strlen(fps_str);
    
    last_render_time = time;
    
    // Render
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

int main(int argc, char* argv[]) {
    
    // Setup GLUT
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
	glutInitWindowPosition(100,100);
	glutInitWindowSize(800,600);
	glutCreateWindow("PSPL Test");
    glutDisplayFunc(renderfunc);
    glutKeyboardFunc(kbfunc);
    
    // Setup PSPL
    const pspl_platform_t* plat;
    pspl_runtime_init(&plat);
    const pspl_runtime_package_t* package = NULL;
    pspl_runtime_load_package_file("rtest.psplp", &package);
    pspl_runtime_enumerate_psplcs(package, enumerate_psplc_hook);
    
    // Start rendering
    glutMainLoop();
    
    return 0;
    
}

