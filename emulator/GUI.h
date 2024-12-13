#pragma once

#include <SDL.h>

namespace GUI {

void initialize(SDL_Window *, SDL_GLContext);
void shutdown(void);
void newFrame(SDL_Window *);
void render();
void processEvent(SDL_Event&);

void drawStatusBar(void);
void drawMenu(void);

}; // namespace GUI
