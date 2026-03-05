

#include <iostream>
#include <Windows.h>
#include <mmdeviceapi.h>
#include <Audioclient.h>

#pragma comment(lib, "ole32.lib")

int main()
{
    HRESULT hr;

    hr = CoInitialize(nullptr);

    IMMDeviceEnumerator* enumerator = NULL;
    IMMDevice* device = NULL;
    IAudioClient* audioClient = NULL;
    IAudioClient* renderAudioClient = NULL;

    hr = CoCreateInstance(
        __uuidof(MMDeviceEnumerator),
        nullptr,
        CLSCTX_ALL,
        __uuidof(IMMDeviceEnumerator),
        (void**)&enumerator
    );

    hr = enumerator->GetDefaultAudioEndpoint(
        eCapture, //changer en eCapture ou eRender
        eConsole,
        &device
        );

    hr = device->Activate(
        __uuidof(IAudioClient),
        CLSCTX_ALL,
        nullptr,
        (void**)&audioClient
    );

    hr = device->Activate(
        __uuidof(IAudioClient),
        CLSCTX_ALL,
        nullptr,
        (void**)&renderAudioClient
    );

    WAVEFORMATEX* pwfx = NULL;
    audioClient->GetMixFormat(&pwfx);

    audioClient->Initialize(
        AUDCLNT_SHAREMODE_SHARED,
        AUDCLNT_STREAMFLAGS_LOOPBACK,//enlever ca ou AUDCLNT_STREAMFLAGS_LOOPBACK
        10000000,
        0,
        pwfx,
        NULL
    );

    renderAudioClient->Initialize(
        AUDCLNT_SHAREMODE_SHARED,
        0,
        10000000,
        0,
        pwfx,
        NULL
    );

    IAudioCaptureClient* captureCLient = nullptr;
    IAudioRenderClient* renderClient = nullptr;

    audioClient->GetService(
        __uuidof(IAudioCaptureClient),
        (void**)&captureCLient
    );

    renderAudioClient->GetService(
        __uuidof(IAudioRenderClient),
        (void**)&renderClient
    );

    audioClient->Start();
    renderAudioClient->Start();

    while(true){
        UINT32 packetLength = 0;
        captureCLient->GetNextPacketSize(&packetLength);

        while(packetLength != 0){
            BYTE* data;
            BYTE* renderData;
            UINT32 numFramesAvailable;
            DWORD flag = 0;

            captureCLient->GetBuffer(&data, &numFramesAvailable, &flag, nullptr, nullptr);
            renderClient->GetBuffer(numFramesAvailable, &renderData);

            float* input = (float*)data;
            float* output = (float*)renderData;

            UINT32 channels = pwfx->nChannels;

            for (UINT32 idx = 0; idx < numFramesAvailable; idx++) {
                for (UINT32 channel = 0; channel < channels; channel++) {
                    float sample = input[idx * channels + channel];

                    sample *= 2;

                    output[idx * channels + channel] = sample;
                }

            }

            renderClient->ReleaseBuffer(numFramesAvailable, 0);
            captureCLient->ReleaseBuffer(numFramesAvailable);
            captureCLient->GetNextPacketSize(&packetLength);
        }
        Sleep(10);
    }
}


