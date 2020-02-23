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

#include <glm/gtc/type_ptr.hpp>

#ifdef RPI
#include <bcm_host.h>
#endif

#include "emulator/common.h"

#include "Emulator.h"
#include "Video.h"
#include "shader_utils.h"

static const char vertex_shader_source[] = 
    "#version 100\n"
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
    "#version 100\n"
    "precision mediump float;\n"
    "uniform sampler2D sampler;\n"
    "varying vec2 v_tex_coord;\n"
    "void main()\n"
    "{\n"
    "   vec4 tex = texture2D(sampler, v_tex_coord);\n"
    "   gl_FragColor = vec4(tex.r, tex.g, tex.b, 1.0);\n"
    "}\n";

Video::Video(const unsigned int width, const unsigned int height)
{
    video_width  = width;
    video_height = height;

    
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 1);

#ifdef RPI
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    uint32_t disp_width, disp_height;

    if (graphics_get_display_size(0 /* LCD */, &disp_width, &disp_height) < 0) {
        throw std::runtime_error("Unable to get display size");
    }

    window = SDL_CreateWindow("XGS", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, disp_width, disp_height, SDL_WINDOW_OPENGL|SDL_WINDOW_FULLSCREEN_DESKTOP);
#else
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    window = SDL_CreateWindow("XGS", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_OPENGL);
#endif
    if (window == nullptr) {
        throw std::runtime_error(SDL_GetError());
    }

    context = SDL_GL_CreateContext(window);
    if (context == nullptr) {
        throw std::runtime_error(SDL_GetError());
    }

    initResources();
#ifdef RPI
    onResize(disp_width, disp_height);
#else
    onResize(width, height);
#endif
}

Video::~Video()
{
}

/**
 * Prepare the window for a new video frame
 */
void Video::startFrame()
{
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

/**
 * This method actually draws the current frame from the VGC on the screen.
 * It just converts the frame buffer into a texture, and uses the texture to
 * draw a pair of triangles that fills the entire window.
 */
void Video::drawFrame(const pixel_t *frame, const unsigned int width, const unsigned int height)
{
    projection = glm::ortho(0.0f, (float) width, (float) height, 0.0f);

    TriangleVertex triangles[4] = {
        { { 0, 0}, { 0.0f, 0.0f } },
        { { 0, (float) height }, { 0.0f, 1.0f } },
        { { (float) width, 0 },  { 1.0f, 0.0f } },
        { { (float) width, (float) height }, { 1.0f, 1.0f } }
    };

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(triangles), triangles, GL_DYNAMIC_DRAW);

    glUseProgram(program);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, frame);

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
}

void Video::endFrame()
{
    SDL_GL_SwapWindow(window);
}

void Video::setFullscreen(bool enabled)
{
#ifndef RPI
    SDL_SetWindowFullscreen(window, enabled? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
#endif
}

void Video::onResize(const unsigned int width, const unsigned int height)
{
    win_width  = width;
    win_height = height;

    float hscale = win_width / video_width;
    float vscale = win_height / video_height;

    if (hscale < vscale) {
        frame_height = video_height * hscale;
        frame_width  = video_width * hscale;
    }
    else {
        frame_height = video_height * vscale;
        frame_width  = video_width * vscale;
    }

    frame_left   = (win_width - frame_width) / 2;
    frame_right  = frame_left + frame_width - 1;
    frame_top    = (win_height - frame_height) / 2;
    frame_bottom = frame_top + frame_height - 1;

    glViewport(frame_left, frame_top, frame_width, frame_height);
    assert(glGetError() == GL_NO_ERROR);
}

void Video::initResources()
{
    // Set up the VBO for our triangle strip
    glGenBuffers(1, &vbo);

    // Set up our texture.
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
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
