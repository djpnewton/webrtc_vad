#include "mic.h"

#include <stdio.h>

#define _USE_MATH_DEFINES
#include <math.h>
#include <assert.h>

#pragma comment(lib, "Winmm.lib")

DWORD WINAPI CMic::waveInProc(LPVOID arg)
{
    CMic* mic = (CMic*)arg;

    MSG msg;
    while (GetMessage(&msg, 0, 0, 0) == 1)
    {
        switch (msg.message)
        {
        default:
            break;
        case WIM_CLOSE:
            return 0;
        case WIM_DATA:
            PWAVEHDR waveHeader = (PWAVEHDR)msg.lParam;

            int16_t* in = (int16_t*)waveHeader->lpData;
            unsigned long frame_count = waveHeader->dwBytesRecorded / mic->wave_format.nBlockAlign;
            if (mic->mic_cb)
                mic->mic_cb(in, frame_count, mic->mic_cb_param);

            // check if mic is shutting down
            if (mic->stopping)
                break;
            // queue another buffer
            waveInAddBuffer(mic->hWaveIn, waveHeader, sizeof(WAVEHDR));
            break;
        }
    }

    return 0;
}

bool CMic::Init(mic_output_t mic_cb, void* param, DWORD wavein_id, int sample_rate, int frames_per_buffer)
{
    this->mic_cb = mic_cb;
    this->mic_cb_param = param;

    this->waveId = wavein_id;

    this->sample_rate = sample_rate;
    this->frames_per_buffer = frames_per_buffer;

    memset(&wave_format, 0, sizeof(wave_format));
    wave_format.wFormatTag = WAVE_FORMAT_PCM;
    wave_format.nChannels = 1; /* mono input */
    wave_format.nSamplesPerSec = sample_rate;
    wave_format.wBitsPerSample = 16;
    wave_format.nBlockAlign = (wave_format.nChannels * wave_format.wBitsPerSample) / 8;
    wave_format.nAvgBytesPerSec = wave_format.nSamplesPerSec * wave_format.nBlockAlign;
    wave_format.cbSize = 0;

    initialized = true;
    return true;
}

bool CMic::Start()
{
    if (stream_open)
        return false;

    stopping = false;
    waveInThread = NULL;
    hWaveIn = NULL;

    DWORD threadId;
    waveInThread = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)waveInProc, this, 0, &threadId);
    if (!waveInThread)
        goto cleanup;

    MMRESULT res = waveInOpen(&hWaveIn, waveId, &wave_format, threadId, 0, CALLBACK_THREAD);
    if (res != MMSYSERR_NOERROR)
        goto cleanup;

    ZeroMemory(waveHeader, sizeof(WAVEHDR) * 2);
    waveHeader[0].dwBufferLength = waveHeader[1].dwBufferLength = frames_per_buffer * wave_format.nBlockAlign;
    waveHeader[0].lpData = (LPSTR)malloc(waveHeader[0].dwBufferLength);
    waveHeader[1].lpData = (LPSTR)malloc(waveHeader[1].dwBufferLength);
    if (!waveHeader[0].lpData || !waveHeader[1].lpData)
        goto cleanup;
    res = waveInPrepareHeader(hWaveIn, &waveHeader[0], sizeof(WAVEHDR));
    if (res != MMSYSERR_NOERROR)
        goto cleanup;
    res = waveInPrepareHeader(hWaveIn, &waveHeader[1], sizeof(WAVEHDR));
    if (res != MMSYSERR_NOERROR)
        goto cleanup;
    res = waveInAddBuffer(hWaveIn, &waveHeader[0], sizeof(WAVEHDR));
    if (res != MMSYSERR_NOERROR)
        goto cleanup;
    res = waveInAddBuffer(hWaveIn, &waveHeader[1], sizeof(WAVEHDR));
    if (res != MMSYSERR_NOERROR)
        goto cleanup;

    res = waveInStart(hWaveIn);
    if (res != MMSYSERR_NOERROR)
        goto cleanup;

    stream_open = true;
    return true;

cleanup:
    if (waveHeader[0].lpData)
        free(waveHeader[0].lpData);
    if (waveHeader[1].lpData)
        free(waveHeader[1].lpData);
    if (waveInThread)
        CloseHandle(waveInThread);
    if (hWaveIn)
        waveInClose(hWaveIn);

    return false;
}

bool CMic::Stop()
{
    if (!stream_open)
        return false;

    //TODO: error handling
    stopping = true;
    MMRESULT res = waveInReset(hWaveIn);
    while (res = waveInUnprepareHeader(hWaveIn, &waveHeader[0], sizeof(WAVEHDR)) == WAVERR_STILLPLAYING)
        Sleep(10);
    while (res = waveInUnprepareHeader(hWaveIn, &waveHeader[1], sizeof(WAVEHDR)) == WAVERR_STILLPLAYING)
        Sleep(10);
    while (res = waveInClose(hWaveIn) == WAVERR_STILLPLAYING)
        Sleep(10);

    WaitForSingleObject(waveInThread, INFINITE);
    CloseHandle(waveInThread);
    free(waveHeader[1].lpData);
    free(waveHeader[0].lpData);

    stream_open = false;
    return true;
}
