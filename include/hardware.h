int hardwareInit();
void hardwareReset();
void hardwareShutdown();
void hardwareBigTick(const long);
void hardwareTick(const long, const long);

byte hardwareInVBL(byte);
void hardwareSetTrace(int val);
void hardwareRaiseNMI(void);
void hardwareHandleWDM(byte val);
