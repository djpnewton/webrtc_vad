// testvad.cpp : Defines the entry point for the console application.
//

#include "mic.h"
#include "../webrtc/common_audio/vad/include/webrtc_vad.h"

#include <windows.h> 
#include <stdio.h> 
#include "tchar.h"

#define SAMPLE_RATE (48000)
#define SAMPLE_LENGTH_MS (30)
#define SAMPLE_COUNT (SAMPLE_RATE / 1000 * SAMPLE_LENGTH_MS)

bool keepRunning = true;
BOOL CtrlHandler(DWORD fdwCtrlType)
{
    keepRunning = false;
    return TRUE;
}

void mic_data(const int16_t* values, int count, void* param)
{
    VadInst* vad = (VadInst*)param;
    int res = WebRtcVad_Process(vad, SAMPLE_RATE, values, count);
    if (res == 1)
        printf("voice\n");
    else if (res == -1)
        printf("error\n");
    else
        printf(".");
}

int _tmain(int argc, _TCHAR* argv[])
{
    if (!SetConsoleCtrlHandler((PHANDLER_ROUTINE)CtrlHandler, TRUE))
    {
        printf("\nERROR: Could not set control handler");
        return 1;
    }

    // create VAD
    VadInst* vad = WebRtcVad_Create();
    if (!vad)
        return 1;
    if (WebRtcVad_Init(vad) != 0)
        return 1;
    if (WebRtcVad_set_mode(vad, 3) != 0)
        return 1;

    // create mic
    CMic::MicInit();
    CMic mic;
    mic.Init(mic_data, vad, SAMPLE_RATE, SAMPLE_COUNT);
    // start mic loop
    if (mic.Start())
    {
        while (keepRunning)
            Pa_Sleep(1000);
        mic.Stop();
    }
    CMic::MicFree();

    WebRtcVad_Free(vad);

	return 0;
}

