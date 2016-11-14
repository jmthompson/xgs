divert(-1)
dnl Macros for reading from or writing to system memory

define(`READ_BYTE',  `board->readMemory($1)')
define(`WRITE_BYTE', `board->writeMemory($1,$2)')

dnl Macros that resolve to the N/V bits of an operand, regardles of the
dnl dnl setting of the M bit.

define(`N_BIT', ifelse(SHORT_M, `1', `0x80', `0x8000'))
define(`V_BIT', ifelse(SHORT_M, `1', `0x40', `0x4000'))

dnl Macro that resolve to the maximum value of the accumulator

define(`MAX_M', ifelse(SHORT_M, `1', `0xFF', `0xFFFF'))

dnl Macros that expand into the byte or word version of the operand, based on
dnl the state of the M or X bit.

define(`OPERAND_m', ifelse(SHORT_M, `1', `operand.B.L', `operand.W'))
define(`OPERAND_x', ifelse(SHORT_X, `1', `operand.B.L', `operand.W'))

dnl Expand a reference to a dualw variable into either the byte or word
dnl version, based on the state of the M or X bit.

define(`TARGET_m', ifelse(SHORT_M, `1', `$1.B.L', `$1.W'))
define(`TARGET_x', ifelse(SHORT_X, `1', `$1.B.L', `$1.W'))

dnl Macros that expand into a reference to a CPU register, either the byte or
dnl word version depending on the status of the E, M, and X bits.

define(`REG_A',  TARGET_m(A))
define(`REG_X',  TARGET_x(X))
define(`REG_Y',  TARGET_x(Y))
define(`REG_S',  ifelse(EMULATION, `1', `S.B.L', `S.W'))

dnl Construct the status register value from the individual boolean flags

define(`STATUS_REG', ifelse(EMULATION, `1', `(P.N|P.V|P.D|P.I|P.Z|P.C)', `(P.N|P.V|P.M|P.X|P.D|P.I|P.Z|P.C)'))

dnl This macro resolves the status register value of the BRK bit when running
dnl in emulation mode, and to 0x00 in native mode (which has no BRK bit).

define(`BRK_BIT', ifelse(EMULATION, `1', `0x10', `0x00'))

dnl Set all status register bits set in the mask in operand to a specific value

define(`MASK_STATUS_BITS', ifelse(EMULATION, `1', `dnl
            if (operand.B.L & 0x80) P.N = $1;
            if (operand.B.L & 0x40) P.V = $1;
            if (operand.B.L & 0x08) P.D = $1;
            if (operand.B.L & 0x04) P.I = $1;
            if (operand.B.L & 0x02) P.Z = $1;
            if (operand.B.L & 0x01) P.C = $1;
', `dnl
            if (operand.B.L & 0x80) P.N = $1;
            if (operand.B.L & 0x40) P.V = $1;
            if (operand.B.L & 0x20) P.M = $1;
            if (operand.B.L & 0x10) { P.X = $1; modeSwitch(); }
            if (operand.B.L & 0x08) P.D = $1;
            if (operand.B.L & 0x04) P.I = $1;
            if (operand.B.L & 0x02) P.Z = $1;
            if (operand.B.L & 0x01) P.C = $1;
'))

dnl Load one of two vector addresses into the PC. The first value is loaded
dnl when in emulation mode, and the second value when in native mode.

define(`LOAD_VECTOR', ifelse(EMULATION, `1',
    `PC.B.B = 0x00; PC.B.L = READ_BYTE($1); PC.B.H  = READ_BYTE($1 + 1);',
    `PC.B.B = 0x00; PC.B.L = READ_BYTE($2); PC.B.H  = READ_BYTE($2 + 1);'
))

dnl Add one cycle if the PC crossing a page boundary in emulation mode

define(`CHECK_PC_PAGE_CROSS', ifelse(EMULATION, `1', `if (PC.B.H != address.B.H) num_cycles++;', `'))

dnl Add one cyce if two addresses are on different pages

define(`CHECK_PAGE_CROSS', `if ($1.B.H != $2.B.H) num_cycles++;')

dnl Add one cycle if the data bank register is not page-aligned

define(`CHECK_DP_ALIGN', `if (D.B.L != 0) num_cycles++;')

dnl Macros for pushing or pulling bytes on the 65816 stack

define(`PUSH_b', `WRITE_BYTE(REG_S--,$1);')
define(`PUSH_w', `PUSH_b($1.B.H) PUSH_b($1.B.L)')
define(`PUSH_l', `PUSH_b($1.B.B) PUSH_b($1.B.H) PUSH_b($1.B.L)')

define(`PULL_b', `$1 = READ_BYTE(++REG_S);')
define(`PULL_w', `PULL_b($1.B.L) PULL_b($1.B.H)')
define(`PULL_l', `PULL_b($1.B.L) PULL_b($1.B.H) PULL_b($1.B.B)')

define(`PULL_SR', ifelse(EMULATION, `1', `dnl
            PULL_b(tmp)

            P.N = tmp & 0x80? 1 : 0;
            P.V = tmp & 0x40? 1 : 0;
            P.D = tmp & 0x08? 1 : 0;
            P.I = tmp & 0x04? 1 : 0;
            P.Z = tmp & 0x02? 1 : 0;
            P.C = tmp & 0x01;

            modeSwitch();
', `dnl
            PULL_b(tmp)

            P.N = tmp & 0x80? 1 : 0;
            P.V = tmp & 0x40? 1 : 0;
            P.M = tmp & 0x20? 1 : 0;
            P.X = tmp & 0x10? 1 : 0;
            P.D = tmp & 0x08? 1 : 0;
            P.I = tmp & 0x04? 1 : 0;
            P.Z = tmp & 0x02? 1 : 0;
            P.C = tmp & 0x01;

            modeSwitch();
'))

define(`PUSH_PC', ifelse(EMULATION, `1', `PUSH_w(PC)', `PULL_l(PC)'))
define(`PULL_PC', ifelse(EMULATION, `1', `PULL_w(PC)', `PULL_l(PC)'))

dnl Set the N and Z flag based on the passed value

define(`CHECK_IF_NEGATIVE_b', `P.N = ($1 & 0x80)? 1 : 0;')
define(`CHECK_IF_NEGATIVE_w', `P.N = ($1 & 0x8000)? 1 : 0;')

define(`CHECK_IF_NEGATIVE_m', ifelse(SHORT_M, `1', `CHECK_IF_NEGATIVE_b($1.B.L)', `CHECK_IF_NEGATIVE_w($1.W)'))
define(`CHECK_IF_NEGATIVE_x', ifelse(SHORT_X, `1', `CHECK_IF_NEGATIVE_b($1.B.L)', `CHECK_IF_NEGATIVE_w($1.W)'))

define(`CHECK_IF_ZERO_b', `P.Z = $1? 1 : 0;')
define(`CHECK_IF_ZERO_w', `P.Z = $1? 1 : 0;')

define(`CHECK_IF_ZERO_m', ifelse(SHORT_M, `1', `CHECK_IF_ZERO_b($1.B.L)', `CHECK_IF_ZERO_w($1.W)'))
define(`CHECK_IF_ZERO_x', ifelse(SHORT_X, `1', `CHECK_IF_ZERO_b($1.B.L)', `CHECK_IF_ZERO_w($1.W)'))

dnl Macros to fetch and store the operand

define(`FETCH_IMMEDIATE_OPERAND_b', `operand.B.L = READ_BYTE(PC.A); operand.B.H = 0; PC.W.L++;')
define(`FETCH_IMMEDIATE_OPERAND_w', `operand.B.L = READ_BYTE(PC.A); operand.B.H = READ_BYTE(PC.A + 1); PC.W.L += 2;')

define(`FETCH_IMMEDIATE_OPERAND_m', ifelse(SHORT_M, `1', `FETCH_IMMEDIATE_OPERAND_b', `FETCH_IMMEDIATE_OPERAND_w'))
define(`FETCH_IMMEDIATE_OPERAND_x', ifelse(SHORT_X, `1', `FETCH_IMMEDIATE_OPERAND_b', `FETCH_IMMEDIATE_OPERAND_w'))

define(`FETCH_OPERAND_b', `operand.B.L = READ_BYTE(address.A); operand.B.H = 0;')
define(`FETCH_OPERAND_w', `operand.B.L = READ_BYTE(address.A); operand.B.H = READ_BYTE(address.A + 1);')

define(`FETCH_OPERAND_m', ifelse(SHORT_M, `1', `FETCH_OPERAND_b', `FETCH_OPERAND_w'))
define(`FETCH_OPERAND_x', ifelse(SHORT_X, `1', `FETCH_OPERAND_b', `FETCH_OPERAND_w'))

define(`STORE_OPERAND_b', `WRITE_BYTE(address.A, operand.B.L);')
define(`STORE_OPERAND_w', `WRITE_BYTE(address.A, operand.B.L); WRITE_BYTE(address.A + 1, operand.B.H);')

define(`STORE_OPERAND_m', ifelse(SHORT_M, `1', `STORE_OPERAND_b', `STORE_OPERAND_w'))
define(`STORE_OPERAND_x', ifelse(SHORT_X, `1', `STORE_OPERAND_b', `STORE_OPERAND_w'))

dnl Macros to fetch the operand address

define(`GET_ADDRESS_a', `
            address.B.L = READ_BYTE(PC.A);
            address.B.H = READ_BYTE(PC.A+1);
            address.B.B = DB;

            PC.W.L += 2;')

define(`GET_ADDRESS_al', `
            address.B.L = READ_BYTE(PC.A);
            address.B.H = READ_BYTE(PC.A+1);
            address.B.B = READ_BYTE(PC.A+2);

            PC.W.L += 3;')

define(`GET_ADDRESS_d', `
            CHECK_DP_ALIGN

            address.W.L = D.W + READ_BYTE(PC.A);
            address.W.H = 0;

            PC.W.L++;')

define(`GET_ADDRESS_dix', `
            CHECK_DP_ALIGN

            atmp.W.L = D.W + READ_BYTE(PC.A);
            atmp.W.H = 0;

            PC.W.L++;

            address.B.L = READ_BYTE(atmp.A);
            address.B.H = READ_BYTE(atmp.A+1);
            address.B.B = DB;

            atmp.A = address.A;

            address.W.L += REG_X;

            CHECK_PAGE_CROSS(atmp, address)')

define(`GET_ADDRESS_dixl', `
            CHECK_DP_ALIGN

            atmp.W.L = D.W + READ_BYTE(PC.A);
            atmp.W.H = 0;

            PC.W.L++;

            address.B.L = READ_BYTE(atmp.A);
            address.B.H = READ_BYTE(atmp.A+1);
            address.B.B = READ_BYTE(atmp.A+2);

            address.A += REG_X;')

define(`GET_ADDRESS_dxi', `
            CHECK_DP_ALIGN

            atmp.W.L = D.W + READ_BYTE(PC.A) + REG_X;
            atmp.W.H = 0;

            PC.W.L++;

            address.B.L = READ_BYTE(atmp.A);
            address.B.H = READ_BYTE(atmp.A+1);
            address.B.B = DB;')

define(`GET_ADDRESS_dxx', `
            CHECK_DP_ALIGN

            address.W.L = READ_BYTE(PC.A) + D.W + REG_X;
            address.W.H = 0;

            PC.W.L++;')

define(`GET_ADDRESS_dxy', `
            CHECK_DP_ALIGN

            address.W.L = READ_BYTE(PC.A) + D.W + REG_Y;
            address.W.H = 0;

            PC.W.L++;')

define(`GET_ADDRESS_axx', `
            address.B.L = READ_BYTE(PC.A);
            address.B.H = READ_BYTE(PC.A+1);
            address.B.B = DB;

            atmp.A = address.A;

            address.W.L += REG_X;

            CHECK_PAGE_CROSS(atmp, address)

            PC.W.L+=2;')

define(`GET_ADDRESS_axy', `
            address.B.L = READ_BYTE(PC.A);
            address.B.H = READ_BYTE(PC.A+1);
            address.B.B = DB;

            atmp.A = address.A;

            address.W.L += REG_Y;

            CHECK_PAGE_CROSS(atmp, address)

            PC.W.L+=2;')

define(`GET_ADDRESS_alxx', `
            address.B.L = READ_BYTE(PC.A);
            address.B.H = READ_BYTE(PC.A+1);
            address.B.B = READ_BYTE(PC.A+2);

            address.A += X.W;

            PC.W.L += 3;')

define(`GET_ADDRESS_pcr', `
            wtmp.B.L = READ_BYTE(PC.A);

            PC.W.L++;

            address.W.L = PC.W.L + (offset_s) wtmp.B.L;
            address.B.B = PC.B.B;')

define(`GET_ADDRESS_pcrl', `
            wtmp.B.L = READ_BYTE(PC.A);
            wtmp.B.H = READ_BYTE(PC.A+1);

            PC.W.L += 2;

            address.W.L = PC.W.L + (offset_l) wtmp.W;
            address.B.B = PC.B.B;')

define(`GET_ADDRESS_ai', `
            atmp.B.L = READ_BYTE(PC.A);
            atmp.B.H = READ_BYTE(PC.A+1);
            atmp.B.B = 0;

            PC.W.L += 2;

            address.B.L = READ_BYTE(atmp.A);
            address.B.H = READ_BYTE(atmp.A+1);
            address.B.B = 0;')

define(`GET_ADDRESS_ail', `
            atmp.B.L = READ_BYTE(PC.A);
            atmp.B.H = READ_BYTE(PC.A+1);
            atmp.B.B = 0;

            PC.W.L += 2;

            address.B.L = READ_BYTE(atmp.A);
            address.B.H = READ_BYTE(atmp.A+1);
            address.B.B = READ_BYTE(atmp.A+2);')

define(`GET_ADDRESS_di', `
            CHECK_DP_ALIGN

            atmp.A = READ_BYTE(PC.A) + D.W;

            PC.W.L++;

            address.B.L = READ_BYTE(atmp.A);
            address.B.H = READ_BYTE(atmp.A+1);
            address.B.B = DB;')

define(`GET_ADDRESS_dil', `
            CHECK_DP_ALIGN

            atmp.A = READ_BYTE(PC.A) + D.W;

            PC.W.L++;

            address.B.L = READ_BYTE(atmp.A);
            address.B.H = READ_BYTE(atmp.A+1);
            address.B.B = READ_BYTE(atmp.A+2);')

define(`GET_ADDRESS_axi', `
            atmp.B.L = READ_BYTE(PC.A);
            atmp.B.H = READ_BYTE(PC.A+1);

            atmp.A += X.W;

            atmp.B.B = PC.B.B;

            PC.W.L += 2;

            address.B.L = READ_BYTE(atmp.A);
            address.B.H = READ_BYTE(atmp.A+1);
            address.B.B = PC.B.B;')

define(`GET_ADDRESS_sr', `
            address.W.L = READ_BYTE(PC.A) + S.W;
            address.B.B = 0;

            PC.W.L++;')

define(`GET_ADDRESS_srix', `dnl
            atmp.W.L = READ_BYTE(PC.A) + S.W;
            atmp.B.B = 0;

            PC.W.L++;

            address.B.L = READ_BYTE(atmp.A);
            address.B.H = READ_BYTE(atmp.A+1);
            address.B.B = DB;

            address.A += Y.W;')

dnl Compare two dualw variables

define(COMPARE_b, `dnl
            tmp = $1.B.L - $2.B.L;

            CHECK_IF_ZERO_b(tmp)
            CHECK_IF_NEGATIVE_b(tmp)')

define(COMPARE_w, `dnl
            tmp = $1.W - $2.W;

            CHECK_IF_ZERO_w(tmp)
            CHECK_IF_NEGATIVE_w(tmp)')

dnl Control flow

define(`JUMP_w', `PC.W.L = address.W.L;')
define(`JUMP_l', `PC.A = address.A;')

divert(0)
