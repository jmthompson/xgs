#define ADB_IN_BUFFSIZE

int		ADB_inputInit(void);
void	ADB_inputShutdown(void);
void	ADB_inputUpdate(void);

void	ADB_inputMotionNotify(int, int);
void	ADB_inputKeyDown(int, int);
void	ADB_inputKeyRelease(int, int);
void	ADB_inputRightMouseDown(void);
void	ADB_inputRightMouseUp(void);
void	ADB_inputLeftMouseDown(void);
void	ADB_inputLeftMouseUp(void);
