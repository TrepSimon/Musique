

#define _USE_MATH_DEFINES

#include <iostream>
#include <Windows.h>
#include <mmdeviceapi.h>
#include <Audioclient.h>
#include <math.h>
#include <vector>
#include <queue>
#include "Fenetre.h"

HWND sliderWindow;

template <class T> void releaseBuffer(T** ppt) {
    if (*ppt) {
        (*ppt)->Release();
        *ppt = NULL;
    }
}

HWND onCreate(HWND window) {
    sliderWindow = CreateWindowExA(0, "TRACKBAR_CLASS", "musique", WS_VISIBLE | WS_BORDER | WS_CHILD, 0, 0, 200, 100, window, NULL, NULL, NULL);
    
    return NULL;
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

    WAVEFORMATEX* pwfCapture = NULL;
    WAVEFORMATEX* pwfRender = NULL;

    audioClient->GetMixFormat(&pwfCapture);
    renderAudioClient->GetMixFormat(&pwfRender);

    REFERENCE_TIME bufferDuration = 0, requestDuration = 0;

    audioClient->GetDevicePeriod(&bufferDuration, &requestDuration);

    bufferDuration *= 4;

    audioClient->Initialize(
        AUDCLNT_SHAREMODE_SHARED,
        AUDCLNT_STREAMFLAGS_EVENTCALLBACK,
        bufferDuration,
        0,
        pwfCapture,
        NULL
    );

    hr = renderAudioClient->Initialize(
        AUDCLNT_SHAREMODE_SHARED,
        0,
        bufferDuration,
        0,
        pwfRender,
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

    float phase = 0;
    float samplingRate = (float)pwfRender->nSamplesPerSec;
    float currentFrequency = 120;

    std::queue<float> delayedData = std::queue<float>();

    app::Fenetre* window = new app::Fenetre();

    *window->getRunning() = window->create_window(500, 500, "musique");

    while(*window->getRunning()){
        window->update_window();
        DWORD waitResult = WaitForSingleObject(hEvent, INFINITE);
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

                UINT32 framesToWrite = min(numFramesAvailable, availableRenderSpace);
                renderClient->GetBuffer(framesToWrite, &renderData);

                if (flag & AUDCLNT_BUFFERFLAGS_SILENT) {
                    memset(renderData, 0, numFramesAvailable * pwfCapture->nBlockAlign);
                    continue;
                }

                float* input = (float*)data;
                float* output = (float*)renderData;

                UINT32 channels = pwfCapture->nChannels;

                int frameCount = 0;

                for (UINT32 idx = 0; idx < framesToWrite * channels; idx++) {
                    //oscillateur
                    float sample = sin(phase);
                    phase += (2.0f * M_PI * currentFrequency) / samplingRate;

                    if (phase > 2 * M_PI)phase -= 2 * M_PI;



                    //float sample = input[idx];

                    //delayedData.push(sample);

                    //amplificateur
                    //sample *= drive;

                    //soft clip
                    //sample = std::tanh(sample * drive);

                    //hardclip
                    //sample *= drive;

                    /*if (sample > 1)sample = 1;
                    else if (sample < -1)sample = -1;*/

                    //waveshaping
                    /*sample *= drive;
                    sample = sample / (1 + fabs(sample));*/

                    //overdrive
                    /*sample *= drive;
                    if (sample > 0)sample = 1 - exp(-sample);
                    else sample = -1 + exp(sample);*/

                    //delay
                    /*if (frameCount >= waitFrames) {
                        float delayedSample = delayedData.front();
                        delayedData.pop();
                        sample += delayedSample;
                    }
                    else frameCount++;*/

                    output[idx] = sample;
                }

                renderClient->ReleaseBuffer(numFramesAvailable, 0);
            }
            captureCLient->ReleaseBuffer(numFramesAvailable);
            captureCLient->GetNextPacketSize(&packetLength);
        }
    }

    CloseHandle(hEvent);
    CoTaskMemFree(pwfCapture);
    releaseBuffer(&audioClient);
    releaseBuffer(&renderAudioClient);
    CoUninitialize();
}


