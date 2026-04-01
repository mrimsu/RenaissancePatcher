#include "music.h"
#include "ibxm/ibxm.h"
#include "resources.h"
#include <mmsystem.h>

static int xm_to_wav( struct module *module, char *wav ) {
	int mix_buf[ 16384 ];
	int idx = 0, duration = 0, samples = 0, ampl = 0, offset = 0, length = 0;
	struct replay *replay = new_replay( module, 8000, 0 );
	if( replay ) {
		duration = replay_calculate_duration( replay );
		length = duration;
		if( wav ) {
			replay_seek( replay, 0 );
			offset = 0;
			while( offset < length ) {
				samples = replay_get_audio( replay, mix_buf, 0 );
				for( idx = 0; idx < samples; idx++ ) {
					ampl = mix_buf[ idx ];
					if( ampl > 32767 ) {
						ampl = 32767;
					}
					if( ampl < -32768 ) {
						ampl = -32768;
					}
					wav[ offset++ ] = ampl & 0xFF;
					wav[ offset++ ] = ( ampl >> 8 ) & 0xFF;
				}
			}
		}
		dispose_replay( replay );
	}
	return length;
}

BOOL WINAPI InitModPlay(HWND Wnd) {
    UNREFERENCED_PARAMETER(Wnd);
    HRSRC ResourceHandle = FindResourceW(NULL, MAKEINTRESOURCE(IDR_RCDATA3), RT_RCDATA);

    if (!ResourceHandle) return FALSE;

    DWORD TrackResSize = SizeofResource(NULL, ResourceHandle);
    HGLOBAL TrackResource = LoadResource(NULL, ResourceHandle);
    PVOID TrackResBuffer = LockResource(TrackResource);

    struct data XmData;

    XmData.buffer = (PCHAR) TrackResBuffer;
    XmData.length = TrackResSize;

    CHAR XmErrorMessage[64] = "";

	struct module *XmTrack = module_load(&XmData, XmErrorMessage);

	INT Length = xm_to_wav(XmTrack, NULL);
	
    PCHAR Buffer = NULL;

    HWAVEOUT WaveOut = 0;
    WAVEFORMATEX Waveformat;

	Waveformat.wFormatTag = WAVE_FORMAT_PCM;
	Waveformat.cbSize = 0;
	Waveformat.nChannels = 2;
	Waveformat.nSamplesPerSec = 16000;
	Waveformat.wBitsPerSample = 8;
	Waveformat.nBlockAlign = (Waveformat.wBitsPerSample * Waveformat.nChannels) / 8;
	Waveformat.nAvgBytesPerSec = Waveformat.nSamplesPerSec * Waveformat.nBlockAlign * Waveformat.nChannels / 8;

    Length = Waveformat.nSamplesPerSec * Waveformat.nChannels * 50;
    
	waveOutOpen(&WaveOut, WAVE_MAPPER, &Waveformat, 0, 0, CALLBACK_NULL);

    WAVEHDR Header = {Buffer, (DWORD)Length, 0, 0, 0, 0, 0, 0 };

    waveOutPrepareHeader(WaveOut, &Header, sizeof(WAVEHDR));

    Buffer = (PCHAR)GlobalAlloc(GMEM_ZEROINIT, Length);

    xm_to_wav(XmTrack, Buffer);

    waveOutWrite(WaveOut, &Header, sizeof(WAVEHDR));

    waveOutUnprepareHeader(WaveOut, &Header, sizeof(WAVEHDR));
    waveOutClose(WaveOut);

    return TRUE;
}