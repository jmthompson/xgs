#define VID_WIDTH	640
#define VID_HEIGHT	400

#define VID_STATUS_FONT	"8x13"

#define PIXEL word32

#define VID_DEPTH 24
#define VID_CLASS TrueColor

#define VID_MODE_40TEXT1	0
#define VID_MODE_40TEXT2	1
#define VID_MODE_80TEXT1	2
#define VID_MODE_80TEXT2	3

#define VID_MODE_LORES1		4
#define VID_MODE_LORES2		5
#define VID_MODE_DLORES1	6
#define VID_MODE_DLORES2	7

#define VID_MODE_HIRES1		8
#define VID_MODE_HIRES2		9
#define VID_MODE_DHIRES1	10
#define VID_MODE_DHIRES2	11

#define VID_MODE_SUPER		12

extern PIXEL	vid_black,vid_white;
extern byte	*vid_font40[2];
extern byte	*vid_font80[2];
extern int	vid_currmode;

extern int	vid_super;
extern int	vid_linear;
extern int	vid_a2mono;

extern int	vid_bordercolor;
extern int	vid_textfgcolor;
extern int	vid_textbgcolor;

extern PIXEL	*vid_lines[];

extern PIXEL vid_textfg,vid_textbg,vid_border,vid_stdcolors[16],vid_supercolors[256];

extern int	vid_80col;
extern int	vid_altcharset;

extern int	vid_text;
extern int	vid_mixed;
extern int	vid_page2;
extern int	vid_hires;
extern int	vid_dblres;

extern int	vid_vgcint;

extern int	vid_vert_cnt;
extern int	vid_horiz_cnt;

extern void	(*VID_updateRoutine)(void);

int VID_init(void);
void VID_update(void);
void VID_reset(void);
void VID_shutdown(void);
void VID_switchPage(int, int, int);
void VID_newMode(void);
void VID_redrawScreen(void);

byte VID_clear80col(byte);
byte VID_set80col(byte);
byte VID_get80col(byte);
byte VID_clearAltCh(byte);
byte VID_setAltCh(byte);
byte VID_getAltCh(byte);
byte VID_getColorReg(byte);
byte VID_setColorReg(byte);
byte VID_getVGCIntReg(byte);
byte VID_setVGCIntReg(byte);
byte VID_clearVGCInt(byte);
byte VID_getNewVideo(byte);
byte VID_setNewVideo(byte);
byte VID_getVertCnt(byte);
byte VID_getHorzCnt(byte);
byte VID_getBorder(byte);
byte VID_setBorder(byte);
byte VID_clearText(byte);
byte VID_setText(byte);
byte VID_getText(byte);
byte VID_clearMixed(byte);
byte VID_setMixed(byte);
byte VID_getMixed(byte);
byte VID_clearPage2(byte);
byte VID_setPage2(byte);
byte VID_getPage2(byte);
byte VID_clearHires(byte);
byte VID_setHires(byte);
byte VID_getHires(byte);
byte VID_clearDblRes(byte);
byte VID_setDblRes(byte);

void VID_refreshText40Page1(void);
void VID_refreshText40Page2(void);
void VID_refreshText80Page1(void);
void VID_refreshText80Page2(void);

void VID_refreshLoresPage1(void);
void VID_refreshLoresPage2(void);
void VID_refreshDLoresPage1(void);
void VID_refreshDLoresPage2(void);

void VID_refreshHiresPage1(void);
void VID_refreshHiresPage2(void);
void VID_refreshDHiresPage1(void);
void VID_refreshDHiresPage2(void);

void VID_refreshSuperHires(void);