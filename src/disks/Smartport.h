#ifndef SMARTPORT_H_

#include <cstdlib>
#include <iostream>
#include <vector>

#include "xgscore/Device.h"

using std::uint8_t;

struct SmartportCommand {
    int command     = -1;
    int read_bytes  = -1;
    int write_bytes = -1;
};

class VirtualDisk;

class Smartport : public Device {
    private:
        VirtualDisk *units[kSmartportUnits];

        std::vector<unsigned int>& ioReadList()
        {
            static std::vector<unsigned int> locs;

            return locs;
        }

        std::vector<unsigned int>& ioWriteList()
        {
            static std::vector<unsigned int> locs;

            return locs;
        }

        void prodosEntry();
        void smartportEntry();

        void statusCmd(const uint8_t, const uint16_t);
        void readBlockCmd(const uint8_t, const uint16_t);
        void writeBlockCmd(const uint8_t, const uint16_t);
        void formatCmd(const uint8_t, const uint16_t);
        void controlCmd(const uint8_t, const uint16_t);
        void initCmd(const uint8_t, const uint16_t);
        void openCmd(const uint8_t, const uint16_t);
        void closeCmd(const uint8_t, const uint16_t);
        void readCmd(const uint8_t, const uint16_t);
        void writeCmd(const uint8_t, const uint16_t);
        void statusCmdExt(const uint8_t, const uint16_t);
        void readBlockCmdExt(const uint8_t, const uint16_t);
        void writeBlockCmdExt(const uint8_t, const uint16_t);
        void formatCmdExt(const uint8_t, const uint16_t);
        void controlCmdExt(const uint8_t, const uint16_t);
        void initCmdExt(const uint8_t, const uint16_t);
        void openCmdExt(const uint8_t, const uint16_t);
        void closeCmdExt(const uint8_t, const uint16_t);
        void readCmdExt(const uint8_t, const uint16_t);
        void writeCmdExt(const uint8_t, const uint16_t);

    public:
        Smartport();
        ~Smartport();

        void reset();
        uint8_t read(const unsigned int& offset) { return 0; }
        void write(const unsigned int& offset, const uint8_t& value) {}

        void wdm(const uint8_t);

        void attach(System *theSystem);

        void tick(const unsigned int) {}
        void microtick(const unsigned int) {}

        void mountImage(const unsigned int, VirtualDisk *image);
        void unmountImage(const unsigned int);
};

#endif // SMARTPORT_H_
