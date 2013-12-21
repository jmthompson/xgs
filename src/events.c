/*********************************************************************
 *                                                                   *
 *                     XGS : Apple IIGS Emulator                     *
 *                                                                   *
 *        Written and Copyright (C)1996 by Joshua M. Thompson        *
 *                                                                   *
 *  You are free to distribute this code for non-commercial purposes *
 * I ask only that you notify me of any changes you make to the code *
 *     Commercial use is prohibited without my written permission    *
 *                                                                   *
 *********************************************************************/

/*
 * File: events.c
 *
 * Handle SDL events and translate them to ADB events
 */

#include "xgs.h"

#include "adb.h"
#include "events.h"
#include "hardware.h"
#include "video.h"

int mouse_grabbed;

static int fullscreen = SDL_FALSE;

static void handleKeyDown(SDL_KeyboardEvent *);
static void handleKeyUp(SDL_KeyboardEvent *);
static void handleJoyButton(SDL_JoyButtonEvent *);
static void handleJoyMotion(SDL_JoyAxisEvent *);
static void handleMouseBtnDown(SDL_MouseButtonEvent *);
static void handleMouseBtnUp(SDL_MouseButtonEvent *);
static void handleMouseMotion(SDL_MouseMotionEvent *);
static void handleWindow(SDL_WindowEvent *);

int eventsInit()
{
    return 0;
}

void eventsShutdown()
{
}

void eventsUpdate()
{
    SDL_Event event;
  
    while(SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_KEYDOWN:
                handleKeyDown(&event.key);
                break;
            case SDL_KEYUP:
                handleKeyUp(&event.key);
                break;
            case SDL_JOYBUTTONDOWN:
            case SDL_JOYBUTTONUP:
                handleJoyButton(&event.jbutton);
                break;
            case SDL_JOYAXISMOTION:
                handleJoyMotion(&event.jaxis);
                break;
            case SDL_MOUSEBUTTONDOWN:
                handleMouseBtnDown(&event.button);
                break;
            case SDL_MOUSEBUTTONUP:
                handleMouseBtnUp(&event.button);
                break;
            case SDL_MOUSEMOTION:
                handleMouseMotion(&event.motion);
                break;
            case SDL_WINDOWEVENT:
                handleWindow(&event.window);
                break;
        }
    }
}

static void handleKeyDown(SDL_KeyboardEvent *event)
{
    SDL_Keycode sym = event->keysym.sym;

    switch(sym) {
        case SDLK_CAPSLOCK:
            ski_modifier_reg |= 0x04;
            break;
        case SDLK_LSHIFT:
        case SDLK_RSHIFT:
            ski_modifier_reg |= 0x01;
            break;
        case SDLK_LCTRL:
        case SDLK_RCTRL:
            ski_modifier_reg |= 0x02;
            break;
        case SDLK_LALT:
            ski_modifier_reg |= 0x80;
            break;
        case SDLK_RALT:
            ski_modifier_reg |= 0x40;
            break;
        default:
            break;
    }
}

static void handleKeyUp(SDL_KeyboardEvent *event)
{
    SDL_Keycode sym = event->keysym.sym;
    char key = 0x00;

    switch(sym) {
        case SDLK_CAPSLOCK:
            ski_modifier_reg &= ~0x04;
            break;
        case SDLK_LSHIFT:
        case SDLK_RSHIFT:
            ski_modifier_reg &= ~0x01;
            break;
        case SDLK_LCTRL:
        case SDLK_RCTRL:
            ski_modifier_reg &= ~0x02;
            break;
        case SDLK_LALT:    
            ski_modifier_reg &= ~0x80;
            break;
        case SDLK_RALT:
            ski_modifier_reg &= ~0x40;
            break;
        case SDLK_F5:
            mouse_grabbed ^= 0x01;
            SDL_SetRelativeMouseMode(mouse_grabbed);
            break;
        case SDLK_F11:
            fullscreen ^= 0x01;
            videoSetFullscreen(fullscreen);
            break;
        case SDLK_PAGEDOWN:
            if (ski_modifier_reg & 0x02) hardwareSetTrace(1);
            break;
        case SDLK_PAGEUP:
            if (ski_modifier_reg & 0x02) hardwareSetTrace(0);
            break;
        case SDLK_HOME:
            if (ski_modifier_reg & 0x02) hardwareReset();
            break;
        case SDLK_END:
            if (ski_modifier_reg & 0x02) schedulerStop();
            break;
        case SDLK_PAUSE:
            hardwareRaiseNMI();
            break;
        case SDLK_LEFT:
            key = 0x08;
            break;
        case SDLK_RIGHT:
            key = 0x15;
            break;
        case SDLK_UP:
            key = 0x0B;
            break;
        case SDLK_DOWN:
            key = 0x0A;
            break;
        case SDLK_KP_ENTER:
            key = 0x8D;
            break;
        case SDLK_KP_0:
            key = 0xB0;
            break;
        case SDLK_KP_1:
            key = 0xB1;
            break;
        case SDLK_KP_2:
            key = 0xB2;
            break;
        case SDLK_KP_3:
            key = 0xB3;
            break;
        case SDLK_KP_4:
            key = 0xB4;
            break;
        case SDLK_KP_5:
            key = 0xB5;
            break;
        case SDLK_KP_6:
            key = 0xB6;
            break;
        case SDLK_KP_7:
            key = 0xB7;
            break;
        case SDLK_KP_8:
            key = 0xB8;
            break;
        case SDLK_KP_9:
            key = 0xB9;
            break;
        case SDLK_KP_MULTIPLY:
            key = 0xAA;
            break;
        case SDLK_KP_PLUS:
            key = 0xAB;
            break;
        case SDLK_KP_MINUS:
            key = 0xAD;
            break;
        case SDLK_KP_PERIOD:
            key = 0xAE;
            break;
        case SDLK_KP_DIVIDE:
            key = 0xAF;
            break;
        case SDLK_KP_EQUALS:
            key = 0xBD;
            break;
        case SDLK_ESCAPE:
            if ((ski_modifier_reg & 0x82) == 0x82) {
                if (!ski_status_irq) {
                    ski_status_irq = 0x20;
                    key = 0x1B;
                    m65816_addIRQ();
                }
            }
            else {
                key = 0x1B;
            }
            break;
        default:
            if (!(sym & 0x40000000)) {
                key = (char) sym;
            }

            break;
    }

    if (key) {
        ski_input_buffer[ski_input_index++] = key;
        if (ski_input_index == ADB_INPUT_BUFFER) ski_input_index = 0;
    }
}

