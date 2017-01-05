inline void stackPush(const uint8_t& v)
{
    system->cpuWrite(0, S-- + StackOffset, v);
}

inline void stackPush(const uint16_t& v)
{
    system->cpuWrite(0, S-- + StackOffset, v >> 8);
    system->cpuWrite(0, S-- + StackOffset, v);
}

inline void stackPull(uint8_t& v)
{
    v = system->cpuRead(0, ++S + StackOffset);
}

inline void stackPull(uint16_t& v)
{
    v = system->cpuRead(0, ++S + StackOffset) | (system->cpuRead(0, ++S + StackOffset) << 8);
}

inline void jumpTo(const uint16_t& addr)
{
    PC = addr;
}

inline void jumpTo(const uint8_t& bank, const uint16_t& addr)
{
    PBR = bank;
    PC  = addr;
}

inline void checkDataPageCross(const uint16_t& check)
{
    if ((StackOffset > 0) && ((check & 0xFF00) != (operand_addr & 0xFF00))) {
        cpu->num_cycles++;
    }
}

inline void checkProgramPageCross()
{
    if ((StackOffset > 0) && ((PC & 0xFF00) != (operand_addr & 0xFF00))) {
        cpu->num_cycles++;
    }
}

inline void checkDirectPageAlignment()
{
    if (D & 0xFF) {
        cpu->num_cycles++;
    }
}

inline void fetchImmediateOperand(uint8_t &op)
{
    op = system->cpuRead(PBR, PC++);
}

inline void fetchImmediateOperand(uint16_t &op)
{
    op = system->cpuRead(PBR, PC++) | (system->cpuRead(PBR, PC++) << 8);
}

inline void fetchOperand(uint8_t &op)
{
    op = system->cpuRead(operand_bank, operand_addr);
}

inline void fetchOperand(uint16_t &op)
{
    op = system->cpuRead(operand_bank, operand_addr) | (system->cpuRead(operand_bank, operand_addr + 1) << 8);
}

inline void storeOperand(uint8_t &op)
{
    system->cpuWrite(operand_bank, operand_addr, op);
}

inline void storeOperand(uint16_t &op)
{
    system->cpuWrite(operand_bank, operand_addr, op);
    system->cpuWrite(operand_bank, operand_addr + 1, op >> 8);
}

inline void checkIfNegative(const uint8_t &v) { SR.N = v & 0x80; }
inline void checkIfNegative(const uint16_t &v) { SR.N = v & 0x8000; }

inline void checkIfZero(const uint8_t &v) { SR.Z = (v == 0); }
inline void checkIfZero(const uint16_t &v) { SR.Z = (v == 0); }

inline void op_AND()
{
    A &= operand.m;

    checkIfNegative(A);
    checkIfZero(A);
}

inline void op_BIT()
{
    SR.Z = !(operand.m & A);
    SR.N = operand.m & n_bit;
    SR.V = operand.m & v_bit;
}

inline void op_EOR()
{
    A ^= operand.m;

    checkIfNegative(A);
    checkIfZero(A);
}

inline void op_ORA()
{
    A |= operand.m;

    checkIfNegative(A);
    checkIfZero(A);
}

inline void op_TRB()
{
    SR.Z = !(operand.m & A);

    operand.m &= ~A;
}

inline void op_TSB()
{
    SR.Z = !(operand.m & A);

    operand.m |= A;
}

// Increment/decrement

inline void op_DEC()
{
    --operand.m;

    checkIfNegative(operand.m);
    checkIfZero(operand.m);
}

inline void op_INC()
{
    ++operand.m;

    checkIfNegative(operand.m);
    checkIfZero(operand.m);
}

// Bit shifts/rotates

inline void op_ASL()
{
    SR.C = operand.m & n_bit;

    operand.m <<= 1;

    checkIfNegative(operand.m);
    checkIfZero(operand.m);
}

inline void op_LSR()
{
    SR.C = operand.m & 1;

    operand.m >>= 1;

    checkIfNegative(operand.m);
    checkIfZero(operand.m);
}

inline void op_ROL()
{
    bool c = SR.C;

    SR.C = operand.m & n_bit;

    operand.m = (operand.m << 1) | c;

    checkIfNegative(operand.m);
    checkIfZero(operand.m);
}

inline void op_ROR()
{
    bool c = SR.C;

    SR.C = operand.m & 1;
    operand.m >>= 1;
    if (c) operand.m |= n_bit;

    checkIfNegative(operand.m);
    checkIfZero(operand.m);
}

// Compares

inline void op_CMP()
{
    MemSizeType tmp = A - operand.m;

    checkIfNegative(tmp);
    checkIfZero(tmp);

    SR.C = (A >= operand.m);
}
    
inline void op_CPX()
{
    IndexSizeType tmp = X - operand.x;

    checkIfNegative(tmp);
    checkIfZero(tmp);

    SR.C = (X >= operand.x);
}
    
