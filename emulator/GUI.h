#ifndef GUI_H_
#define GUI_H_

#include <SDL.h>

class Emulator;

namespace GUI {

void initialize();
void shutdown(void);
void newFrame(SDL_Window *);
bool processEvent(SDL_Event&);

void drawStatusBar(Emulator&);
void drawMenu(Emulator&);

} // namespace GUI

#endif // GUI_H_
