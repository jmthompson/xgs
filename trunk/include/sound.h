#define IRQ_STACK_SIZE      256
#define SAMPLE_RATE		    28000
#define OUTPUT_BUFFER_SIZE	2048

extern int snd_enable;

typedef struct {
	int	left,right;
} snd_sample_struct;

/* Function prototypes */

int  soundInit(void);
void soundReset(void);
void soundShutdown(void);
void soundUpdate(void);

byte soundClickSpeaker(byte);

byte SND_readSoundCtl(byte);
byte SND_readSoundData(byte);
byte SND_readSoundAddrL(byte);
byte SND_readSoundAddrH(byte);

byte SND_writeSoundCtl(byte);
byte SND_writeSoundData(byte);
byte SND_writeSoundAddrL(byte);
byte SND_writeSoundAddrH(byte);
