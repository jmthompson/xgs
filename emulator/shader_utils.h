#ifndef _CREATE_SHADER_H
#define _CREATE_SHADER_H

#ifdef RPI
    #include "GLES2/gl2.h"
    #include "EGL/egl.h"
    #include "EGL/eglext.h"
#else
    #include "gl_33.h"
#endif
extern GLuint create_shader(const GLenum type, const GLchar *source, const GLint length);
extern GLuint link_program(const GLuint vertex_shader, const GLuint fragment_shader);

#endif
