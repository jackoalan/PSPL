//
//  gl_runtime.c
//  PSPL
//
//  Created by Jack Andersen on 5/25/13.
//
//

#include <stdlib.h>
#include <PSPLExtension.h>
#include "gl_common.h"

static const char* GLES_HEAD = "#define GLES\n";
static const char* GL_HEAD = "";
static const char* HEAD = NULL;

static void load_object(pspl_runtime_psplc_t* object) {
    int i;
    
    // Determine if we're using GLES
    if (!HEAD) {
        HEAD = GL_HEAD;
        const char* version = (char*)glGetString(GL_VERSION);
        if (!strncmp(version, "OpenGL ES", sizeof("OpenGL ES")))
            HEAD = GLES_HEAD;
    }
    
    // Config structure
    pspl_data_object_t config_struct;
    pspl_runtime_get_embedded_data_object_from_integer(object, GL_CONFIG_STRUCT, &config_struct);
    object->native_shader.config = config_struct.object_data;
    
    // Generate platform shader objects
    object->native_shader.program = glCreateProgram();
    glBindAttribLocation(object->native_shader.program, 0, "pos");
    glBindAttribLocation(object->native_shader.program, 1, "norm");
    for (i=0 ; i<object->native_shader.config->uv_attr_count ; ++i) {
        char uv[8];
        snprintf(uv, 8, "uv%u", i);
        glBindAttribLocation(object->native_shader.program, 2+i, uv);
    }
    GLint compile_result;
    
    // Vertex
    object->native_shader.vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    pspl_data_object_t vertex_source;
    pspl_runtime_get_embedded_data_object_from_integer(object, GL_VERTEX_SOURCE, &vertex_source);
    const GLchar* vsource[] = {HEAD, vertex_source.object_data};
    glShaderSource(object->native_shader.vertex_shader, 2, vsource, NULL);
    glCompileShader(object->native_shader.vertex_shader);
    glGetShaderiv(object->native_shader.vertex_shader, GL_COMPILE_STATUS, &compile_result);
    if (compile_result != GL_TRUE) {
        GLint log_len = 0;
        glGetShaderiv(object->native_shader.vertex_shader, GL_INFO_LOG_LENGTH, &log_len);
        char* log = malloc(log_len);
        glGetShaderInfoLog(object->native_shader.vertex_shader, log_len, NULL, log);
        pspl_error(-1, "GLES Vertex Shader Compile Failure", "%s", log);
    }
    glAttachShader(object->native_shader.program, object->native_shader.vertex_shader);
    
    // Fragment
    object->native_shader.fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    pspl_data_object_t fragment_source;
    pspl_runtime_get_embedded_data_object_from_integer(object, GL_FRAGMENT_SOURCE, &fragment_source);
    const GLchar* fsource[] = {HEAD, fragment_source.object_data};
    glShaderSource(object->native_shader.fragment_shader, 2, fsource, NULL);
    glCompileShader(object->native_shader.fragment_shader);
    glGetShaderiv(object->native_shader.fragment_shader, GL_COMPILE_STATUS, &compile_result);
    if (compile_result != GL_TRUE) {
        GLint log_len = 0;
        glGetShaderiv(object->native_shader.fragment_shader, GL_INFO_LOG_LENGTH, &log_len);
        char* log = malloc(log_len);
        glGetShaderInfoLog(object->native_shader.fragment_shader, log_len, NULL, log);
        pspl_error(-1, "GLES Fragment Shader Compile Failure", "%s", log);
    }
    glAttachShader(object->native_shader.program, object->native_shader.fragment_shader);
    
    // Link
    glLinkProgram(object->native_shader.program);
    glGetProgramiv(object->native_shader.program, GL_LINK_STATUS, &compile_result);
    if (compile_result != GL_TRUE) {
        GLint log_len = 0;
        glGetProgramiv(object->native_shader.program, GL_INFO_LOG_LENGTH, &log_len);
        char* log = malloc(log_len);
        glGetProgramInfoLog(object->native_shader.program, log_len, NULL, log);
        pspl_error(-1, "GLES Program Link Failure", "%s", log);
    }
    
}

static void unload_object(pspl_runtime_psplc_t* object) {
    
    glDeleteProgram(object->native_shader.program);
    glDeleteShader(object->native_shader.vertex_shader);
    glDeleteShader(object->native_shader.fragment_shader);
    
}

static void bind_object(pspl_runtime_psplc_t* object) {
    
    glUseProgram(object->native_shader.program);
    
    if (object->native_shader.config->depth_write == ENABLED)
        glDepthMask(GL_TRUE);
    else if (object->native_shader.config->depth_write == DISABLED)
        glDepthMask(GL_FALSE);
    
    if (object->native_shader.config->depth_test == ENABLED)
        glEnable(GL_DEPTH_TEST);
    else if (object->native_shader.config->depth_test == DISABLED)
        glDisable(GL_DEPTH_TEST);
    
}

pspl_runtime_platform_t GL2_runplat = {
    .load_object_hook = load_object,
    .unload_object_hook = unload_object,
    .bind_object_hook = bind_object
};

