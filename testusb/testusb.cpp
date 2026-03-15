

#include <iostream>
#include <Windows.h>
#include <mmdeviceapi.h>
#include <Audioclient.h>
#include <math.h>
#include <vector>
#include <queue>

template <class T> void releaseBuffer(T** ppt) {
    if (*ppt) {
        (*ppt)->Release();
        *ppt = NULL;
    }
}

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

    WAVEFORMATEX* pwf = NULL;

    audioClient->GetMixFormat(&pwf);

    audioClient->Initialize(
        AUDCLNT_SHAREMODE_SHARED,
        AUDCLNT_STREAMFLAGS_EVENTCALLBACK,
        100000,
        0,
        pwf,
        NULL
    );

    hr = renderAudioClient->Initialize(
        AUDCLNT_SHAREMODE_SHARED,
        0,
        100000,
        0,
        pwf,
        NULL
    );

    if (FAILED(hr)) {
        std::cout << "failed\n";
        std::cout << "Initialize error: 0x" << std::hex << hr << std::endl;
    }

    HANDLE hEvent = CreateEvent(NULL, false, false, NULL);
    audioClient->SetEventHandle(hEvent);

    IAudioCaptureClient* captureCLient = nullptr;
    IAudioRenderClient* renderClient = nullptr;

    hr = audioClient->GetService(
        __uuidof(IAudioCaptureClient),
        (void**)&captureCLient
    );

    hr = renderAudioClient->GetService(
        __uuidof(IAudioRenderClient),
        (void**)&renderClient
    );

    audioClient->Start();
    renderAudioClient->Start();

    float drive = 10;
    int waitFrames = 10000;

    while(true){
        DWORD waitResult = WaitForSingleObject(hEvent, 1000);
        if (waitResult != WAIT_OBJECT_0)break;

        UINT32 packetLength = 0;
        captureCLient->GetNextPacketSize(&packetLength);

        while(packetLength != 0){
            BYTE* data;
            UINT32 numFramesAvailable;
            DWORD flag = 0;
            UINT32 padding, bufferLenght, availableRenderSpace;

            renderAudioClient->GetCurrentPadding(&padding);
            renderAudioClient->GetBufferSize(&bufferLenght);

            availableRenderSpace = bufferLenght - padding;

            captureCLient->GetBuffer(&data, &numFramesAvailable, &flag, nullptr, nullptr);

            if(numFramesAvailable <= (bufferLenght - padding)){
                BYTE* renderData;

                renderClient->GetBuffer(numFramesAvailable, &renderData);

                if (flag & AUDCLNT_BUFFERFLAGS_SILENT)memset(renderData, 0, numFramesAvailable * pwf->nBlockAlign);

                float* input = (float*)data;
                float* output = (float*)renderData;

                UINT32 channels = pwf->nChannels;

                std::queue<float> delayedData = std::queue<float>();
                int frameCount = 0;

                for (UINT32 idx = 0; idx < numFramesAvailable * channels; idx++) {
                    float sample = input[idx];
                    delayedData.push(sample);

                    //amplificateur
                    //sample *= drive;

                    //soft clip
                    //sample = std::tanh(sample * drive);

                    //hardclip
                    /*sample *= drive;

                    if (sample > 1)sample = 1;
                    else if (sample < -1)sample = -1;*/

                    //waveshaping
                    /*sample *= drive;
                    sample = sample / (1 + fabs(sample));*/

                    //overdrive
                    /*sample *= drive;
                    if (sample > 0)sample = 1 - exp(-sample);
                    else sample = -1 + exp(sample);*/

                    //delay
                    if (frameCount >= waitFrames) {
                        float delayedSample = delayedData.front();
                        delayedData.pop();
                        sample += delayedSample;
                    }
                    else frameCount++;

                    output[idx] = sample;
                }

                renderClient->ReleaseBuffer(numFramesAvailable, 0);
            }
            captureCLient->ReleaseBuffer(numFramesAvailable);
            captureCLient->GetNextPacketSize(&packetLength);
        }
    }

    CloseHandle(hEvent);
    CoTaskMemFree(pwf);
    releaseBuffer(&audioClient);
    releaseBuffer(&renderAudioClient);
    CoUninitialize();
}


