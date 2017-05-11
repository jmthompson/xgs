#include <GL/glew.h>

#include "common.h"

static void print_log(GLuint object) {
    GLint log_length = 0;
    if (glIsShader(object)) {
        glGetShaderiv(object, GL_INFO_LOG_LENGTH, &log_length);
    } else if (glIsProgram(object)) {
        glGetProgramiv(object, GL_INFO_LOG_LENGTH, &log_length);
    } else {
        std::cerr << "printlog: Not a shader or a program" << std::endl;
        return;
    }

    char* log = (char*)malloc(log_length);

    if (glIsShader(object))
        glGetShaderInfoLog(object, log_length, NULL, log);
    else if (glIsProgram(object))
        glGetProgramInfoLog(object, log_length, NULL, log);

    std::cerr << log;
    free(log);
}

/**
 * Compile a shader and return the resulting shader ID
 */
GLuint create_shader(const GLenum type, const GLchar *source, const GLint length)
{
	GLuint id = glCreateShader(type);
    GLint status;

    if (id == 0) {
        throw std::runtime_error("Failed to create shader");
    }

	glShaderSource(id, 1, &source, NULL);
	glCompileShader(id);
	glGetShaderiv(id, GL_COMPILE_STATUS, &status);

	if (status == GL_FALSE) {
        print_log(id);

		glDeleteShader(id);

        throw std::runtime_error("Failed to compile shader");
	}
	
	return id;
}

/**
 * Link a vertex shader and a fragment shader into a program,
 * and return the resulting program ID.
 */
GLuint link_program(const GLuint vertex_shader, const GLuint fragment_shader)
{
    GLuint id = glCreateProgram();
    GLint status;
 
    glAttachShader(id, vertex_shader);
    glAttachShader(id, fragment_shader);
    glLinkProgram(id);
    glGetProgramiv(id, GL_LINK_STATUS, &status);
 
	if (status == GL_FALSE) {
        throw std::runtime_error("Failed to link program");
	}
	
    return id;
}
