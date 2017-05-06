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

#include "emulator/common.h"

#include "Video.h"

Video::Video(const unsigned int width, const unsigned int height)
{
    win_width = width;
    win_height = height;

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    window = SDL_CreateWindow("XGS", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, win_width, win_height, SDL_WINDOW_OPENGL);
    if (window == nullptr) {
        //printSDLError("Failed to create window");

        throw std::runtime_error("SDL_CreateWindow() failed");
    }

    glcontext = SDL_GL_CreateContext(window);
    if (glcontext == nullptr) {
        throw std::runtime_error("SDL_GL_CreateContext() failed");
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (renderer == nullptr) {
        //printSDLError("Failed to create renderer");

        throw std::runtime_error("SDL_CreateRenderer() failed");
    }

    SDL_RenderSetLogicalSize(renderer, width, height);
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
}

void Video::drawFrame(pixel_t *frame, const unsigned int width, const unsigned int height)
{
    if (width != dest_rect.w) {
        if (texture != nullptr) {
            SDL_DestroyTexture(texture);
        }

        texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, width, height);

        if (texture == nullptr) {
            throw std::runtime_error("SDL_CreateTexture() failed");
        }

        dest_rect.w = width;
        dest_rect.h = height * 2; // otherwise it will look stretched
        dest_rect.x = (win_width - dest_rect.w) / 2;
        dest_rect.y = (win_height - dest_rect.h) / 2;
    }

    SDL_UpdateTexture(texture, NULL, frame, width * sizeof(pixel_t));

    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, NULL, &dest_rect);
    SDL_RenderPresent(renderer);
}

void Video::toggleFullscreen()
{
    fullscreen = !fullscreen;

    SDL_SetWindowFullscreen(window, fullscreen? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
}
