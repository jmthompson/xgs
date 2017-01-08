#ifndef DOC_H_
#define DOC_H_

#include <cstdlib>
#include <string>
#include <vector>

#include "gstypes.h"
#include "Device.h"

using std::uint8_t;
using std::uint16_t;
using std::uint32_t;

struct AudioSample {
    float left;
    float right;
};

class DOC : public Device {
    public:
        static const unsigned int kNumOscillators = 32;

    private:
        static const unsigned int kInterruptStackSize = 256;
        static const unsigned int kSampleRate         = 26320;
        static const unsigned int kAudioBufferSize    = 4096;

        std::string       sound_device;
        SDL_AudioDeviceID sound_device_id;

        unsigned int glu_ctrl_reg;
        unsigned int glu_next_val;
        uint16_t     glu_addr_reg;

        unsigned int num_osc;

        float system_volume;

        uint8_t doc_registers[256];
        uint8_t doc_ram[65536];

        uint16_t     last_addr[kNumOscillators];
        uint32_t     osc_acc[kNumOscillators];
        bool         osc_enable[kNumOscillators];
        unsigned int osc_chan[kNumOscillators];
        unsigned int osc_freq[kNumOscillators];
        float        osc_vol[kNumOscillators];
        unsigned int osc_wp[kNumOscillators];
        unsigned int osc_int[kNumOscillators];
        unsigned int osc_ws[kNumOscillators];
        unsigned int osc_res[kNumOscillators];
        unsigned int osc_mode[kNumOscillators];
        unsigned int osc_shift[kNumOscillators];
        unsigned int osc_accmask[kNumOscillators];
        uint8_t *    osc_addrbase[kNumOscillators];

        unsigned int irq_stack[kInterruptStackSize];
        unsigned int irq_index;

        AudioSample *sample_buffer;
        AudioSample last_sample;

        float click_sample;

        unsigned int buffer_index;
        unsigned int buffer_max;
        unsigned int buffer_len;

        std::vector<unsigned int>& ioReadList()
        {
            static std::vector<unsigned int> locs = { 0x30, 0x3C, 0x3D, 0x3E, 0x3F };

            return locs;
        }

        std::vector<unsigned int>& ioWriteList()
        {
            static std::vector<unsigned int> locs = { 0x30, 0x3C, 0x3D, 0x3E, 0x3F };

            return locs;
        }

        void enableOscillators(void);
        void scanOscillators(AudioSample *);
        void updateOscillator(const unsigned int);

        void pushIRQ(const unsigned int);
        const int pullIRQ();

    public:
        DOC() = default;
        ~DOC();

        void reset();
        uint8_t read(const unsigned int& offset);
        void write(const unsigned int& offset, const uint8_t& value);

        void microtick(const unsigned int);

        //void clickSpeaker();
        void setOutputDevice(string&);

        void bufferCallback(Uint8 *, int);
};

#endif // DOC_H_
