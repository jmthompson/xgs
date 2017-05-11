/**
 * XGS: The Linux GS Emulator
 * Written and Copyright (C) 1996 - 2016 by Joshua M. Thompson
 *
 * You are free to distribute this code for non-commercial purposes
 * I ask only that you notify me of any changes you make to the code
 * Commercial use is prohibited without my written permission
 */

/*
 * This class handles low-level video functions such as OpenGL
 * initialization and pushing video frames out to the hardware.
 */

#include <SDL.h>
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "emulator/common.h"

#include "Video.h"
#include "shader_utils.h"

namespace Video {

struct TriangleVertex {
    GLfloat coord[2];
    GLfloat tex[2];
};

static const char vertex_shader_source[] = 
    "#version 120\n"
    "attribute vec2 coord;\n"
    "attribute vec2 tex_coord;\n"
    "uniform mat4 transform;\n"
    "varying vec2 v_tex_coord;\n"
    "void main()\n"
    "{\n"
    "    v_tex_coord = tex_coord;\n"
    "    gl_Position = transform * vec4(coord.x, coord.y, 0, 1.0);\n"
    "}\n";

static const char fragment_shader_source[] =
    "#version 120\n"
    //"precision mediump float;\n"
    "uniform sampler2D sampler;\n"
    "varying vec2 v_tex_coord;\n"
    "void main()\n"
    "{\n"
    "   vec4 tex = texture2D(sampler, v_tex_coord);\n"
    "   gl_FragColor = vec4(tex.r, tex.g, tex.b, 1.0);\n"
    "}\n";

static SDL_Window    *window;
static SDL_GLContext context;

static GLuint vertex_shader;
static GLuint fragment_shader;
static GLuint program;
static GLuint texture;
static GLuint vbo;

static GLint attribute_coord;
static GLint attribute_tex_coord;
static GLint uniform_transform;
static GLint uniform_sampler;

static glm::mat4 projection;

static bool fullscreen = false;

static unsigned int win_width;
static unsigned int win_height;
static float win_aspect;

static void initResources()
{
    //projection = glm::ortho(-1, 1, 1, -1);
    projection = glm::ortho(0.0f, (float) win_width, (float) win_height, 0.0f);

    // Set up the VBO for our triangle strip
    glGenBuffers(1, &vbo);

    // Set up our texture. These parameters are set up to allow
    // non power-of-2 textures on OpenGL ES.
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Compile and link the shaders
    vertex_shader   = create_shader(GL_VERTEX_SHADER, vertex_shader_source, sizeof(vertex_shader_source));
    fragment_shader = create_shader(GL_FRAGMENT_SHADER, fragment_shader_source, sizeof(fragment_shader_source));
    program         = link_program(vertex_shader, fragment_shader);

    attribute_coord     = glGetAttribLocation(program, "coord");
    attribute_tex_coord = glGetAttribLocation(program, "tex_coord");
    uniform_transform   = glGetUniformLocation(program, "transform");
    uniform_sampler     = glGetUniformLocation(program, "sampler");
}

void initialize(const unsigned int width, const unsigned int height)
{
    win_width  = width;
    win_height = height;
    win_aspect = (float) width / (float) height;

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    window = SDL_CreateWindow("XGS", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, win_width, win_height, SDL_WINDOW_OPENGL);
    if (window == nullptr) {
        //printSDLError("Failed to create window");

        throw std::runtime_error("SDL_CreateWindow() failed");
    }

    context = SDL_GL_CreateContext(window);
    if (context == nullptr) {
        throw std::runtime_error("SDL_GL_CreateContext() failed");
    }

    GLenum glew_status = glewInit();
    if (glew_status != GLEW_OK) {
        //cerr << "Error: glewInit: " << glewGetErrorString(glew_status) << endl;

        throw std::runtime_error("glewInit() failed");
    }

    initResources();
}

/**
 * This method actually draws the current frame from the VGC on the screen.
 * It just converts the frame buffer into a texture, and uses the texture to
 * draw a pair of triangles that fills the entire window.
 */
void drawFrame(const pixel_t *frame, const unsigned int width, const unsigned int height)
{
    unsigned int actualWidth, actualHeight;
    float leftX, rightX, topY, bottomY;

    if (win_aspect > 1.33) {
        actualHeight = win_height;
        actualWidth  = actualHeight * 1.33;
    }
    else {
        actualHeight = height * 2;
        actualWidth  = actualHeight / 1.33;
    }

    leftX   = (win_width - actualWidth) / 2;
    rightX  = leftX + actualWidth - 1;
    topY    = (win_height - actualHeight) / 2;
    bottomY = topY + actualHeight - 1;

    TriangleVertex triangles[4] = {
        { { leftX, topY }, { 0.0f, 0.0f } },
        { { leftX, bottomY }, { 0.0f, 1.0f } },
        { { rightX, topY },  { 1.0f, 0.0f } },
        { { rightX, bottomY }, { 1.0f, 1.0f } }
    };

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(triangles), triangles, GL_STATIC_DRAW);

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
 
    glUseProgram(program);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_BGRA, GL_UNSIGNED_BYTE, frame);

    glUniform1i(uniform_sampler, 0 /* GL_TEXTURE0 */);
    glUniformMatrix4fv(uniform_transform, 1, GL_FALSE, glm::value_ptr(projection));

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glVertexAttribPointer(
        attribute_coord,
        2,
        GL_FLOAT,
        GL_FALSE,
        sizeof(TriangleVertex),
        0
    );
    glVertexAttribPointer(
        attribute_tex_coord,
        2,
        GL_FLOAT,
        GL_FALSE,
        sizeof(TriangleVertex),
        (GLvoid *) offsetof(struct TriangleVertex, tex)
    );
    glEnableVertexAttribArray(attribute_coord);
    glEnableVertexAttribArray(attribute_tex_coord);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glDisableVertexAttribArray(attribute_coord);
    glDisableVertexAttribArray(attribute_tex_coord);

    SDL_GL_SwapWindow(window);
}

void toggleFullscreen()
{
    fullscreen = !fullscreen;

    SDL_SetWindowFullscreen(window, fullscreen? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
}

}; // namespace Video
