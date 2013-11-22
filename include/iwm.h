#ifndef _IWM_H_
#define _IWM_H_
#define NUM_IWM_DEVS	4

#define AVG_NIB_AREA_525	(2*0x1900)
#define LEN_NIB_AREA_525	(2*0x2000)

#define LEN_NIB_IMG_TRK_525	6656

#define LEN_NIB_SECTOR_35	(2*800)

typedef struct {
	int	track_valid;
	int	track_dirty;
	int	overflow_size;
	int	track_len;
	byte	*nib_area;
} Track;

typedef struct {
	disk_struct	*disk;
	int		cur_qtr_track;
	int		vol_num;
	word32		time_last_read;
	int		last_phase;
	int		nib_pos;
	Track		track[35*4];
} Disk525;

typedef struct {
	disk_struct	*disk;
	int		motor_on;
	int		cur_track;
	int		disk_switched;
	int		step;
	int		head;
	int		nib_pos;
	Track		track[80*2];
} Disk35;

extern int	iwm_slot4_motor;
extern int	iwm_slot5_motor;
extern int	iwm_slot6_motor;
extern int	iwm_slot7_motor;

int		IWM_init(void);
void		IWM_shutdown(void);
void		IWM_reset(void);
void		IWM_update(void);
int		IWM_loadDrive(int, int, char *);
int		IWM_unloadDrive(int, int);
byte		IWM_getDiskReg(byte);
byte		IWM_setDiskReg(byte);
void		IWM_showStats(void);
void		IWM_touchSwitches(int);
void		IWM_phaseChange525(int, int);
int		IWM_readStatus35(void);
void		IWM_doAction35(void);
byte		IWM_readLoc(int);
byte		IWM_readC0E0(byte);
byte		IWM_readC0E1(byte);
byte		IWM_readC0E2(byte);
byte		IWM_readC0E3(byte);
byte		IWM_readC0E4(byte);
byte		IWM_readC0E5(byte);
byte		IWM_readC0E6(byte);
byte		IWM_readC0E7(byte);
byte		IWM_readC0E8(byte);
byte		IWM_readC0E9(byte);
byte		IWM_readC0EA(byte);
byte		IWM_readC0EB(byte);
byte		IWM_readC0EC(byte);
byte		IWM_readC0ED(byte);
byte		IWM_readC0EE(byte);
byte		IWM_readC0EF(byte);
byte		IWM_writeLoc(int, int);
byte		IWM_writeC0E0(byte);
byte		IWM_writeC0E1(byte);
byte		IWM_writeC0E2(byte);
byte		IWM_writeC0E3(byte);
byte		IWM_writeC0E4(byte);
byte		IWM_writeC0E5(byte);
byte		IWM_writeC0E6(byte);
byte		IWM_writeC0E7(byte);
byte		IWM_writeC0E8(byte);
byte		IWM_writeC0E9(byte);
byte		IWM_writeC0EA(byte);
byte		IWM_writeC0EB(byte);
byte		IWM_writeC0EC(byte);
byte		IWM_writeC0ED(byte);
byte		IWM_writeC0EE(byte);
byte		IWM_writeC0EF(byte);

int		IWM_readEnable2(void);
int		IWM_readEnable2Handshake(void);
void		IWM_writeEnable2(int);

int		IWM_readData525(void);
void		IWM_writeData525(int);
void		IWM_nibblize525(byte *, byte *);
int		IWM_unnib525(void);
int		IWM_unnib525_4x4(void);
int		IWM_trackToUnix525(Disk525 *, Track *, int, byte *);
void		IWM_unixToNib525(Disk525 *, int);
void		IWM_4x4nibOut525(Disk525 *, Track *, word32);
void		IWM_nibOut525(Disk525 *, Track *, byte, int);

int		IWM_readData35(void);
void		IWM_writeData35(int);
void		IWM_nibblize35(byte *, byte *, byte *);
int		IWM_unnib35(void);
int		IWM_trackToUnix35(Disk35 *, Track *, int, int, byte *);
void		IWM_unixToNib35(Disk35 *, int);
void		IWM_4x4nibOut35(Disk35 *, Track *, word32);
void		IWM_nibOut35(Disk35 *, Track *, byte, int);
#endif
