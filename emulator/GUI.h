#ifndef GUI_H_
#define GUI_H_

#include <SDL.h>

class Emulator;

namespace GUI {

void initialize(SDL_Window *, SDL_GLContext);
void shutdown(void);
void newFrame(SDL_Window *);
void render();
void processEvent(SDL_Event&);

void drawStatusBar(Emulator&);
void drawMenu(Emulator&);

} // namespace GUI

#endif // GUI_H_
