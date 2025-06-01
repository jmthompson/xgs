#pragma once

#include "gl.h"

#include <cstdint>
#include <SDL.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

struct TriangleVertex {
  GLfloat coord[2];
  GLfloat tex[2];
};

// Typedef representing pixel data in the video driver (32-bit color)
typedef uint32_t pixel_t;

typedef void (lineRenderer)(const unsigned int, pixel_t *);

namespace Video
{
// Pixels per line, including borders
constexpr unsigned int kPixelsPerLine = 720;
// Lines per frame
constexpr unsigned int kLinesPerFrame = 262;
// Border width. The border height is not fixed
constexpr unsigned int kBorderWidth = 40;

extern SDL_Window *window;
extern SDL_GLContext context;

extern const pixel_t standard_colors[16];

// The frame buffer
extern pixel_t frame_buffer[kPixelsPerLine * kLinesPerFrame];

extern float frame_width, frame_height;
extern float frame_left, frame_right, frame_top, frame_bottom;

extern uint8_t *display_buffer[2];
extern uint8_t *text_font;
extern pixel_t fgcolor;
extern pixel_t bgcolor;

void setup(const unsigned int, const unsigned int);
void setFullscreen(bool);
void onResize(void);

void startFrame();
void drawFrame(const pixel_t *, const unsigned int, const unsigned int);
void endFrame(void);

void renderLine_text40(const unsigned int, pixel_t *);
void renderLine_text80(const unsigned int, pixel_t *);
void renderLine_lores(const unsigned int, pixel_t *);
void renderLine_dlores(const unsigned int, pixel_t *);
void renderLine_hires(const unsigned int, pixel_t *);
void renderLine_dhires(const unsigned int, pixel_t *);
void renderLine_shr(const unsigned int, pixel_t *);

} // namespace Video
