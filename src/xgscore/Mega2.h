#ifndef MEGA2_H_
#define MEGA2_H_

#include <vector>
#include "gstypes.h"

#include "Device.h"

class ADB;
class VGC;

class Mega2 : public Device {
    friend class VGC;

    private:
        VGC *vgc;
        ADB *adb;

        unsigned int last_access;

        bool sw_80store;
        bool sw_auxrd;
        bool sw_auxwr;
        bool sw_altzp;

        bool sw_lcbank2;
        bool sw_lcread;
        bool sw_lcwrite;
        bool sw_lcsecond;

        bool sw_shadow_text;
        bool sw_shadow_text2;
        bool sw_shadow_hires1;
        bool sw_shadow_hires2;
        bool sw_shadow_super;
        bool sw_shadow_aux;
        bool sw_shadow_lc;

        bool sw_slot_reg[8];

        bool sw_intcxrom;
        bool sw_slotc3rom;
        bool sw_rombank;

        uint8_t sw_diagtype;

        bool sw_fastmode;
        bool sw_slot7_motor;
        bool sw_slot6_motor;
        bool sw_slot5_motor;
        bool sw_slot4_motor;

        void updateMemoryMaps();
        void buildLanguageCard(unsigned int bank, unsigned int src_bank);

        std::vector<unsigned int>& ioReadList()
        {
            static std::vector<unsigned int> locs = {
                0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                0x09, 0x0A, 0x0B, 0x11, 0x12, 0x13, 0x14, 0x15,
                0x16, 0x17, 0x18, 0x2D, 0x35, 0x36, 0x41, 0x46,
                0x68, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77,
                0x78, 0x79, 0x7A, 0x7B, 0x7C, 0x7D, 0x7E, 0x7F,
                0x80, 0x81, 0x82, 0x83, 0x88, 0x89, 0x8A, 0x8B
            };

            return locs;
        }

        std::vector<unsigned int>& ioWriteList()
        {
            static std::vector<unsigned int> locs = {
                0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                0x08, 0x09, 0x0A, 0x0B, 0x2D, 0x35, 0x36, 0x41,
                0x47, 0x68, 0x80, 0x81, 0x82, 0x83, 0x88, 0x89,
                0x8A, 0x8B
            };

            return locs;
        }

    public:
        Mega2() = default;
        ~Mega2() = default;

        void reset();
        uint8_t read(const unsigned int& offset);
        void write(const unsigned int& offset, const uint8_t& value);
};

#endif // MEGA2_H_
