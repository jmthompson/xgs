/**
 * XGS: The Linux GS Emulator
 * Written and Copyright (C) 1996 - 2025 by Joshua M. Thompson
 *
 * You are free to distribute this code for non-commercial purposes
 * I ask only that you notify me of any changes you make to the code
 * Commercial use is prohibited without my written permission
 */

/*
 * This code implements the ADB GLU and the SKI
 */

#include <cstdlib>

#include <SDL.h>

#include "adb.h"
#include "interrupts.h"
#include "m65816.h"
#include "memory.h"
#include "soft_switches.h"

using std::uint8_t;

namespace Adb
{

static unsigned int paddle0, paddle1;
static m65816::cycles_t paddle0_time, paddle1_time;

static uint8_t ski_kbd_reg;
static uint8_t ski_modifier_reg = 0;
static uint8_t ski_data_reg = 0;
static uint8_t ski_status_reg = 0x10;
static uint8_t ski_mode_byte = 0x10;
static uint8_t ski_status_irq = 0;
static uint8_t ski_conf[3];
static uint8_t ski_error;

static uint8_t ski_ram[96];

static uint8_t ski_data[16];

static bool ski_button0;
static bool ski_button1;

static int ski_xdelta;
static int ski_ydelta;

static unsigned int ski_input_index;
static unsigned int ski_output_index;

static uint8_t ski_input_buffer[kInputBufferSize];

static unsigned int ski_read, ski_written;

SKICommand current;

static SKICommand command_list[] = {
    {0x00, 0, 1}, // Abort current command
    {0x01, 0, 0}, // Abort current command
    {0x02, 0, 0}, // Reset SKI
    {0x03, 0, 0}, // Flush type-ahead buffer
    {0x04, 1, 0}, // Set mode bits
    {0x05, 1, 0}, // Clear mode bits
    {0x06, 3, 0}, // Set configuration
    {0x07, 4, 0}, // Sync (clear modes + set configuration)
    {0x08, 2, 0}, // Write byte to address
    {0x09, 2, 1}, // Read byte from address
    {0x0A, 0, 1}, // Read mode byte
    {0x0B, 0, 3}, // Read configuration
    {0x0C, 0, 1}, // Read+clear error byte
    {0x0D, 0, 1}, // Return version number
    {0x0E, 0, 2}, // Read character sets available
    {0x0F, 0, 2}, // Read layouts available
    {0x10, 0, 0}, // Reset system
    {0x11, 0, 1}, // Send ADB keycode
    {0x40, 0, 0}, // Reset ADB
    {0x48, 1, 0}, // Send command
    {0x73, 0, 0}, // Disable SRQ on device 3 (mouse)
    {0xB3, 2, 0}, // Listen on device 3 (mouse), reg 3
    {-1,   0, 0}  // END OF TABLE
};

static char applyModifiers(char key, uint8_t modifiers)
{
  char high = key & 0x80;

  key &= 0x7F;

  if (modifiers & 0x02) { // control
    if ((key >= 'a') && (key <= 'z')) {
      key -= 95;
    }
  } else if (modifiers & 0x41) { // caps lock, shift
    if ((key >= 'a') && (key <= 'z')) {
      key -= 32;
    }
    switch (key) {
    case '`':
      key = '~';
      break;
    case '1':
      key = '!';
      break;
    case '2':
      key = '@';
      break;
    case '3':
      key = '#';
      break;
    case '4':
      key = '$';
      break;
    case '5':
      key = '%';
      break;
    case '6':
      key = '^';
      break;
    case '7':
      key = '&';
      break;
    case '8':
      key = '*';
      break;
    case '9':
      key = '(';
      break;
    case '0':
      key = ')';
      break;
    case '[':
      key = '{';
      break;
    case ']':
      key = '}';
      break;
    case '-':
      key = '_';
      break;
    case '=':
      key = '+';
      break;
    case ';':
      key = ':';
      break;
    case '\'':
      key = '"';
      break;
    case '\\':
      key = '|';
      break;
    case ',':
      key = '<';
      break;
    case '.':
      key = '>';
      break;
    case '/':
      key = '?';
      break;
    default:
      break;
    }
  }

  return key | high;
}

static uint8_t readKeyboard(const uint8_t offset, const uint8_t _val)
{
  if ((ski_input_index != ski_output_index)) {
    uint8_t key = ski_input_buffer[ski_output_index++];

    if (ski_output_index == kInputBufferSize) ski_output_index = 0;

    if (key & 0x80) {
      ski_kbd_reg = key;
      ski_modifier_reg |= 0x10;
    } else {
      ski_kbd_reg = key | 0x80;
      ski_modifier_reg &= ~0x10;
    }
  }

  return ski_kbd_reg;
}

static uint8_t readMouse(const uint8_t offset, const uint8_t _val)
{
  int delta;
  bool button;

  if (!(ski_status_reg & 0x80)) return 0;

  if (ski_status_reg & 0x02) {
    delta = ski_ydelta;
    button = ski_button0;

    ski_status_reg &= ~0x82;

    if (ski_status_reg & 0x40) lowerInterrupt(ADB_IRQ);
  } else {
    delta = ski_xdelta;
    button = ski_button1;

    ski_status_reg |= 0x02;
  }

  uint8_t val = 0;

  if (!button) val |= 0x80;

  if (delta > 0x3F) {
    delta = 0x3F;
  } else if (delta < 0) {
    val |= 0x40;

    if (delta < -0x40) {
      delta = 0x0;
    } else {
      delta &= 0x3F;
    }
  }

  return val | delta;
}

static uint8_t readModifiers(const uint8_t offset, const uint8_t _val)
{
  return ski_modifier_reg;
}

static uint8_t readCommand(const uint8_t offset, const uint8_t _val)
{
  uint8_t val;

  if (ski_status_irq) {
    val = ski_status_irq;

    ski_status_irq = false;

    lowerInterrupt(ADB_IRQ);

    return val;
  }

  val = ski_data_reg;

  ski_status_reg &= ~0x20;

  if ((current.write_bytes > 0) && (ski_written < current.write_bytes)) {
    ski_data_reg = ski_data[ski_written++];

    ski_status_reg |= 0x20;

    if (current.write_bytes == ski_written) {
      current.command = -1;
      current.read_bytes = -1;
      current.write_bytes = -1;
    }
  }

  return val;
}

static uint8_t readCommandKey(const uint8_t offset, const uint8_t _val)
{
  return ski_modifier_reg & 0x80;
}

static uint8_t readOptionKey(const uint8_t offset, const uint8_t _val)
{
  return (ski_modifier_reg & 0x40) ? 0x80 : 0x00;
}

static uint8_t readStatus(const uint8_t offset, const uint8_t _val)
{
  if (ski_status_irq) {
    return ski_status_reg | 0x20;
  } else {
    return ski_status_reg;
  }
}

static uint8_t readMouseX(const uint8_t offset, const uint8_t _val)
{
  // printf("Access to Mega II Mouse Delta X register!\n");
  return 0;
}

static uint8_t readMouseY(const uint8_t offset, const uint8_t _val)
{
  // printf("Access to Mega II Mouse Delta Y register!\n");
  return 0;
}

static uint8_t readPaddle0(const uint8_t offset, const uint8_t _val)
{
  if (m65816::total_cycles < paddle0_time) {
    return 0x80;
  } else {
    return 0x00;
  }
  return 0x80;
}

static uint8_t readPaddle1(const uint8_t offset, const uint8_t _val)
{
  if (m65816::total_cycles < paddle1_time) {
    return 0x80;
  } else {
    return 0x00;
  }
  return 0x80;
}

static uint8_t readPaddle2(const uint8_t offset, const uint8_t _val)
{
  return 0x80;
}

static uint8_t readPaddle3(const uint8_t offset, const uint8_t _val)
{
  return 0x80;
}

static uint8_t clearLastKey(const uint8_t offset, const uint8_t _val)
{
  ski_kbd_reg &= 0x7F;
  return 0;
}

static uint8_t setCommand(const uint8_t offset, const uint8_t val)
{
  unsigned int i;

  if (current.read_bytes < 0) {
    i = 0;

    while (command_list[i].command != -1) {
      if (command_list[i].command == val) break;

      ++i;
    }

    if (command_list[i].command == -1) {
      /* printf("Unknown ADB command : %d\n",(int) val); */
      return 0;
    }

    current = command_list[i];

    ski_read = 0;
    ski_written = 0;
  } else {
    ski_data[ski_read++] = val;
  }

  if (current.read_bytes == ski_read) {
    switch (current.command) {
    case 0x00: // ??? (behavior hacked from ROM)
      ski_data[0] = 0xA5;
      break;
    case 0x01: // Abort current command
      current.command = -1;
      current.read_bytes = -1;
      current.write_bytes = -1;

      ski_status_reg &= ~0x20;

      break;
    case 0x02: // Reset SKI
      reset();

      break;
    case 0x03: // Flush type-ahead buffer
      break;
    case 0x04: // Set mode bits
      ski_mode_byte |= ski_data[0];

      break;
    case 0x05: // Clear mode bits
      ski_mode_byte &= ~ski_data[0];

      break;
    case 0x06: // Set configuration
      ski_conf[0] = ski_data[0];
      ski_conf[1] = ski_data[1];
      ski_conf[2] = ski_data[2];

      break;
    case 0x07: // Sync
      ski_mode_byte &= ~ski_data[0];

      ski_conf[0] = ski_data[1];
      ski_conf[1] = ski_data[2];
      ski_conf[2] = ski_data[3];

      break;
    case 0x08: // Write SKI RAM
      i = ski_data[0];
      if (i > 0x5F) i = 0x5F;

      ski_ram[i] = ski_data[1];

      break;
    case 0x09: // Read SKI RAM/ROM
      i = ski_data[0] | (ski_data[1] << 8);
      if (i > 0x5F) i = 0;

      ski_data[0] = ski_ram[i];

      break;
    case 0x0A: // Read mode byte
      ski_data[0] = ski_mode_byte;

      break;
    case 0x0B: // Read configuration
      ski_data[0] = ski_conf[0];
      ski_data[1] = ski_conf[1];
      ski_data[2] = ski_conf[2];

      break;
    case 0x0C: // return & reset error codes
      ski_data[0] = ski_error;
      ski_error = 0;

      break;
    case 0x0D: // Return version nibble
      ski_data[0] = kADBVersion;

      break;
    case 0x0E: // Read available char sets
      ski_data[0] = 1;
      ski_data[1] = 0;

      break;
    case 0x0F: // Read available layouts
      ski_data[0] = 1;
      ski_data[1] = 0;

      break;
    case 0x10: // Reset system
      // hardwareReset();
      return 0;
    case 0x11: // Send ADB keycode
      break;
    case 0x40: // Reset ADB
      reset();

      break;
    case 0x48: // Send command
      break;

    default:
      /* printf("Unimplemented ADB command : %d\n",(int) current.command); */
      break;
    }

    if (current.write_bytes > 0) {
      ski_status_reg |= 0x20;
      ski_data_reg = ski_data[0];
      ski_written = 1;

      if (current.write_bytes == ski_written) {
        current.command = -1;
        current.read_bytes = -1;
        current.write_bytes = -1;
      }
    } else {
      current.command = -1;
      current.read_bytes = -1;
      current.write_bytes = -1;
    }
  }

  return 0;
}

static uint8_t setStatus(const uint8_t offset, const uint8_t val)
{
  if (val & 0x40) {
    ski_status_reg |= 0x40;
  } else {
    ski_status_reg &= ~0x40;
  }

  return 0;
}

static uint8_t triggerPaddles(const uint8_t offset, const uint8_t _val)
{
  paddle0_time = m65816::total_cycles + (paddle0 * 11);
  paddle1_time = m65816::total_cycles + (paddle1 * 11);

  return 0;
}

static void handleKeyDown(SDL_KeyboardEvent *event)
{
  SDL_Keycode sym = event->keysym.sym;
  char key = 0x00;

  switch (sym) {
  case SDLK_CAPSLOCK:
    ski_modifier_reg ^= 0x04;
    break;
  case SDLK_LSHIFT:
  case SDLK_RSHIFT:
    ski_modifier_reg |= 0x01;
    break;
  case SDLK_LCTRL:
    ski_modifier_reg |= 0x02;
    break;
  case SDLK_LALT:
    ski_modifier_reg |= 0x80;
    break;
  case SDLK_RALT:
    ski_modifier_reg |= 0x40;
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
    key = 0x1B;

    if ((ski_modifier_reg & 0x82) == 0x82) {
      if (!ski_status_irq) {
        ski_status_irq = 0x20;

        raiseInterrupt(ADB_IRQ);
      }
    }

    break;
  default:
    if (!(sym & 0x40000000)) {
      key = (char)sym;
    }

    break;
  }

  if (key) {
    ski_input_buffer[ski_input_index++] = applyModifiers(key, ski_modifier_reg);

    if (ski_input_index == kInputBufferSize) ski_input_index = 0;
  }
}

static void handleKeyUp(SDL_KeyboardEvent *event)
{
  SDL_Keycode sym = event->keysym.sym;

  switch (sym) {
  case SDLK_LSHIFT:
  case SDLK_RSHIFT:
    ski_modifier_reg &= ~0x01;
    break;
  case SDLK_LCTRL:
    ski_modifier_reg &= ~0x02;
    break;
  case SDLK_LALT:
    ski_modifier_reg &= ~0x80;
    break;
  case SDLK_RALT:
    ski_modifier_reg &= ~0x40;
    break;
  default:
    break;
  }
}

static void handleJoyButton(SDL_JoyButtonEvent *event)
{
#if 0
  // down, up
  if (adb_grab_mode == 1) { /* Joystick */
    switch (event.xbutton.button) {
    case Button1:
      ski_modifier_reg |= 0x80;
      break;
    case Button3:
      ski_modifier_reg |= 0x40;
      break;
    default:
      break;
    }

    switch (event.xbutton.button) {
    case Button1:
      ski_modifier_reg &= ~0x80;
      break;
    case Button3:
      ski_modifier_reg &= ~0x40;
      break;
    default:
      break;
    }
  }
#endif
}

