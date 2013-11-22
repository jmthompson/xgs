// -P-
#include <windows.h>
#include <mmsystem.h>

#define SOUNDBUFFERS 5

typedef struct {
	HANDLE hData;
	HPSTR lpData;
	HGLOBAL hWaveHdr;
	LPWAVEHDR lpWaveHdr;
	int busy;
} soundinfo;
// -P-

int SND_outputInit(int);
void SND_outputShutdown(void);
size_t SND_outputWrite(snd_sample_struct *, size_t);
