#include "mic.h"

#include <stdio.h>

#define _USE_MATH_DEFINES
#include <math.h>

bool CMic::pa_init = false;

int CMic::pa_callback(const void* input, void* output,
    unsigned long frame_count,
    const PaStreamCallbackTimeInfo* time_info,
    PaStreamCallbackFlags status_flags,
    void *user_data)
{
    int16_t* in = (int16_t*)input;
    CMic* mic = (CMic*)user_data;

    // call mic output callback
    if (mic->mic_cb)
        mic->mic_cb(in, frame_count, mic->mic_cb_param);

    return paContinue;
}

bool CMic::Init(mic_output_t mic_cb, void* param, int sample_rate, int frames_per_buffer)
{
    this->mic_cb = mic_cb;
    this->mic_cb_param = param;
    this->sample_rate = sample_rate;
    this->frames_per_buffer = frames_per_buffer;

    input_parameters.device = Pa_GetDefaultInputDevice();
    if (input_parameters.device == paNoDevice)
        return false;

    input_parameters.channelCount = 1;       /* mono input */
    input_parameters.sampleFormat = paInt16;
    input_parameters.suggestedLatency = Pa_GetDeviceInfo(input_parameters.device)->defaultLowInputLatency;
    input_parameters.hostApiSpecificStreamInfo = nullptr;

    initialized = true;
    return true;
}

bool CMic::Start()
{
    PaError err;

    if (stream_open)
        return false;

    err = Pa_OpenStream(&stream, &input_parameters, nullptr, this->sample_rate, this->frames_per_buffer, paClipOff, CMic::pa_callback, this);
    if (err != paNoError)
        return false;

    err = Pa_StartStream(stream);
    if (err != paNoError)
        return false;

    stream_open = true;
    return true;
}

bool CMic::Stop()
{
    PaError err;

    if (!stream_open)
        return false;

    err = Pa_CloseStream(stream);
    if (err != paNoError)
        return false;

    stream_open = false;
    return true;
}