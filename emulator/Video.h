#ifndef VIDEO_H_
#define VIDEO_H_

#include <SDL.h>

#include "emulator/common.h"

class Video {
    public:
        Video(const unsigned int w, const unsigned int h);
        virtual ~Video() = default;

        void drawFrame(pixel_t *, const unsigned int, const unsigned int);
        void toggleFullscreen();

    private:
        SDL_Rect      dest_rect;
        SDL_Window    *window;
        SDL_Renderer  *renderer;
        SDL_Texture   *texture = nullptr;
        SDL_GLContext glcontext;

        bool fullscreen = false;

        unsigned int win_width;
        unsigned int win_height;
};

#endif // VIDEO_H_
