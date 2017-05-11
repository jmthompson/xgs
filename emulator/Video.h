#ifndef VIDEO_H_
#define VIDEO_H_

namespace Video {

void initialize(const unsigned int width, const unsigned int height);
void drawFrame(const pixel_t *, const unsigned int, const unsigned int);
void toggleFullscreen();

}; // namespace Video

#endif // VIDEO_H_
