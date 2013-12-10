extern int	emul_period;
extern long emul_tick;
extern long emul_microtick;
extern int	emul_vbl_count;
extern int	emul_vbl;
extern int	emul_vblirq;
extern int	emul_qtrsecirq;
extern int	emul_onesecirq;
extern int	emul_scanirq;
extern int	emul_speed;
extern int	emul_speed2;
extern int	emul_delay;

extern long emul_target_cycles;
extern double	emul_target_speed;

extern char	*emul_path;

long EMUL_getCurrentTime(void);
char *EMUL_expandPath(const char *);

void EMUL_doVBL(void);
void EMUL_hardwareUpdate(word32);
int EMUL_init(int, char **);
void EMUL_run(void);
void EMUL_reset(void);
void EMUL_shutdown(void);
void EMUL_trace(int val);
void EMUL_nmi(void);
void EMUL_handleWDM(byte val);
byte EMUL_getVBL(byte val);
void EMUL_displayUsage(char *);
