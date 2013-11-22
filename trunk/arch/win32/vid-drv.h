#define VID_WIN_WIDTH	800
#define VID_WIN_HEIGHT	600

extern int	vid_out_width,vid_out_height,vid_out_x,vid_out_y;

extern PIXEL	*vid_lines[];

int		VID_outputInit(void);
void	VID_outputImage(void);
void	VID_outputShutdown();
void	VID_outputStatus1(char *);
void	VID_outputStatus2(char *);
void	VID_outputResize(int, int);
void	VID_outputSetStandardColors();
void	VID_outputSetSuperColors();
void	VID_outputSetTextColors();
void	VID_outputSetBorderColor();
