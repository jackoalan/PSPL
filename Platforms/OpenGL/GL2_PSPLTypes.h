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
#  include <OpenGL/gl.h>
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
    
} GL2_shader_object_t;

#endif
