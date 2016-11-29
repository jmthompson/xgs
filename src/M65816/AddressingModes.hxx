void getAddress_a()
{
    operand_addr = board->readMemory(PBR, PC++) | (board->readMemory(PBR, PC++) << 8);
    operand_bank = DBR;
}

void getAddress_al()
{
    operand_addr = board->readMemory(PBR, PC++) | (board->readMemory(PBR, PC++) << 8);
    operand_bank = board->readMemory(PBR, PC++);
}

void getAddress_d()
{
    checkDirectPageAlignment();

    operand_addr = D + board->readMemory(PBR, PC++);
    operand_bank = 0;
}

void getAddress_dix()
{
    checkDirectPageAlignment();

    uint16_t tmp = D + board->readMemory(PBR, PC++);

    operand_addr = board->readMemory(0, tmp) | (board->readMemory(0, tmp + 1) << 8);
    operand_bank = DBR;

    tmp = operand_addr;

    operand_addr += X;

    checkDataPageCross(tmp);
}

void getAddress_dixl()
{
    uint16_t tmp = D + board->readMemory(PBR, PC++);

    checkDirectPageAlignment();

    operand_addr = board->readMemory(0, tmp) | (board->readMemory(0, tmp + 1) << 8) + X;
    operand_bank = board->readMemory(0, tmp + 2);
}

void getAddress_dxi()
{
    uint16_t tmp = D + X + board->readMemory(PBR, PC++);

    checkDirectPageAlignment();

    operand_addr = board->readMemory(0, tmp) | (board->readMemory(0, tmp + 1) << 8);
    operand_bank = DBR;
}

void getAddress_dxx()
{
    checkDirectPageAlignment();

    operand_addr = D + X + board->readMemory(PBR, PC++);
    operand_bank = 0;
}

void getAddress_dxy()
{
    checkDirectPageAlignment();

    operand_addr = D + Y + board->readMemory(PBR, PC++);
    operand_bank = 0;
}

void getAddress_axx()
{
    operand_addr = board->readMemory(PBR, PC) | (board->readMemory(PBR, PC + 1) << 8);
    operand_bank = DBR;

    uint16_t tmp = operand_addr;

    operand_addr += X;

    checkDataPageCross(tmp);

    PC += 2;
}

void getAddress_axy()
{
    operand_addr = board->readMemory(PBR, PC) | (board->readMemory(PBR, PC + 1) << 8);
    operand_bank = DBR;

    uint16_t tmp = operand_addr;

    operand_addr += Y;

    checkDataPageCross(tmp);

    PC += 2;
}

void getAddress_alxx()
{
    operand_addr = board->readMemory(PBR, PC) | (board->readMemory(PBR, PC + 1) << 8) + X;
    operand_bank = board->readMemory(PBR, PC + 2);

    PC += 3;
}

void getAddress_pcr()
{
    int8_t offset = board->readMemory(PBR, PC++);

    operand_addr = PC + offset;
    operand_bank = PBR;
}

void getAddress_pcrl()
{
    int16_t offset = board->readMemory(PBR, PC++) | (board->readMemory(PBR, PC++) << 8);

    operand_addr = PC + offset;
    operand_bank = PBR;
}

void getAddress_ai()
{
    uint16_t tmp = board->readMemory(PBR, PC) | (board->readMemory(PBR, PC + 1) << 8);

    PC += 2;

    operand_addr = board->readMemory(0, tmp) | (board->readMemory(0, tmp + 1) << 8);
    operand_bank = 0;
}

void getAddress_ail()
{
    uint16_t tmp = board->readMemory(PBR, PC) | (board->readMemory(PBR, PC + 1) << 8);

    PC += 2;

    operand_addr = board->readMemory(0, tmp) | (board->readMemory(0, tmp + 1) << 8);
    operand_bank = board->readMemory(0, tmp + 2);
}

void getAddress_di()
{
    uint16_t tmp = D + board->readMemory(PBR, PC++);

    checkDirectPageAlignment();

    operand_addr = board->readMemory(0, tmp) | (board->readMemory(0, tmp + 1) << 8);
    operand_bank = DBR;
}

void getAddress_dil()
{
    uint16_t tmp = D + board->readMemory(PBR, PC++);

    checkDirectPageAlignment();

    operand_addr = board->readMemory(0, tmp) | (board->readMemory(0, tmp + 1) << 8);
    operand_bank = board->readMemory(0, tmp + 2);
}

void getAddress_axi()
{
    uint16_t tmp = board->readMemory(PBR, PC) | (board->readMemory(PBR, PC + 1) << 8) + X;

    PC += 2;

    operand_addr = board->readMemory(0, tmp) | (board->readMemory(0, tmp + 1) << 8);
    operand_bank = PBR;
}

void getAddress_sr()
{
    operand_addr = S + board->readMemory(PBR, PC++) + StackOffset;
    operand_bank = 0;
}

void getAddress_srix()
{
    uint16_t tmp = S + board->readMemory(PBR, PC++) + StackOffset;

    operand_addr = board->readMemory(0, tmp) | (board->readMemory(0, tmp + 1) << 8);
    operand_bank = DBR;

    operand_addr += Y;
}
