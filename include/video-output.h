extern int	vid_shm;
extern int	vid_mapped;

int	VID_outputInit(void);
void	VID_outputImage(void);
void	VID_outputShutdown(void);
void	VID_outputStatus1(char *);
void	VID_outputStatus2(char *);
void	VID_outputResize(int, int);
void	VID_outputSetStandardColors(void);
void	VID_outputSetSuperColors(void);
void	VID_outputSetTextColors(void);
void	VID_outputSetBorderColor(void);
