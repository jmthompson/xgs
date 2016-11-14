divert(-1)

dnl
dnl Macros to implement actual opcode logic
dnl 

dnl Status bit management

define(`OPCODE_REP', `MASK_STATUS_BITS(0)')
define(`OPCODE_SEP', `MASK_STATUS_BITS(1)')

dnl Bitwise operations

define(`OPCODE_AND', `dnl
            REG_A &= OPERAND_m;

            CHECK_IF_NEGATIVE_m(A)
            CHECK_IF_ZERO_m(A)')

define(`OPCODE_BIT', `dnl
            P.Z = (OPERAND_m & REG_A)? 0 : 1;
            P.N = (OPERAND_m & N_BIT)? 1 : 0;
            P.V = (OPERAND_m & V_BIT)? 1 : 0;
')

define(`OPCODE_EOR', `dnl
            REG_A ^= OPERAND_m;

            CHECK_IF_NEGATIVE_m(A)
            CHECK_IF_ZERO_m(A)')

define(`OPCODE_ORA', `dnl
            REG_A |= OPERAND_m;

            CHECK_IF_NEGATIVE_m(A)
            CHECK_IF_ZERO_m(A)')

define(`OPCODE_TRB', `dnl
            P.Z = (OPERAND_m & REG_A)? 0 : 1;

            OPERAND_m &= ~REG_A;')

define(`OPCODE_TSB', `dnl
            P.Z = (OPERAND_m & REG_A)? 0 : 1;

            OPERAND_m |= REG_A;')

dnl Increment/decrement

define(`OPCODE_DEC', `dnl
            TARGET_m($1)--;
            CHECK_IF_NEGATIVE_m($1)
            CHECK_IF_ZERO_m($1)')

define(`OPCODE_INC', `dnl
            TARGET_m($1)++;
            CHECK_IF_NEGATIVE_m($1)
            CHECK_IF_ZERO_m($1)')

dnl Bit shifts/rotates

define(`OPCODE_ASL', `dnl
            P.C = TARGET_m($1) & N_BIT? 1 : 0;

            TARGET_m($1) <<= 1;

            CHECK_IF_NEGATIVE_m($1)
            CHECK_IF_ZERO_m($1)')

define(`OPCODE_LSR', `dnl
            P.C = TARGET_m($1) & 0x01;

            TARGET_m($1) >>= 1;

            CHECK_IF_NEGATIVE_m($1)
            CHECK_IF_ZERO_m($1)')

define(`OPCODE_ROL', `dnl
            tmp = P.C;
            P.C = TARGET_m($1) & N_BIT? 1 : 0;

            TARGET_m($1) = (TARGET_m($1) << 1) | tmp;

            CHECK_IF_NEGATIVE_m($1)
            CHECK_IF_ZERO_m($1)')

define(`OPCODE_ROR', `dnl
            tmp = P.C;
            P.C = TARGET_m($1) & 0x01;

            TARGET_m($1) = TARGET_m($1) >> 1;
            if (tmp) TARGET_m($1) |= N_BIT;

            CHECK_IF_NEGATIVE_m($1)
            CHECK_IF_ZERO_m($1)')

dnl Jumps and returns

define(`OPCODE_JMP', `PC.A = address.A;')

define(`OPCODE_JSR', `dnl
            PC.W.L--;
            PUSH_w(PC)

            JUMP_w')

dnl Stack-related instructions

define(`OPCODE_PLA', ifelse(SHORT_M, `1', `dnl
            PULL_b(A.B.L)
            CHECK_IF_NEGATIVE_b(A.B.L)
            CHECK_IF_ZERO_b(A.B.L)
', `dnl
            PULL_w(A)
            CHECK_IF_NEGATIVE_w(A.W)
            CHECK_IF_ZERO_w(A.W)
'))

define(`OPCODE_PLX', ifelse(SHORT_X, `1', `dnl
            PULL_b(X.B.L)
            CHECK_IF_NEGATIVE_b(X.B.L)
            CHECK_IF_ZERO_b(X.B.L)
', `dnl
            PULL_w(X)
            CHECK_IF_NEGATIVE_w(X.W)
            CHECK_IF_ZERO_w(X.W)
'))

define(`OPCODE_PLY', ifelse(SHORT_X, `1', `dnl
            PULL_b(Y.B.L)
            CHECK_IF_NEGATIVE_b(Y.B.L)
            CHECK_IF_ZERO_b(Y.B.L)
', `dnl
            PULL_w(Y)
            CHECK_IF_NEGATIVE_w(Y.W)
            CHECK_IF_ZERO_w(Y.W)
'))

dnl Compares

define(`OPCODE_CMP', ifelse(SHORT_M, `1', `COMPARE_b(A, operand)', `COMPARE_w(A, operand)'))
define(`OPCODE_CPX', ifelse(SHORT_X, `1', `COMPARE_b(X, operand)', `COMPARE_w(X, operand)'))
define(`OPCODE_CPY', ifelse(SHORT_X, `1', `COMPARE_b(Y, operand)', `COMPARE_w(Y, operand)'))

dnl Block moves

define(`OPCODE_MVP', `dnl
            operand.B.L = READ_BYTE(PC.A);     // dest bank
            operand.B.H = READ_BYTE(PC.A + 1); // src bank

            if (A.W != 0xFFFF) {
                WRITE_BYTE((operand.B.L << 16) | Y.W, READ_BYTE((operand.B.H << 16) | X.W));

                A.W--;
                REG_X--;
                REG_Y--;

                PC.W.L--;
            }
            else {
                PC.W.L += 2;
            }
')

define(`OPCODE_MVN', `dnl
            operand.B.L = READ_BYTE(PC.A);     // dest bank
            operand.B.H = READ_BYTE(PC.A + 1); // src bank

            if (A.W != 0xFFFF) {
                WRITE_BYTE((operand.B.L << 16) | Y.W, READ_BYTE((operand.B.H << 16) | X.W));

                A.W--;
                REG_X++;
                REG_Y++;

                PC.W.L--;
            }
            else {
                PC.W.L += 2;
            }
')

dnl Loads and stores

define(`OPCODE_LDA', `dnl
            REG_A = OPERAND_m;

            CHECK_IF_NEGATIVE_m(A);
            CHECK_IF_ZERO_m(A);
')

define(`OPCODE_LDX', `dnl
            REG_X = OPERAND_x;

            CHECK_IF_NEGATIVE_x(X);
            CHECK_IF_ZERO_x(X);
')

define(`OPCODE_LDY', `dnl
            REG_Y = OPERAND_x;

            CHECK_IF_NEGATIVE_x(Y);
            CHECK_IF_ZERO_x(Y);
')

define(`OPCODE_STA', ifelse(SHORT_M, `1',
    `WRITE_BYTE(address.A, A.B.L);',
    `WRITE_BYTE(address.A, A.B.L); WRITE_BYTE(address.A + 1, A.B.H);'
))

define(`OPCODE_STX', ifelse(SHORT_X, `1',
    `WRITE_BYTE(address.A, X.B.L);',
    `WRITE_BYTE(address.A, X.B.L); WRITE_BYTE(address.A + 1, X.B.H);'
))

define(`OPCODE_STY', ifelse(SHORT_X, `1',
    `WRITE_BYTE(address.A, Y.B.L);',
    `WRITE_BYTE(address.A, Y.B.L); WRITE_BYTE(address.A + 1, Y.B.H);'
))

define(`OPCODE_STZ', ifelse(SHORT_M, `1',
    `WRITE_BYTE(address.A, 0);',
    `WRITE_BYTE(address.A, 0); WRITE_BYTE(address.A + 1, 0);'
))

dnl Register transfers

define(`OPCODE_TCD', `dnl
            D.W = A.W;

            CHECK_IF_NEGATIVE_w(D.W)
            CHECK_IF_ZERO_w(D.W)
')

define(`OPCODE_TDC', `dnl
            A.W = D.W;

            CHECK_IF_NEGATIVE_w(A.W)
            CHECK_IF_ZERO_w(A.W)
')

define(`OPCODE_TAX', `dnl
            REG_X = TARGET_x(A);

            CHECK_IF_NEGATIVE_x(X)
            CHECK_IF_ZERO_x(X)
')

define(`OPCODE_TAY', `dnl
            REG_Y = TARGET_x(A);

            CHECK_IF_NEGATIVE_x(Y)
            CHECK_IF_ZERO_x(Y)
')

define(`OPCODE_TSX', `dnl
            S.W = X.W;

            CHECK_IF_NEGATIVE_x(X)
            CHECK_IF_ZERO_x(X)
')

define(`OPCODE_TXA', `dnl
            REG_A = TARGET_m(X);

            CHECK_IF_NEGATIVE_m(A)
            CHECK_IF_ZERO_m(A)
')

define(`OPCODE_TXS', `S.W = X.W;')

define(`OPCODE_TXY', `dnl
            REG_Y = REG_X;

            CHECK_IF_NEGATIVE_x(Y)
            CHECK_IF_ZERO_x(Y)
')

define(`OPCODE_TYA', `dnl
            REG_A = TARGET_m(Y);

            CHECK_IF_NEGATIVE_m(A)
            CHECK_IF_ZERO_m(A)
')

define(`OPCODE_TYX', `dnl
            REG_X = REG_Y;

            CHECK_IF_NEGATIVE_x(X)
            CHECK_IF_ZERO_x(X)
')

define(`OPCODE_XBA', `dnl
            tmp = A.B.H;
            A.B.H = A.B.L;
            A.B.L = tmp;

            CHECK_IF_NEGATIVE_b(A.B.L)
            CHECK_IF_ZERO_b(A.B.L)
')

define(`OPCODE_XCE', `dnl
            tmp = P.E;
            P.E = P.C;
            P.C = tmp;

            modeSwitch();
')

dnl Addition and subtraction

define(`OPCODE_ADC', `dnl
            if (P.D) {
                tmp = decimalAdd((int) REG_A, (int) OPERAND_m);
            }
            else {
                tmp = REG_A + OPERAND_m + P.C;

                P.C = (tmp > MAX_M);
            }

            P.V = ((~(tmp ^ OPERAND_m) & (tmp ^ REG_A)) & N_BIT)? 1 : 0;

            REG_A = tmp & MAX_M;

            CHECK_IF_NEGATIVE_m(A)
            CHECK_IF_ZERO_m(A)
')

define(`OPCODE_SBC', `dnl
            if (P.D) {
                tmp = decimalSubtract((int) REG_A, (int) OPERAND_m);
            }
            else {
                tmp = A.B.L - operand.B.L - !P.C;

                P.C = wtmp.B.H? 0 : 1;
            }

            P.V = ((tmp ^ OPERAND_m) & (tmp ^ REG_A) & N_BIT)? 1 : 0;

            REG_A = tmp & MAX_M;

            CHECK_IF_NEGATIVE_m(A)
            CHECK_IF_ZERO_m(A)
'))

divert(0)
