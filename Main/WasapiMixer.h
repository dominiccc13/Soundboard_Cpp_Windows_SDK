#pragma once
#define NOMINMAX
#include <windows.h>
#include <audioclient.h>
#include <mmdeviceapi.h>
#include <functiondiscoverykeys_devpkey.h>
#include <algorithm>
#include <vector>
#include <atomic>
#include <thread>
#include <iostream>
#include <string>

const int g_soundbiteCount = 20;

class WasapiMixer {
    public:
    WasapiMixer() = default;
    ~WasapiMixer();
    void SetMicGain(float gain) { micGain.store(gain); }
    void SetWavGain(float gain) { wavGain.store(gain); }
    void LoadSoundbite(int index, const std::vector<float>& floatPcmData);
    void TriggerSoundbite(int index);
    bool Start(const wchar_t* targetRenderDeviceName);
    void Stop(); 

private:
    void MixLoop();
    
    std::thread workerThread;

    IAudioClient* captureAudioClient = nullptr;
    IAudioCaptureClient* captureClient = nullptr;
    IAudioClient* renderAudioClient = nullptr;
    IAudioRenderClient* renderClient = nullptr;

    std::atomic<bool> isRunning{false};
    std::atomic<bool> isPlayingWav{false};
    std::atomic<int> wavBufferIndex{-1};
    std::atomic<size_t> wavReadIndex{0};
    std::vector<float> wavBuffers[g_soundbiteCount];
    
    std::atomic<float> micGain{2.4f};
    std::atomic<float> wavGain{0.2f};

    UINT32 captureSampleRate = 0;
    UINT32 captureChannels = 0;
    WORD captureBlockAlign = 0;
    WORD captureBitsPerSample = 0;
    
    UINT32 renderSampleRate = 0;
    WORD renderChannels = 0;
    WORD renderBlockAlign = 0;
    WORD renderBitsPerSample = 0;
};