void executeOpcode(unsigned int opcode)
{
    switch (opcode) {
        case 0x00:  /* BRK s */
            ++PC;

            if (StackOffset) {
                stackPush(PC);
                stackPush(uint8_t(SR | 0x10));  // set B bit on stack
            }
            else {
                stackPush(PBR);
                stackPush(PC);
                stackPush(SR);
            }

            SR.D = false;
            SR.I = true;

            cpu->loadVector(StackOffset? 0xFFFE : 0xFFE6);

            break;

        case 0x01:  /* ORA (d,x) */
            getAddress_dxi();
            fetchOperand(operand.m);

            op_ORA();

            break;

        case 0x02:  /* COP s */
            ++PC;

            if (!StackOffset) {
                stackPush(PBR);
            }

            stackPush(PC);
            stackPush(SR);

            SR.D = false;
            SR.I = true;

            cpu->loadVector(StackOffset? 0xFFF4 : 0xFFE4);

            break;

        case 0x03:  /* ORA d,s */
            getAddress_sr();

            fetchOperand(operand.m);
            op_ORA();

            break;

        case 0x04:  /* TSB d */
            getAddress_d();

            fetchOperand(operand.m);
            op_TSB();
            storeOperand(operand.m);

            break;

        case 0x05:  /* ORA d */
            getAddress_d();

            fetchOperand(operand.m);
            op_ORA();

            break;

        case 0x06:  /* ASL d */
            getAddress_d();

            fetchOperand(operand.m);
            op_ASL();
            storeOperand(operand.m);

            break;

        case 0x07:  /* ORA [d] */
            getAddress_dil();

            fetchOperand(operand.m);
            op_ORA();

            break;

        case 0x08:  /* PHP s */
            stackPush(SR);

            break;

        case 0x09:  /* ORA # */
            fetchImmediateOperand(operand.m);
            op_ORA();

            break;

        case 0x0A:  /* ASL A */
            operand.m = A;
            op_ASL();
            A = operand.m;

            break;

        case 0x0B:  /* PHD s */
            stackPush(D);

            break;

        case 0x0C:  /* TSB a */
            getAddress_a();

            fetchOperand(operand.m);
            op_TSB();
            storeOperand(operand.m);

            break;

        case 0x0D:  /* ORA a */
            getAddress_a();

            fetchOperand(operand.m);
            op_ORA();

            break;

        case 0x0E:  /* ASL a */
            getAddress_a();

            fetchOperand(operand.m);
            op_ASL();
            storeOperand(operand.m);

            break;

        case 0x0F:  /* ORA al */
            getAddress_al();

            fetchOperand(operand.m);
            op_ORA();

            break;

        case 0x10:  /* BPL r */
            getAddress_pcr();

            if (!SR.N) {
                checkProgramPageCross();

                jumpTo(operand_addr);

                ++cpu->num_cycles;
            }

            break;

        case 0x11:  /* ORA (d),y */
            getAddress_dix();

            fetchOperand(operand.m);
            op_ORA();

            break;

        case 0x12:  /* ORA (d) */
            getAddress_di();

            fetchOperand(operand.m);
            op_ORA();

            break;

        case 0x13:  /* ORA (d,s),y */
            getAddress_srix();

            fetchOperand(operand.m);
            op_ORA();

            break;

        case 0x14:  /* TRB d */
            getAddress_d();

            fetchOperand(operand.m);
            op_TRB();
            storeOperand(operand.m);

            break;

        case 0x15:  /* ORA d,x */
            getAddress_dxx();

            fetchOperand(operand.m);
            op_ORA();

            break;

        case 0x16:  /* ASL d,x */
            getAddress_dxx();

            fetchOperand(operand.m);
            op_ASL();
            storeOperand(operand.m);

            break;

        case 0x17:  /* ORA [d],y */
            getAddress_dixl();

            fetchOperand(operand.m);
            op_ORA();

            break;

        case 0x18:  /* CLC i */
            SR.C = false;

            break;

        case 0x19:  /* ORA a,y */
            getAddress_axy();

            fetchOperand(operand.m);
            op_ORA();

            break;

        case 0x1A:  /* INC A */
            operand.m = A;
            op_INC();
            A = operand.m;

            break;

        case 0x1B:  /* TCS i */
            if (StackOffset) {
                S = A;
            }
            else {
                // native mode ignores M bit
                cpu->S.W = cpu->A.W;
            }

            break;

        case 0x1C:  /* TRB a */
            getAddress_a();
            fetchOperand(operand.m);

            op_TRB();
            storeOperand(operand.m);

            break;

        case 0x1D:  /* ORA a,x */
            getAddress_axx();
            fetchOperand(operand.m);

            op_ORA();

            break;

        case 0x1E:  /* ASL a,x */
            getAddress_axx();

            fetchOperand(operand.m);
            op_ASL();
            storeOperand(operand.m);

            break;

        case 0x1F:  /* ORA al,x */
            getAddress_alxx();
            fetchOperand(operand.m);

            op_ORA();

            break;

        case 0x20:  /* JSR a */
            getAddress_a();

            --PC;

            stackPush(PC);
            jumpTo(operand_addr);

            break;

        case 0x21:  /* AND (d,x) */
            getAddress_dxi();
            fetchOperand(operand.m);

            op_AND();

            break;

        case 0x22:  /* JSL al */
            getAddress_al();

            --PC;

            stackPush(PBR);
            stackPush(PC);

            jumpTo(operand_bank, operand_addr);

            break;

        case 0x23:  /* AND d,s */
            getAddress_sr();
            fetchOperand(operand.m);

            op_AND();

            break;

        case 0x24:  /* BIT d */
            getAddress_d();
            fetchOperand(operand.m);

            op_BIT();

            break;

        case 0x25:  /* AND d */
            getAddress_d();
            fetchOperand(operand.m);

            op_AND();

            break;

        case 0x26:  /* ROL d */
            getAddress_d();

            fetchOperand(operand.m);
            op_ROL();
            storeOperand(operand.m);

            break;

        case 0x27:  /* AND [d] */
            getAddress_dil();
            fetchOperand(operand.m);

            op_AND();

            break;

        case 0x28:  /* PLP s */
            {
                uint8_t v;

                stackPull(v);

                SR = v;
            }

            break;

        case 0x29:  /* AND # */
            fetchImmediateOperand(operand.m);

            op_AND();

            break;

        case 0x2A:  /* ROL A */
            operand.m = A;
            op_ROL();
            A = operand.m;

            break;

        case 0x2B:  /* PLD s */
            stackPull(D);

            checkIfNegative(D);
            checkIfZero(D);

            break;

        case 0x2C:  /* BIT a */
            getAddress_a();
            fetchOperand(operand.m);

            op_BIT();

            break;

        case 0x2D:  /* AND a */
            getAddress_a();
            fetchOperand(operand.m);

            op_AND();

            break;

        case 0x2E:  /* ROL a */
            getAddress_a();

            fetchOperand(operand.m);
            op_ROL();
            storeOperand(operand.m);

            break;

        case 0x2F:  /* AND al */
            getAddress_al();
            fetchOperand(operand.m);

            op_AND();

            break;

        case 0x30:  /* BMI r */
            getAddress_pcr();

            if (SR.N) {
                checkProgramPageCross();

                jumpTo(operand_addr);

                ++cpu->num_cycles;
            }

            break;

        case 0x31:  /* AND (d),y */
            getAddress_dix();
            fetchOperand(operand.m);

            op_AND();

            break;

        case 0x32:  /* AND (d) */
            getAddress_di();
            fetchOperand(operand.m);

            op_AND();

            break;

        case 0x33:  /* AND (d,s),y */
            getAddress_srix();
            fetchOperand(operand.m);

            op_AND();

            break;

        case 0x34:  /* BIT d,x */
            getAddress_dxx();
            fetchOperand(operand.m);

            op_BIT();

            break;

        case 0x35:  /* AND d,x */
            getAddress_dxx();
            fetchOperand(operand.m);

            op_AND();

            break;

        case 0x36:  /* ROL d,x */
            getAddress_dxx();

            fetchOperand(operand.m);
            op_ROL();
            storeOperand(operand.m);

            break;

        case 0x37:  /* AND [d],y */
            getAddress_dixl();
            fetchOperand(operand.m);

            op_AND();

            break;

        case 0x38:  /* SEC i */
            SR.C = true;

            break;

        case 0x39:  /* AND a,y */
            getAddress_axy();
            fetchOperand(operand.m);

            op_AND();

            break;

        case 0x3A:  /* DEC A */
            operand.m = A;
            op_DEC();
            A = operand.m;

            break;

        case 0x3B:  /* TSC i */
            // ignore M bit
            cpu->A.W = cpu->S.W;

            if (StackOffset) {
                checkIfNegative(A);
                checkIfZero(A);
            }
            else {
                checkIfNegative(cpu->A.W);
                checkIfZero(cpu->A.W);
            }

            break;

        case 0x3C:  /* BIT a,x */
            getAddress_axx();
            fetchOperand(operand.m);

            op_BIT();

            break;

        case 0x3D:  /* AND a,x */
            getAddress_axx();
            fetchOperand(operand.m);

            op_AND();

            break;

        case 0x3E:  /* ROL a,x */
            getAddress_axx();

            fetchOperand(operand.m);
            op_ROL();
            storeOperand(operand.m);

            break;

        case 0x3F:  /* AND al,x */
            getAddress_alxx();
            fetchOperand(operand.m);

            op_AND();

            break;

        case 0x40:  /* RTI */
            {
                uint8_t v;

                stackPull(v);

                SR = v;

                stackPull(PC);

                if (!StackOffset) {
                    stackPull(PBR);
                }
            }

            break;

        case 0x41:  /* EOR (d,x) */
            getAddress_dxi();
            fetchOperand(operand.m);

            op_EOR();

            break;

        case 0x42:  /* WDM */
            fetchImmediateOperand(operand.b);

            system->handleWdm(operand.b);

            break;

        case 0x43:  /* EOR d,s */
            getAddress_sr();
            fetchOperand(operand.m);

            op_EOR();

            break;

        case 0x44:  /* MVP xyc */
            op_MVP();

            break;

        case 0x45:  /* EOR d */
            getAddress_d();
            fetchOperand(operand.m);

            op_EOR();

            break;

        case 0x46:  /* LSR d */
            getAddress_d();
            fetchOperand(operand.m);

            op_LSR();
            storeOperand(operand.m);

            break;

        case 0x47:  /* EOR [d] */
            getAddress_dil();
            fetchOperand(operand.m);

            op_EOR();

            break;

        case 0x48:  /* PHA */
            stackPush(A);

            break;

        case 0x49:  /* EOR # */
            fetchImmediateOperand(operand.m);

            op_EOR();

            break;

        case 0x4A:  /* LSR A */
            operand.m = A;
            op_LSR();
            A = operand.m;

            break;

        case 0x4B:  /* PHK */
            stackPush(PBR);

            break;

        case 0x4C:  /* JMP a */
            getAddress_a();

            jumpTo(operand_addr);

            break;

        case 0x4D:  /* EOR a */
            getAddress_a();
            fetchOperand(operand.m);

            op_EOR();

            break;

        case 0x4E:  /* LSR a */
            getAddress_a();
            fetchOperand(operand.m);

            op_LSR();
            storeOperand(operand.m);

            break;

        case 0x4F:  /* EOR al */
            getAddress_al();
            fetchOperand(operand.m);

            op_EOR();

            break;

        case 0x50:  /* BVC r */
            getAddress_pcr();

            if (!SR.V) {
                checkProgramPageCross();

                jumpTo(operand_addr);

                ++cpu->num_cycles;
            }

            break;

        case 0x51:  /* EOR (d),y */
            getAddress_dix();
            fetchOperand(operand.m);

            op_EOR();

            break;

        case 0x52:  /* EOR (d) */
            getAddress_di();
            fetchOperand(operand.m);

            op_EOR();

            break;

        case 0x53:  /* EOR (d,s),y */
            getAddress_srix();
            fetchOperand(operand.m);

            op_EOR();

            break;

        case 0x54:  /* MVN xyc */
            op_MVN();

            break;

        case 0x55:  /* EOR d,x */
            getAddress_dxx();
            fetchOperand(operand.m);

            op_EOR();

            break;

        case 0x56:  /* LSR d,x */
            getAddress_dxx();
            fetchOperand(operand.m);

            op_LSR();
            storeOperand(operand.m);

            break;

        case 0x57:  /* EOR [d],y */
            getAddress_dixl();
            fetchOperand(operand.m);

            op_EOR();

            break;

        case 0x58:  /* CLI i */
            SR.I = 0;

            break;

        case 0x59:  /* EOR a,y */
            getAddress_axy();
            fetchOperand(operand.m);

            op_EOR();

            break;

        case 0x5A:  /* PHY s */
            stackPush(Y);

            break;

        case 0x5B:  /* TCD i */
            op_TCD();

            break;

        case 0x5C:  /* JMP al */
            getAddress_al();

            jumpTo(operand_bank, operand_addr);

            break;

        case 0x5D:  /* EOR a,x */
            getAddress_axx();
            fetchOperand(operand.m);

            op_EOR();

            break;

        case 0x5E:  /* LSR a,x */
            getAddress_axx();
            fetchOperand(operand.m);

            op_LSR();
            storeOperand(operand.m);

            break;

        case 0x5F:  /* EOR al,x */
            getAddress_alxx();
            fetchOperand(operand.m);

            op_EOR();

            break;

        case 0x60:  /* RTS s */
            stackPull(PC);

            ++PC;

            break;

        case 0x61:  /* ADC (d,x) */
            getAddress_dxi();
            fetchOperand(operand.m);

            op_ADC();

            break;

        case 0x62:  /* PER s */
            getAddress_pcrl();

            stackPush(operand_addr);

            break;

        case 0x63:  /* ADC d,s */
            getAddress_sr();

            fetchOperand(operand.m);
            op_ADC();

            break;

        case 0x64:  /* STZ d */
            getAddress_d();

            op_STZ();

            break;

        case 0x65:  /* ADC d */
            getAddress_d();
            fetchOperand(operand.m);

            op_ADC();

            break;

        case 0x66:  /* ROR d */
            getAddress_d();

            fetchOperand(operand.m);
            op_ROR();
            storeOperand(operand.m);

            break;

        case 0x67:  /* ADC [d] */
            getAddress_dil();
            fetchOperand(operand.m);

            op_ADC();

            break;

        case 0x68:  /* PLA s */
            stackPull(A);
            checkIfNegative(A);
            checkIfZero(A);

            break;

        case 0x69:  /* ADC # */
            fetchImmediateOperand(operand.m);

            op_ADC();

            break;

        case 0x6A:  /* ROR A */
            operand.m = A;
            op_ROR();
            A = operand.m;

            break;

        case 0x6B:  /* RTL s */
            stackPull(PC);
            stackPull(PBR);

            ++PC;

            break;

        case 0x6C:  /* JMP (a) */
            getAddress_ai();

            jumpTo(operand_addr);

            break;

        case 0x6D:  /* ADC a */
            getAddress_a();
            fetchOperand(operand.m);

            op_ADC();

            break;

        case 0x6E:  /* ROR a */
            getAddress_a();

            fetchOperand(operand.m);
            op_ROR();
            storeOperand(operand.m);

            break;

        case 0x6F:  /* ADC al */
            getAddress_al();
            fetchOperand(operand.m);

            op_ADC();

            break;

        case 0x70:  /* BVS r */
            getAddress_pcr();

            if (SR.V) {
                checkProgramPageCross();

                jumpTo(operand_addr);

                ++cpu->num_cycles;
            }

            break;

        case 0x71:  /* ADC (d),y */
            getAddress_dix();
            fetchOperand(operand.m);

            op_ADC();

            break;

        case 0x72:  /* ADC (d) */
            getAddress_di();
            fetchOperand(operand.m);

            op_ADC();

            break;

        case 0x73:  /* ADC (d,s),y */
            getAddress_srix();
            fetchOperand(operand.m);

            op_ADC();

            break;

        case 0x74:  /* STZ d,x */
            getAddress_dxx();

            op_STZ();

            break;

        case 0x75:  /* ADC d,x */
            getAddress_dxx();
            fetchOperand(operand.m);

            op_ADC();

            break;

        case 0x76:  /* ROR d,x */
            getAddress_dxx();

            fetchOperand(operand.m);
            op_ROR();
            storeOperand(operand.m);

            break;

        case 0x77:  /* ADC [d],y */
            getAddress_dixl();
            fetchOperand(operand.m);

            op_ADC();

            break;

        case 0x78:  /* SEI i */
            SR.I = true;

            break;

        case 0x79:  /* ADC a,y */
            getAddress_axy();
            fetchOperand(operand.m);

            op_ADC();

            break;

        case 0x7A:  /* PLY */
            stackPull(Y);
            checkIfNegative(Y);
            checkIfZero(Y);

            break;

        case 0x7B:  /* TDC i */
            op_TDC();

            break;

        case 0x7C:  /* JMP (a,x) */
            getAddress_axi();

            jumpTo(operand_addr);

            break;

        case 0x7D:  /* ADC a,x */
            getAddress_axx();
            fetchOperand(operand.m);

            op_ADC();

            break;

        case 0x7E:  /* ROR a,x */
            getAddress_axx();

            fetchOperand(operand.m);
            op_ROR();
            storeOperand(operand.m);

            break;

        case 0x7F:  /* ADC al,x */
            getAddress_alxx();
            fetchOperand(operand.m);

            op_ADC();

            break;

        case 0x80:  /* BRA r */
            getAddress_pcr();

            checkProgramPageCross();
            jumpTo(operand_addr);

            break;

        case 0x81:  /* STA (d,x) */
            getAddress_dxi();

            op_STA();

            break;

        case 0x82:  /* BRL rl */
            getAddress_pcrl();

            jumpTo(operand_bank, operand_addr);

            break;

        case 0x83:  /* STA d,s */
            getAddress_sr();

            op_STA();

            break;

        case 0x84:  /* STY d */
            getAddress_d();

            op_STY();

            break;

        case 0x85:  /* STA d */
            getAddress_d();

            op_STA();

            break;

        case 0x86:  /* STX d */
            getAddress_d();

            op_STX();

            break;

        case 0x87:  /* STA [d] */
            getAddress_dil();

            op_STA();

            break;

        case 0x88:  /* DEY i */
            --Y;

            checkIfNegative(Y);
            checkIfZero(Y);

            break;

        case 0x89:  /* BIT # */
            fetchImmediateOperand(operand.m);

            SR.Z = !(operand.m & A);

            break;

        case 0x8A:  /* TXA i */
            A = X;

            checkIfNegative(A);
            checkIfZero(A);

            break;

        case 0x8B:  /* PHB */
            stackPush(DBR);

            break;

        case 0x8C:  /* STY a */
            getAddress_a();

            op_STY();

            break;

        case 0x8D:  /* STA a */
            getAddress_a();

            op_STA();

            break;

        case 0x8E:  /* STX a */
            getAddress_a();

            op_STX();

            break;

        case 0x8F:  /* STA al */
            getAddress_al();

            op_STA();

            break;

        case 0x90:  /* BCC r */
            getAddress_pcr();

            if (!SR.C) {
                checkProgramPageCross();

                jumpTo(operand_addr);

                ++cpu->num_cycles;
            }

            break;

        case 0x91:  /* STA (d),y */
            getAddress_dix();

            op_STA();

            break;

        case 0x92:  /* STA (d) */
            getAddress_di();

            op_STA();

            break;

        case 0x93:  /* STA (d,s),y */
            getAddress_srix();

            op_STA();

            break;

        case 0x94:  /* STY d,x */
            getAddress_dxx();

            op_STY();

            break;

        case 0x95:  /* STA d,x */
            getAddress_dxx();

            op_STA();

            break;

        case 0x96:  /* STX d,y */
            getAddress_dxy();

            op_STX();

            break;

        case 0x97:  /* STA [d],y */
            getAddress_dixl();

            op_STA();

            break;

        case 0x98:  /* TYA i */
            A = Y;

            checkIfNegative(A);
            checkIfZero(A);

            break;

        case 0x99:  /* STA a,y */
            getAddress_axy();

            op_STA();

            break;

        case 0x9A:  /* TXS i */
            S = X;

            checkIfNegative(S);
            checkIfZero(S);

            break;

        case 0x9B:  /* TXY i */
            Y = X;

            checkIfNegative(Y);
            checkIfZero(Y);

            break;

        case 0x9C:  /* STZ a */
            getAddress_a();

            op_STZ();

            break;

        case 0x9D:  /* STA a,x */
            getAddress_axx();

            op_STA();

            break;

        case 0x9E:  /* STZ a,x */
            getAddress_axx();

            op_STZ();

            break;

        case 0x9F:  /* STA al,x */
            getAddress_alxx();

            op_STA();

            break;

        case 0xA0:  /* LDY # */
            fetchImmediateOperand(operand.x);

            op_LDY();

            break;

        case 0xA1:  /* LDA (d,x) */
            getAddress_dxi();
            fetchOperand(operand.m);

            op_LDA();

            break;

        case 0xA2:  /* LDX # */
            fetchImmediateOperand(operand.x);

            op_LDX();

            break;

        case 0xA3:  /* LDA d,s */
            getAddress_sr();
            fetchOperand(operand.m);

            op_LDA();

            break;

        case 0xA4:  /* LDY d */
            getAddress_d();
            fetchOperand(operand.x);

            op_LDY();

            break;

        case 0xA5:  /* LDA d */
            getAddress_d();
            fetchOperand(operand.m);

            op_LDA();

            break;

        case 0xA6:  /* LDX d */
            getAddress_d();
            fetchOperand(operand.x);

            op_LDX();

            break;

        case 0xA7:  /* LDA [d] */
            getAddress_dil();
            fetchOperand(operand.m);

            op_LDA();

            break;

        case 0xA8:  /* TAY i */
            // transfer all bits when x=0, even if m=1
            Y = static_cast<IndexSizeType> (cpu->A);

            checkIfNegative(Y);
            checkIfZero(Y);

            break;

        case 0xA9:  /* LDA # */
            fetchImmediateOperand(operand.m);

            op_LDA();

            break;

        case 0xAA:  /* TAX i */
            // transfer all bits when x=0, even if m=1
            X = static_cast<IndexSizeType> (cpu->A);

            checkIfNegative(X);
            checkIfZero(X);

            break;

        case 0xAB:  /* PLB s */
            stackPull(DBR);

            checkIfNegative(DBR);
            checkIfZero(DBR);

            break;

        case 0xAC:  /* LDY a */
            getAddress_a();
            fetchOperand(operand.x);

            op_LDY();

            break;

        case 0xAD:  /* LDA a */
            getAddress_a();
            fetchOperand(operand.m);

            op_LDA();

            break;

        case 0xAE:  /* LDX a */
            getAddress_a();
            fetchOperand(operand.x);

            op_LDX();

            break;

        case 0xAF:  /* LDA al */
            getAddress_al();
            fetchOperand(operand.m);

            op_LDA();

            break;

        case 0xB0:  /* BCS r */
            getAddress_pcr();

            if (SR.C) {
                checkProgramPageCross();

                jumpTo(operand_addr);

                ++cpu->num_cycles;
            }

            break;

        case 0xB1:  /* LDA (d),y */
            getAddress_dix();
            fetchOperand(operand.m);

            op_LDA();

            break;

        case 0xB2:  /* LDA (d) */
            getAddress_di();
            fetchOperand(operand.m);

            op_LDA();

            break;

        case 0xB3:  /* LDA (d,s),y */
            getAddress_srix();
            fetchOperand(operand.m);

            op_LDA();

            break;

        case 0xB4:  /* LDY d,x */
            getAddress_dxx();
            fetchOperand(operand.x);

            op_LDY();

            break;

        case 0xB5:  /* LDA d,x */
            getAddress_dxx();
            fetchOperand(operand.m);

            op_LDA();

            break;

        case 0xB6:  /* LDX d,y */
            getAddress_dxy();
            fetchOperand(operand.x);

            op_LDX();

            break;

        case 0xB7:  /* LDA [d],y */
            getAddress_dixl();
            fetchOperand(operand.m);

            op_LDA();

            break;

        case 0xB8:  /* CLV i */
            SR.V = false;

            break;

        case 0xB9:  /* LDA a,y */
            getAddress_axy();
            fetchOperand(operand.m);

            op_LDA();

            break;

        case 0xBA:  /* TSX i */
            X = S;

            checkIfNegative(X);
            checkIfZero(X);

            break;

        case 0xBB:  /* TYX i */
            X = Y;

            checkIfNegative(X);
            checkIfZero(X);

            break;

        case 0xBC:  /* LDY a,x */
            getAddress_axx();
            fetchOperand(operand.x);

            op_LDY();

            break;

        case 0xBD:  /* LDA a,x */
            getAddress_axx();
            fetchOperand(operand.m);

            op_LDA();

            break;

        case 0xBE:  /* LDX a,y */
            getAddress_axy();
            fetchOperand(operand.x);

            op_LDX();

            break;

        case 0xBF:  /* LDA al,x */
            getAddress_alxx();
            fetchOperand(operand.m);

            op_LDA();

            break;

        case 0xC0:  /* CPY # */
            fetchImmediateOperand(operand.x);
            op_CPY();

            break;

        case 0xC1:  /* CMP (d,x) */
            getAddress_dxi();

            fetchOperand(operand.m);
            op_CMP();

            break;

        case 0xC2:  /* REP # */
            fetchImmediateOperand(operand.b);

            SR &= ~operand.b;

            break;

        case 0xC3:  /* CMP d,s */
            getAddress_sr();

            fetchOperand(operand.m);
            op_CMP();

            break;

        case 0xC4:  /* CPY d */
            getAddress_d();

            fetchOperand(operand.x);
            op_CPY();

            break;

        case 0xC5:  /* CMP d */
            getAddress_d();

            fetchOperand(operand.m);
            op_CMP();

            break;

        case 0xC6:  /* DEC d */
            getAddress_d();
            fetchOperand(operand.m);

            op_DEC();
            storeOperand(operand.m);

            break;

        case 0xC7:  /* CMP [d] */
            getAddress_dil();

            fetchOperand(operand.m);
            op_CMP();

            break;

        case 0xC8:  /* INY */
            ++Y;

            checkIfNegative(Y);
            checkIfZero(Y);

            break;

        case 0xC9:  /* CMP # */
            fetchImmediateOperand(operand.m);
            op_CMP();

            break;

        case 0xCA:  /* DEX i */
            --X;

            checkIfNegative(X);
            checkIfZero(X);

            break;

        case 0xCB:  /* WAI */
            cpu->waiting = true;

            break;

        case 0xCC:  /* CPY a */
            getAddress_a();

            fetchOperand(operand.x);
            op_CPY();

            break;

        case 0xCD:  /* CMP a */
            getAddress_a();

            fetchOperand(operand.m);
            op_CMP();

            break;

        case 0xCE:  /* DEC a */
            getAddress_a();
            fetchOperand(operand.m);

            op_DEC();
            storeOperand(operand.m);

            break;

        case 0xCF:  /* CMP al */
            getAddress_al();

            fetchOperand(operand.m);
            op_CMP();

            break;

        case 0xD0:  /* BNE r */
            getAddress_pcr();

            if (!SR.Z) {
                checkProgramPageCross();

                jumpTo(operand_addr);

                ++cpu->num_cycles;
            }

            break;

        case 0xD1:  /* CMP (d),y */
            getAddress_dix();

            fetchOperand(operand.m);
            op_CMP();

            break;

        case 0xD2:  /* CMP (d) */
            getAddress_di();

            fetchOperand(operand.m);
            op_CMP();

            break;

        case 0xD3:  /* CMP (d,s),y */
            getAddress_srix();

            fetchOperand(operand.m);
            op_CMP();

            break;

        case 0xD4:  /* PEI d */
            getAddress_d();
            fetchOperand(operand.w);

            stackPush(operand.w);

            break;

        case 0xD5:  /* CMP d,x */
            getAddress_dxx();

            fetchOperand(operand.m);
            op_CMP();

            break;

        case 0xD6:  /* DEC d,x */
            getAddress_dxx();
            fetchOperand(operand.m);

            op_DEC();
            storeOperand(operand.m);

            break;

        case 0xD7:  /* CMP [d],y */
            getAddress_dixl();

            fetchOperand(operand.m);
            op_CMP();

            break;

        case 0xD8:  /* CLD i */
            SR.D = false;

            break;

        case 0xD9:  /* CMP a,y */
            getAddress_axy();

            fetchOperand(operand.m);
            op_CMP();

            break;

        case 0xDA:  /* PHX */
            stackPush(X);

            break;

        case 0xDB:  /* STP */
            cpu->stopped = true;

            break;

        case 0xDC:  /* JML (a) */
            getAddress_ail();

            jumpTo(operand_bank, operand_addr);

            break;

        case 0xDD:  /* CMP a,x */
            getAddress_axx();

            fetchOperand(operand.m);
            op_CMP();

            break;

        case 0xDE:  /* DEC a,x */
            getAddress_axx();
            fetchOperand(operand.m);

            op_DEC();
            storeOperand(operand.m);

            break;

        case 0xDF:  /* CMP al,x */
            getAddress_alxx();

            fetchOperand(operand.m);
            op_CMP();

            break;

        case 0xE0:  /* CPX # */
            fetchImmediateOperand(operand.x);
            op_CPX();

            break;

        case 0xE1: /* SBC (d,x) */
            getAddress_dxi();
            fetchOperand(operand.m);

            op_SBC();

            break;

        case 0xE2:  /* SEP # */
            fetchImmediateOperand(operand.b);

            SR |= operand.b;

            if (SR.X) {
                cpu->X.B.H = cpu->Y.B.H = 0;
            }

            break;

        case 0xE3:  /* SBC d,s */
            getAddress_sr();
            fetchOperand(operand.m);

            op_SBC();

            break;

        case 0xE4:  /* CPX d */
            getAddress_d();

            fetchOperand(operand.x);
            op_CPX();

            break;

        case 0xE5:  /* SBC d */
            getAddress_d();

            fetchOperand(operand.m);
            op_SBC();

            break;

        case 0xE6:  /* INC d */
            getAddress_d();

            fetchOperand(operand.m);
            op_INC();
            storeOperand(operand.m);

            break;

        case 0xE7:  /* SBC [d] */
            getAddress_di();

            fetchOperand(operand.m);
            op_SBC();

            break;

        case 0xE8:  /* INX */
            ++X;

            checkIfNegative(X);
            checkIfZero(X);

            break;

        case 0xE9:  /* SBC # */
            fetchImmediateOperand(operand.m);

            op_SBC();

            break;

        case 0xEA:
            break;

        case 0xEB:  /* XBA i */
            operand.b = cpu->A.B.H;
            cpu->A.B.H = cpu->A.B.L;
            cpu->A.B.L = operand.b;

            checkIfNegative(cpu->A.B.L);
            checkIfZero(cpu->A.B.L);

            break;

        case 0xEC:  /* CPX a */
            getAddress_a();

            fetchOperand(operand.x);
            op_CPX();

            break;

        case 0xED: /* SBC a */
            getAddress_a();

            fetchOperand(operand.m);
            op_SBC();

            break;

        case 0xEE:  /* INC a */
            getAddress_a();

            fetchOperand(operand.m);
            op_INC();
            storeOperand(operand.m);

            break;

        case 0xEF:  /* SBC al */
            getAddress_al();

            fetchOperand(operand.m);
            op_SBC();

            break;

        case 0xF0:  /* BEQ r */
            getAddress_pcr();

            if (SR.Z) {
                checkProgramPageCross();

                jumpTo(operand_addr);

                ++cpu->num_cycles;
            }

            break;

        case 0xF1:  /* SBC (d),y */
            getAddress_dix();

            fetchOperand(operand.m);
            op_SBC();

            break;

        case 0xF2:  /* SBC (d) */
            getAddress_di();

            fetchOperand(operand.m);
            op_SBC();

            break;

        case 0xF3:  /* SBC (d,s),y */
            getAddress_srix();

            fetchOperand(operand.m);
            op_SBC();

            break;

        case 0xF4:  /* PEA s */
            fetchImmediateOperand(operand.w);
            stackPush(operand.w);

            break;

        case 0xF5:  /* SBC d,x */
            getAddress_dxx();

            fetchOperand(operand.m);
            op_SBC();

            break;

        case 0xF6:  /* INC d,x */
            getAddress_dxx();

            fetchOperand(operand.m);
            op_INC();
            storeOperand(operand.m);

            break;

        case 0xF7:  /* SBC [d],y */
            getAddress_dixl();

            fetchOperand(operand.m);
            op_SBC();

            break;

        case 0xF8:  /* SED i */
            SR.D = true;

            break;

        case 0xF9:  /* SBC a,y */
            getAddress_axy();

            fetchOperand(operand.m);
            op_SBC();

            break;

        case 0xFA:  /* PLX s */
            stackPull(X);
            checkIfNegative(X);
            checkIfZero(X);

            break;

        case 0xFB:   /* XCE i */
            operand.b = SR.E;
            SR.E = SR.C;
            SR.C = operand.b;

            cpu->modeSwitch();

            break;

        case 0xFC:  /* JSR (a,x) */
            getAddress_axi();

            --PC;

            stackPush(PC);
            jumpTo(operand_addr);

            break;

        case 0xFD:  /* SBC a,x */
            getAddress_axx();
            fetchOperand(operand.m);

            op_SBC();

            break;

        case 0xFE:  /* INC a,x */
            getAddress_axx();
            fetchOperand(operand.m);

            op_INC();

            storeOperand(operand.m);

            break;

        case 0xFF:  /* SBC al,x */
            getAddress_alxx();
            fetchOperand(operand.m);

            op_SBC();

            break;

        case 0x100: /* irq */
            cpu->waiting = false;

            if (!StackOffset) {
                stackPush(PBR);
            }

            stackPush(PC);
            stackPush(SR);

            SR.D = false;
            SR.I = true;

            cpu->loadVector(StackOffset? 0xFFFE : 0xFFEE);

            break;

        case 0x101: /* nmi */
            cpu->nmi_pending = cpu->waiting = false;

            if (!StackOffset) {
                stackPush(PBR);
            }

            stackPush(PC);
            stackPush(SR);

            SR.D = false;
            SR.I = true;

            cpu->loadVector(StackOffset? 0xFFFA : 0xFFEA);

            break;

        case 0x102: /* abort */
            cpu->abort_pending = cpu->waiting = false;

            if (!StackOffset) {
                stackPush(PBR);
            }

            stackPush(PC);
            stackPush(SR);

            SR.D = false;
            SR.I = true;

            cpu->loadVector(StackOffset? 0xFFF8 : 0xFFE8);

            break;

        default:
            break;
    }
}