  static void handleJoyMotion(SDL_JoyAxisEvent * event)
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

  static void handleMouseBtnDown(SDL_MouseButtonEvent * event)
  {
    if (ski_status_reg & 0x80) return; // Mouse reg still full

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
      raiseInterrupt(ADB_IRQ);
    }

    ski_status_reg |= 0x80;
    ski_status_reg &= ~0x02;
  }

  static void handleMouseBtnUp(SDL_MouseButtonEvent * event)
  {
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
      raiseInterrupt(ADB_IRQ);
    }

    ski_status_reg |= 0x80;
    ski_status_reg &= ~0x02;
  }

  static void handleMouseMotion(SDL_MouseMotionEvent * event)
  {
    if (ski_status_reg & 0x80) { /* Mouse reg still full */
      return;
    }

    ski_xdelta = event->xrel;
    ski_ydelta = event->yrel;

    ski_button0 = (event->state & SDL_BUTTON_LMASK) ? 1 : 0;
    ski_button1 = (event->state & SDL_BUTTON_RMASK) ? 1 : 0;

    if (ski_status_reg & 0x40) {
      raiseInterrupt(ADB_IRQ);
    }

    ski_status_reg |= 0x80;
    ski_status_reg &= ~0x02;
  }

  void start(void)
  {
    setIoReadHandler(0x00, readKeyboard);
    setIoReadHandler(0x10, clearLastKey);
    setIoReadHandler(0x24, readMouse);
    setIoReadHandler(0x25, readModifiers);
    setIoReadHandler(0x26, readCommand);
    setIoReadHandler(0x27, readStatus);
    setIoReadHandler(0x44, readMouseX);
    setIoReadHandler(0x45, readMouseY);
    setIoReadHandler(0x61, readCommandKey);
    setIoReadHandler(0x62, readOptionKey);
    setIoReadHandler(0x64, readPaddle0);
    setIoReadHandler(0x65, readPaddle1);
    setIoReadHandler(0x66, readPaddle2);
    setIoReadHandler(0x67, readPaddle3);
    setIoReadHandler(0x70, triggerPaddles);

    setIoWriteHandler(0x10, clearLastKey);
    setIoWriteHandler(0x26, setCommand);
    setIoWriteHandler(0x27, setStatus);
    setIoWriteHandler(0x70, triggerPaddles);
  }

  void stop(void) {}

  void reset()
  {
    ski_input_index = 0;
    ski_output_index = 0;
    ski_error = 0;

    current.command = -1;
    current.read_bytes = -1;
    current.write_bytes = -1;

    ski_status_reg &= ~0xC1;

    sw_m2mouseenable = false;
    sw_m2mousemvirq = false;
    sw_m2mouseswirq = false;

    ski_button0 = false;
    ski_button1 = false;
  }

  void tick(unsigned int) {}

  void microtick(unsigned int) {}

  bool processEvent(SDL_Event & event)
  {
    switch (event.type) {
    case SDL_KEYDOWN:
      handleKeyDown(&event.key);

      return true;
    case SDL_KEYUP:
      handleKeyUp(&event.key);

      return true;
    case SDL_JOYBUTTONDOWN:
    case SDL_JOYBUTTONUP:
      handleJoyButton(&event.jbutton);

      return true;
    case SDL_JOYAXISMOTION:
      handleJoyMotion(&event.jaxis);

      return true;
    case SDL_MOUSEBUTTONDOWN:
      handleMouseBtnDown(&event.button);

      return true;
    case SDL_MOUSEBUTTONUP:
      handleMouseBtnUp(&event.button);

      return true;
    case SDL_MOUSEMOTION:
      handleMouseMotion(&event.motion);

      return true;
    }

    return false;
  }

} // namespace Adb