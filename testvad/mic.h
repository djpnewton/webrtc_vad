#pragma once

#include "../portaudio/include/portaudio.h"
#include <list>
#include <stdint.h>

typedef void(*mic_output_t)(const int16_t* values, int count, void* param);

class CMic
{
private:
    static bool pa_init;

    bool initialized;
    PaStreamParameters input_parameters;
    bool stream_open;
    PaStream* stream;
    int sample_rate;
    int frames_per_buffer;

    mic_output_t mic_cb;
    void* mic_cb_param;

    static int pa_callback(const void* input, void* output,
        unsigned long frame_count,
        const PaStreamCallbackTimeInfo* time_info,
        PaStreamCallbackFlags status_flags,
        void *user_data);

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

    bool Init(mic_output_t mic_cb, void* param, int sample_rate, int frames_per_buffer);
    bool Start();
    bool Stop();

    static bool MicInit()
    {
        if (!pa_init)
        {
            PaError err = Pa_Initialize();
            if (err == paNoError)
                pa_init = true;
        }
        return pa_init;
    }
    static bool MicFree()
    {
        if (pa_init)
        {
            PaError err = Pa_Terminate();
            if (err == paNoError)
                pa_init = false;
        }
        return !pa_init;
    }
};