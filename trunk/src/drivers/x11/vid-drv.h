#define VID_WIN_WIDTH	720
#define VID_WIN_HEIGHT	480

extern int	vid_shm;
extern int	vid_mapped;

extern PIXEL	*vid_lines[];

extern PIXEL vid_textfg,vid_textbg,vid_border;

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
