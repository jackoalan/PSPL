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

static void renderfunc() {
    
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    
    glutSwapBuffers();
    
    
}

static void kbfunc(unsigned char key, int x, int y) {
    if (key == 'q') {
        pspl_runtime_shutdown();
        exit(0);
    }
}

static int enumerate_psplc_hook(const pspl_runtime_psplc_t* psplc_object) {
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

