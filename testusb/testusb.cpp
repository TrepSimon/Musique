

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

    hr = CoCreateInstance(
        __uuidof(MMDeviceEnumerator),
        nullptr,
        CLSCTX_ALL,
        __uuidof(IMMDeviceEnumerator),
        (void**)&enumerator
    );

    hr = enumerator->GetDefaultAudioEndpoint(
        eRender, //changer en eCapture
        eConsole,
        &device
        );

    hr = device->Activate(
        __uuidof(IAudioClient),
        CLSCTX_ALL,
        nullptr,
        (void**)&audioClient
    );

    WAVEFORMATEX* pwfx = NULL;
    audioClient->GetMixFormat(&pwfx);

    audioClient->Initialize(
        AUDCLNT_SHAREMODE_SHARED,
        AUDCLNT_STREAMFLAGS_LOOPBACK, //enlever ca
        10000000,
        0,
        pwfx,
        NULL
    );

    IAudioCaptureClient* captureCLient = nullptr;

    audioClient->GetService(
        __uuidof(IAudioCaptureClient),
        (void**)&captureCLient
    );

    audioClient->Start();

    while(true){
        UINT32 packetLength = 0;
        captureCLient->GetNextPacketSize(&packetLength);

        while(packetLength != 0){
            BYTE* data;
            UINT32 numFramesAvailable;
            DWORD flag = 0;

            captureCLient->GetBuffer(&data, &numFramesAvailable, &flag, nullptr, nullptr);

            float* sample = (float*)data;

            float total = 0;

            for (UINT32 idx = 0; idx < numFramesAvailable; idx++) {
                //std::cout << sample[idx] << std::endl;
                total += sample[idx];
            }
            std::cout << total << std::endl;
            captureCLient->ReleaseBuffer(numFramesAvailable);
            captureCLient->GetNextPacketSize(&packetLength);
        }
        Sleep(10);
    }
}


