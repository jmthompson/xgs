#ifndef SYSTEMBOARD_H_
#define SYSTEMBOARD_H_

class SystemBoard {
    public:
        SystemBoard() {};
        ~SystemBoard() {};

        virtual byte readMemory(word32 address);
        virtual void writeMemory(word32 address, byte value);
        virtual void wdmCallback(byte operand);
};

#endif // SYSTEMBOARD_H_