static void handleJoyButton(SDL_JoyButtonEvent *event)
{
#if 0
            // down, up
             if (adb_grab_mode == 1) {        /* Joystick */
                 switch(event.xbutton.button) {
                    case Button1 :    ski_modifier_reg |= 0x80;
                             break;
                    case Button3 :    ski_modifier_reg |= 0x40;
                             break;
                    default :    break;
                 }

                 switch(event.xbutton.button) {
                    case Button1 :    ski_modifier_reg &= ~0x80;
                             break;
                    case Button3 :    ski_modifier_reg &= ~0x40;
                             break;
                    default :    break;
                 }
#endif
}

static void handleJoyMotion(SDL_JoyAxisEvent *event)
{
#if 0
                 adb_pdl0 = (int) (event.xmotion.x / 2.2);
                 adb_pdl1 = (int) (event.xmotion.y / 1.5);
                if (adb_pdl0 > 255) adb_pdl0 = 255;
                if (adb_pdl1 > 255) adb_pdl1 = 255;
                 if (event.xmotion.state & Button1Mask) {
                     ski_modifier_reg |= 0x80;
                 } else {
                     ski_modifier_reg &= ~0x80;
                 }
                 if (event.xmotion.state & Button3Mask) {
                     ski_modifier_reg |= 0x40;
                 } else {
                     ski_modifier_reg &= ~0x40;
                 }
#endif
}

static void handleMouseBtnDown(SDL_MouseButtonEvent *event)
{
    if (!mouse_grabbed) {
        return;
    }

    if (ski_status_reg & 0x80) { /* Mouse reg still full */
        return;
    }

    switch (event->button) {
        case SDL_BUTTON_LEFT:
            ski_button0 = 1;
            break;
        case SDL_BUTTON_RIGHT:
            ski_button1 = 1;
            break;
        default:
            break;
    }

    ski_xdelta = 0;
    ski_ydelta = 0;

    if (ski_status_reg & 0x40) {
        m65816_addIRQ();
    }

    ski_status_reg |= 0x80;
    ski_status_reg &= ~0x02;
}

static void handleMouseBtnUp(SDL_MouseButtonEvent *event)
{
    if (!mouse_grabbed) {
        return;
    }

    if (ski_status_reg & 0x80) { /* Mouse reg still full */
        return;
    }

    switch (event->button) {
        case SDL_BUTTON_LEFT:
            ski_button0 = 0;
            break;
        case SDL_BUTTON_RIGHT:
            ski_button1 = 0;
            break;
        default:
            break;
    }

    ski_xdelta = 0;
    ski_ydelta = 0;

    if (ski_status_reg & 0x40) {
        m65816_addIRQ();
    }

    ski_status_reg |= 0x80;
    ski_status_reg &= ~0x02;
}

static void handleMouseMotion(SDL_MouseMotionEvent *event)
{
    if (!mouse_grabbed) {
        return;
    }

    if (ski_status_reg & 0x80) { /* Mouse reg still full */
        return;
    }

    ski_xdelta = event->xrel;
    ski_ydelta = event->yrel;

    ski_button0 = (event->state & SDL_BUTTON_LMASK)? 1 : 0;
    ski_button1 = (event->state & SDL_BUTTON_RMASK)? 1 : 0;

    if (ski_status_reg & 0x40) {
        m65816_addIRQ();
    }

    ski_status_reg |= 0x80;
    ski_status_reg &= ~0x02;
}

static void handleWindow(SDL_WindowEvent *event)
{
    switch (event->event) {
        case SDL_WINDOWEVENT_CLOSE:
            globalShutdown();
            break;
/*
        case SDL_WINDOWEVENT_ENTER:
            SDL_ShowCursor(SDL_DISABLE);
            break;
        case SDL_WINDOWEVENT_LEAVE:
            SDL_ShowCursor(SDL_ENABLE);
            break;
*/
    }
}
