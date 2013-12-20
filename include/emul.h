int EMUL_init(int, char **);
void EMUL_run(void);
void EMUL_reset(void);
void EMUL_shutdown(void);
void EMUL_trace(int val);
void EMUL_nmi(void);
void EMUL_handleWDM(byte val);
byte EMUL_getVBL(byte val);
