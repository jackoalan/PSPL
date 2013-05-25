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

/* OpenGL-specific type for shader object */
typedef struct {
    
    // Shader object
    GLuint shader_obj;
    
} GL2_shader_object_t;

#endif
