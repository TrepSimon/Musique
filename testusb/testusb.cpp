

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
    IMMDevice* renderDevice = NULL;
    IMMDevice* captureDevice = NULL;
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
        eRender, 
        eConsole,
        &renderDevice
        );

    hr = enumerator->GetDefaultAudioEndpoint(
        eCapture,
        eConsole,
        &captureDevice
    );

    hr = captureDevice->Activate(
        __uuidof(IAudioClient),
        CLSCTX_ALL,
        nullptr,
        (void**)&audioClient
    );

    hr = renderDevice->Activate(
        __uuidof(IAudioClient),
        CLSCTX_ALL,
        nullptr,
        (void**)&renderAudioClient
    );

    WAVEFORMATEX* captureFormat = NULL;
    WAVEFORMATEX* renderFormat = NULL;

    audioClient->GetMixFormat(&captureFormat);
    renderAudioClient->GetMixFormat(&renderFormat);

    audioClient->Initialize(
        AUDCLNT_SHAREMODE_SHARED,
        0,
        10000000,
        0,
        captureFormat,
        NULL
    );

    renderAudioClient->Initialize(
        AUDCLNT_SHAREMODE_SHARED,
        0,
        10000000,
        0,
        renderFormat,
        NULL
    );

    IAudioCaptureClient* captureCLient = nullptr;
    IAudioRenderClient* renderClient = nullptr;

    hr = audioClient->GetService(
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

            if (flag & AUDCLNT_BUFFERFLAGS_SILENT)memset(renderData, 0, numFramesAvailable * renderFormat->nBlockAlign);

            float* input = (float*)data;
            float* output = (float*)renderData;

            UINT32 channels = captureFormat->nChannels;

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


