#pragma once

#include <Windows.h>
#include <mmreg.h>
#include <list>
#include <stdint.h>

typedef void(*mic_output_t)(const int16_t* values, unsigned long count, void* param);

class CMic
{
private:
    bool initialized;

    DWORD waveId;
    int sample_rate;
    int frames_per_buffer;
    WAVEFORMATEX wave_format;
    bool stream_open;
    bool stopping;
    HWAVEIN hWaveIn;
    HANDLE waveInThread;
    WAVEHDR waveHeader[2];

    mic_output_t mic_cb;
    void* mic_cb_param;

    static DWORD WINAPI waveInProc(LPVOID arg);

public:

    CMic()
    {
        initialized = false;
        stream_open = false;
        mic_cb = nullptr;
    }

    ~CMic()
    {
    }

    bool Init(mic_output_t mic_cb, void* param, DWORD wavein_id, int sample_rate, int frames_per_buffer);
    bool Start();
    bool Stop();
};