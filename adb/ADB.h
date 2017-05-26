#ifndef ADB_H_

#include <cstdlib>
#include <iostream>
#include <vector>

#include <SDL.h>

#include "emulator/Device.h"

using std::uint8_t;

/**
 * Structure for representing a SKI command in progress. The command will
 * begin execution after we have read read_bytes, and ends after write_bytes
 * are written.
 */

struct SKICommand {
    int command     = -1;
    int read_bytes  = -1;
    int write_bytes = -1;
};

class Mega2;

class ADB : public Device {
    friend class Mega2;

    private:
        // This is the version returned on a real ROM 03
        static const unsigned int kADBVersion = 6;

        // Size of the SKI buffer
        static const unsigned int kInputBufferSize = 128;

        bool mouse_grabbed;

        bool sw_m2mouseenable;
        bool sw_m2mousemvirq;
        bool sw_m2mouseswirq;

        unsigned int paddle0, paddle1;
        cycles_t paddle0_time, paddle1_time;

        uint8_t ski_kbd_reg;
        uint8_t ski_modifier_reg = 0;
        uint8_t ski_data_reg     = 0;
        uint8_t ski_status_reg   = 0x10;
        uint8_t ski_mode_byte    = 0x10;
        bool    ski_status_irq = false;
        uint8_t ski_conf[3];
        uint8_t ski_error;

        uint8_t ski_ram[96];

        uint8_t ski_data[16];

        bool ski_button0;
        bool ski_button1;

        int     ski_xdelta;
        int     ski_ydelta;

        unsigned int ski_input_index;
        unsigned int ski_output_index;

        uint8_t    ski_input_buffer[kInputBufferSize];

        unsigned int ski_read,ski_written;

        SKICommand current;

        std::vector<unsigned int>& ioReadList()
        {
            static std::vector<unsigned int> locs = {
                0x00, 0x10, 0x24, 0x25, 0x26, 0x27, 0x44, 0x45,
                0x61, 0x62, 0x64, 0x65, 0x66, 0x67, 0x70
            };

            return locs;
        }

        std::vector<unsigned int>& ioWriteList()
        {
            static std::vector<unsigned int> locs = {
                0x10, 0x26, 0x27, 0x70
            }; 

            return locs;
        }

        uint8_t readKeyboard();
        uint8_t readMouse();
        uint8_t readModifiers();
        uint8_t readCommand();
        uint8_t readCommandKey();
        uint8_t readOptionKey();
        uint8_t readStatus();
        uint8_t readM2MouseX();
        uint8_t readM2MouseY();
        uint8_t readPaddle0();
        uint8_t readPaddle1();
        uint8_t readPaddle2();
        uint8_t readPaddle3();

        void clearLastKey();
        void setCommand(uint8_t);
        void setStatus(uint8_t);
        void triggerPaddles();

        void handleKeyDown(SDL_KeyboardEvent *);
        void handleKeyUp(SDL_KeyboardEvent *);
        void handleJoyButton(SDL_JoyButtonEvent *);
        void handleJoyMotion(SDL_JoyAxisEvent *);
        void handleMouseBtnDown(SDL_MouseButtonEvent *);
        void handleMouseBtnUp(SDL_MouseButtonEvent *);
        void handleMouseMotion(SDL_MouseMotionEvent *);

    public:
        ADB() = default;
        ~ADB() = default;

        void reset();
        uint8_t read(const unsigned int& offset);
        void write(const unsigned int& offset, const uint8_t& value);

        void tick(const unsigned int);
        void microtick(const unsigned int);

        bool processEvent(SDL_Event&);
};

#endif // ADB_H_
