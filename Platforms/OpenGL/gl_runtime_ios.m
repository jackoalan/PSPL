//
//  gl_runtime_ios.m
//  PSPL
//
//  Created by Jack Andersen on 6/24/13.
//
//

#include <stdlib.h>

#ifdef __APPLE__
#include <TargetConditionals.h>
#if TARGET_OS_IPHONE
#include <OpenGLES/EAGL.h>
#include <OpenGLES/ES2/gl.h>
static EAGLContext* gl_load_context = NULL;
void gl_init_load_context() {
    EAGLSharegroup* sharegroup = [EAGLContext currentContext].sharegroup;
    gl_load_context = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES2 sharegroup:sharegroup];
}
void gl_set_load_context() {
    [EAGLContext setCurrentContext:gl_load_context];
}
#endif
#endif
