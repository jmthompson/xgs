/**
 * If the CPU is in emulation mode, and the low byte of D is 0x00,
 * then wrap the address at the DP boundary.
 */
uint16_t wrapDirectPage(uint16_t address)
{
    if (!(D & 0xFF) && StackOffset) {
        return (D & 0xFF00) | (address & 0xFF);
    }
    else {
        return address;
    }
}

void getAddress_a()
{
    operand_addr = system->cpuRead(PBR, PC++, INSTR) | (system->cpuRead(PBR, PC++, INSTR) << 8);
    operand_bank = DBR;
}

void getAddress_al()
{
    operand_addr = system->cpuRead(PBR, PC++, INSTR) | (system->cpuRead(PBR, PC++, INSTR) << 8);
    operand_bank = system->cpuRead(PBR, PC++, INSTR);
}

void getAddress_d()
{
    checkDirectPageAlignment();

    operand_addr = wrapDirectPage(D + system->cpuRead(PBR, PC++, INSTR));
    operand_bank = 0;
}

void getAddress_dix()
{
    checkDirectPageAlignment();

    uint16_t tmp = D + system->cpuRead(PBR, PC++, INSTR);
    uint8_t lo = system->cpuRead(0, wrapDirectPage(tmp), OPADDR);
    uint8_t hi = system->cpuRead(0, wrapDirectPage(tmp + 1), OPADDR);

    operand_addr = lo | (hi << 8);
    operand_bank = DBR;

    tmp = operand_addr;
    operand_addr += Y;

    if (operand_addr < tmp) operand_bank++;

    checkDataPageCross(tmp);
}

void getAddress_dixl()
{
    uint16_t tmp = D + system->cpuRead(PBR, PC++, INSTR);

    checkDirectPageAlignment();

    operand_addr = system->cpuRead(0, tmp, OPADDR) | (system->cpuRead(0, tmp + 1, OPADDR) << 8);
    operand_bank = system->cpuRead(0, tmp + 2, OPADDR);

    tmp = operand_addr;
    operand_addr += Y;

    if (operand_addr < tmp) operand_bank++;
}

// (DIRECT,X)
void getAddress_dxi()
{
    uint16_t tmp = D + X + system->cpuRead(PBR, PC++, INSTR);

    checkDirectPageAlignment();

    uint8_t lo = system->cpuRead(0, wrapDirectPage(tmp) , OPADDR);
    uint8_t hi = system->cpuRead(0, wrapDirectPage(tmp + 1), OPADDR);

    operand_addr = lo | (hi << 8);
    operand_bank = DBR;
}

// DIRECT,X
void getAddress_dxx()
{
    checkDirectPageAlignment();

    uint8_t loc = system->cpuRead(PBR, PC++, INSTR);

    operand_addr = wrapDirectPage(D + loc + X);
    operand_bank = 0;
}

// DIRECT,Y
void getAddress_dxy()
{
    checkDirectPageAlignment();

    uint8_t loc = system->cpuRead(PBR, PC++, INSTR);

    operand_addr = wrapDirectPage(D + loc + Y);
    operand_bank = 0;
}

void getAddress_axx()
{
    operand_addr = system->cpuRead(PBR, PC, INSTR) | (system->cpuRead(PBR, PC + 1, INSTR) << 8);
    operand_bank = DBR;

    uint16_t tmp = operand_addr;
    operand_addr += X;

    if (operand_addr < tmp) operand_bank++;

    checkDataPageCross(tmp);

    PC += 2;
}

void getAddress_axy()
{
    operand_addr = system->cpuRead(PBR, PC, INSTR) | (system->cpuRead(PBR, PC + 1, INSTR) << 8);
    operand_bank = DBR;

    uint16_t tmp = operand_addr;

    operand_addr += Y;

    if (operand_addr < tmp) operand_bank++;

    checkDataPageCross(tmp);

    PC += 2;
}

void getAddress_alxx()
{
    uint32_t address = system->cpuRead(PBR, PC, INSTR)
                        | (system->cpuRead(PBR, PC + 1, INSTR) << 8)
                        | (system->cpuRead(PBR, PC + 2, INSTR) << 16);

    address += X;

    operand_addr = address;
    operand_bank = address >> 16;

    PC += 3;
}

void getAddress_pcr()
{
    int8_t offset = system->cpuRead(PBR, PC++, INSTR);

    operand_addr = PC + offset;
    operand_bank = PBR;
}

void getAddress_pcrl()
{
    int16_t offset = system->cpuRead(PBR, PC++, INSTR) | (system->cpuRead(PBR, PC++, INSTR) << 8);

    operand_addr = PC + offset;
    operand_bank = PBR;
}

void getAddress_ai()
{
    uint16_t tmp = system->cpuRead(PBR, PC, INSTR) | (system->cpuRead(PBR, PC + 1, INSTR) << 8);

    PC += 2;

    operand_addr = system->cpuRead(0, tmp, OPADDR) | (system->cpuRead(0, tmp + 1, OPADDR) << 8);
    operand_bank = 0;
}

void getAddress_ail()
{
    uint16_t tmp = system->cpuRead(PBR, PC, INSTR) | (system->cpuRead(PBR, PC + 1, INSTR) << 8);

    PC += 2;

    operand_addr = system->cpuRead(0, tmp, OPADDR) | (system->cpuRead(0, tmp + 1, OPADDR) << 8);
    operand_bank = system->cpuRead(0, tmp + 2, OPADDR);
}

// (DIRECT)
void getAddress_di()
{
    uint16_t tmp = D + system->cpuRead(PBR, PC++, INSTR);

    checkDirectPageAlignment();

    uint8_t lo = system->cpuRead(0, tmp, OPADDR);
    uint8_t hi = system->cpuRead(0, wrapDirectPage(tmp + 1), OPADDR);

    operand_addr = lo | (hi << 8);
    operand_bank = DBR;
}

void getAddress_dil()
{
    uint16_t tmp = D + system->cpuRead(PBR, PC++, INSTR);

    checkDirectPageAlignment();

    operand_addr = system->cpuRead(0, tmp, OPADDR) | (system->cpuRead(0, tmp + 1, OPADDR) << 8);
    operand_bank = system->cpuRead(0, tmp + 2, OPADDR);
}

void getAddress_axi()
{
    uint16_t tmp = (system->cpuRead(PBR, PC, INSTR) | (system->cpuRead(PBR, PC + 1, INSTR) << 8)) + X;

    PC += 2;

    operand_addr = system->cpuRead(PBR, tmp, OPADDR) | (system->cpuRead(PBR, tmp + 1, OPADDR) << 8);
    operand_bank = PBR;
}

void getAddress_sr()
{
    operand_addr = S + system->cpuRead(PBR, PC++, INSTR) + StackOffset;
    operand_bank = 0;
}

void getAddress_srix()
{
    uint16_t tmp = S + system->cpuRead(PBR, PC++, INSTR) + StackOffset;

    operand_addr = system->cpuRead(0, tmp, OPADDR) | (system->cpuRead(0, tmp + 1, OPADDR) << 8);
    operand_bank = DBR;

    tmp = operand_addr;
    operand_addr += Y;

    if (operand_addr < tmp) operand_bank++;
}
