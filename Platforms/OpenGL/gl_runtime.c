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

static const GLint TEX_IDX_ARRAY[16] = {
    0,1,2,3,4,5,6,7,
    8,9,10,11,12,13,14,15
};

static void load_object(pspl_runtime_psplc_t* object) {
    int i;
    
    // Determine if we're using GLES
    if (!HEAD) {
        HEAD = GL_HEAD;
        const char* version = (char*)glGetString(GL_VERSION);
        if (!strncmp(version, "OpenGL ES", 9))
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
        pspl_error(-1, "GLSL Vertex Shader Compile Failure", "%.*s", log_len, log);
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
        pspl_error(-1, "GLSL Fragment Shader Compile Failure", "%.*s", log_len, log);
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
        pspl_error(-1, "GLSL Program Link Failure", "%.*s", log_len, log);
    }
    glUseProgram(object->native_shader.program);
    
    // Texture map uniforms
    GLint texs_uniform = glGetUniformLocation(object->native_shader.program, "texs");
    if (texs_uniform >= 0) {
        GLsizei map_count = (object->native_shader.config->texmap_count>16)?
                            16:object->native_shader.config->texmap_count;
        glUniform1iv(texs_uniform, map_count, TEX_IDX_ARRAY);
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
    
    if (object->native_shader.config->blending == ENABLED) {
        glEnable(GL_BLEND);
        
        GLenum src_fac = GL_SRC_ALPHA;
        unsigned data_source = object->native_shader.config->source_factor & 0x3;
        unsigned inverse = object->native_shader.config->source_factor & 0x4;
        if (data_source == SRC_COLOUR)
            src_fac = (inverse)?GL_ONE_MINUS_SRC_COLOR:GL_SRC_COLOR;
        else if (data_source == DST_COLOUR)
            src_fac = (inverse)?GL_ONE_MINUS_DST_COLOR:GL_DST_COLOR;
        else if (data_source == SRC_ALPHA)
            src_fac = (inverse)?GL_ONE_MINUS_SRC_ALPHA:GL_SRC_ALPHA;
        else if (data_source == DST_ALPHA)
            src_fac = (inverse)?GL_ONE_MINUS_DST_ALPHA:GL_DST_ALPHA;
        
        GLenum dest_fac = GL_ONE_MINUS_SRC_ALPHA;
        data_source = object->native_shader.config->dest_factor & 0x3;
        inverse = object->native_shader.config->dest_factor & 0x4;
        if (data_source == SRC_COLOUR)
            dest_fac = (inverse)?GL_ONE_MINUS_SRC_COLOR:GL_SRC_COLOR;
        else if (data_source == DST_COLOUR)
            dest_fac = (inverse)?GL_ONE_MINUS_DST_COLOR:GL_DST_COLOR;
        else if (data_source == SRC_ALPHA)
            dest_fac = (inverse)?GL_ONE_MINUS_SRC_ALPHA:GL_SRC_ALPHA;
        else if (data_source == DST_ALPHA)
            dest_fac = (inverse)?GL_ONE_MINUS_DST_ALPHA:GL_DST_ALPHA;
        
        glBlendFunc(src_fac, dest_fac);
    } else if (object->native_shader.config->blending == DISABLED)
        glDisable(GL_BLEND);
    
}

pspl_runtime_platform_t GL2_runplat = {
    .load_object_hook = load_object,
    .unload_object_hook = unload_object,
    .bind_object_hook = bind_object
};

