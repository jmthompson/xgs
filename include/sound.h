#define IRQ_STACK_SIZE 256
#define SAMPLE_RATE		28000
#define OUTPUT_BUFFER_SIZE	2000

extern int snd_enable;

typedef struct {
	int	left,right;
} snd_sample_struct;

/* Function prototypes */

int  SND_init(void);
void SND_update(void);
void SND_reset(void);
void SND_shutdown(void);

byte SND_clickSpeaker(byte);
byte SND_readSoundCtl(byte);
byte SND_readSoundData(byte);
byte SND_readSoundAddrL(byte);
byte SND_readSoundAddrH(byte);

byte SND_writeSoundCtl(byte);
byte SND_writeSoundData(byte);
byte SND_writeSoundAddrL(byte);
byte SND_writeSoundAddrH(byte);

void SND_pushIRQ(int);
int  SND_pullIRQ(void);
void SND_enableOscillators(void);
void SND_updateOscillator(int);
void SND_scanOscillators(snd_sample_struct *);
void SND_updateClassicSound(snd_sample_struct *);

void SND_generateSamples(snd_sample_struct *, byte *, int);

extern word32 snd_click_sample;

extern snd_sample_struct *output_buffer;
extern int output_index;
extern int output_buffer_size;

/*
 * Provided by the configured sound driver
 */

int SND_outputInit(int);
void SND_outputShutdown(void);
