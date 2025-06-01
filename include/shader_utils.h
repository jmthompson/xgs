#pragma once

#include "gl.h"

extern GLuint create_shader(const GLenum type, const GLchar *source, const GLint length);
extern GLuint link_program(const GLuint vertex_shader, const GLuint fragment_shader);