inline void op_CPY()
{
    IndexSizeType tmp = Y - operand.x;

    checkIfNegative(tmp);
    checkIfZero(tmp);

    SR.C = (Y >= operand.x);
}

// Block moves

void op_MVP()
{
    uint8_t dst_bank = system->cpuRead(PBR, PC);
    uint8_t src_bank = system->cpuRead(PBR, PC + 1);

    if (cpu->A.W != 0xFFFF) {
        system->cpuWrite(dst_bank, cpu->Y.W, system->cpuRead(src_bank, cpu->X.W));

        --cpu->A.W;
        --cpu->X.W;
        --cpu->Y.W;

        --PC;
    }
    else {
        PC += 2;
    }
}

void op_MVN()
{
    uint8_t dst_bank = system->cpuRead(PBR, PC);
    uint8_t src_bank = system->cpuRead(PBR, PC + 1);

    if (cpu->A.W != 0xFFFF) {
        system->cpuWrite(dst_bank, cpu->Y.W, system->cpuRead(src_bank, cpu->X.W));

        --cpu->A.W;
        ++cpu->X.W;
        ++cpu->Y.W;

        --PC;
    }
    else {
        PC += 2;
    }
}

// Loads and stores

inline void op_LDA()
{
    A = operand.m;

    checkIfNegative(A);
    checkIfZero(A);
}

inline void op_LDX()
{
    X = operand.x;

    checkIfNegative(X);
    checkIfZero(X);
}

inline void op_LDY()
{
    Y = operand.x;

    checkIfNegative(Y);
    checkIfZero(Y);
}

inline void op_STA()
{
    system->cpuWrite(operand_bank, operand_addr, A);

    if (sizeof(MemSizeType) == 2) {
        system->cpuWrite(operand_bank, operand_addr + 1, A >> 8);
    }
}

inline void op_STX()
{
    system->cpuWrite(operand_bank, operand_addr, X);

    if (sizeof(IndexSizeType) == 2) {
        system->cpuWrite(operand_bank, operand_addr + 1, X >> 8);
    }
}

inline void op_STY()
{
    system->cpuWrite(operand_bank, operand_addr, Y);

    if (sizeof(IndexSizeType) == 2) {
        system->cpuWrite(operand_bank, operand_addr + 1, Y >> 8);
    }
}

inline void op_STZ()
{
    system->cpuWrite(operand_bank, operand_addr, 0);

    if (sizeof(MemSizeType) == 2) {
        system->cpuWrite(operand_bank, operand_addr + 1, 0);
    }
}

// Register transfers

inline void op_TCD()
{
    D = cpu->A.W;

    checkIfNegative(D);
    checkIfZero(D);
}

inline void op_TDC()
{
    cpu->A.W = D;

    checkIfNegative(cpu->A.W);
    checkIfZero(cpu->A.W);
}

// Addition and subtraction

inline void op_ADC()
{
    unsigned int sum = 0;

    if (SR.D) {
        int op1[4], op2[4];
        bool carry = SR.C;

        bcdToNibbles(A, op1);
        bcdToNibbles(operand.m, op2);

        for (unsigned int i = 0 ; i < sizeof(MemSizeType) * 2 ; i++) {
            unsigned int digit = op1[i] + op2[i] + carry;

            if (digit >= 10) {
                digit -= 10;

                carry = true;
            }
            else {
                carry = false;
            }

            sum = (sum << 4) | (digit & 0x0F);
        }

        SR.C = carry;
    }
    else {
        sum = A + operand.m + SR.C;

        SR.C = (sum > m_max);
    }

    SR.V = (sum ^ operand.m) & (sum ^ A) & n_bit;

    A = sum;

    checkIfNegative(A);
    checkIfZero(A);
}

inline void op_SBC()
{
    unsigned int diff = 0;
    bool borrow = !SR.C;

    if (SR.D) {
        int op1[4], op2[4];

        bcdToNibbles(A, op1);
        bcdToNibbles(operand.m, op2);

        for (unsigned int i = 0 ; i < sizeof(MemSizeType) * 2 ; i++) {
            int digit = op2[i] - op1[i] - borrow;

            if (digit < 0) {
                digit += 10;

                borrow = true;
            }
            else {
                borrow = false;
            }

            diff = (diff << 4) | (digit & 0x0F);
        }

        SR.C = borrow;
    }
    else {
        diff = A - operand.m - borrow;

        SR.C = !(diff > m_max);
    }

    SR.V = (diff ^ (255 - operand.m)) & (diff ^ A) & n_bit;

    A = diff;

    checkIfNegative(A);
    checkIfZero(A);
}

inline void bcdToNibbles(MemSizeType in, int *out)
{
    for (unsigned int i = 0 ; i < sizeof(MemSizeType) * 2; i++) {
        out[i] = in & 0x0F;

        in >>= 4;
    }
}
