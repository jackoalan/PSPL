//
//  GL2_PSPLTypes.h
//  PSPL
//
//  Created by Jack Andersen on 5/25/13.
//
//

#ifndef PSPL_GL2_PSPLTypes_h
#define PSPL_GL2_PSPLTypes_h

#ifdef __APPLE__
#include <TargetConditionals.h>
#  if TARGET_OS_IPHONE
#    include <OpenGLES/ES2/gl.h>
#  else
#    include <OpenGL/gl.h>
#  endif
#else
#  include <GL/gl.h>
#endif

#include "gl_common.h"

/* OpenGL-specific type for shader object */
typedef struct {
    
    // Program object
    GLuint program;
    
    // Vertex shader
    GLuint vertex_shader;
    
    // Fragment shader
    GLuint fragment_shader;
    
    // Config structure
    gl_config_t* config;
    
    // Uniforms
    GLint bone_mat_uni, bone_base_uni;
    GLint mv_mtx_uni, mv_invxpose_uni;
    GLint proj_mtx_uni;
    GLint tc_genmtx_arr;
    
} GL2_shader_object_t;

#endif
