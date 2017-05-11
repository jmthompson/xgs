#ifndef _CREATE_SHADER_H
#define _CREATE_SHADER_H

#include <GL/glew.h>

extern GLuint create_shader(const GLenum type, const GLchar *source, const GLint length);
extern GLuint link_program(const GLuint vertex_shader, const GLuint fragment_shader);

#endif
