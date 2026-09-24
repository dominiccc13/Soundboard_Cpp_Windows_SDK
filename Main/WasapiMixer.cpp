#include "WasapiMixer.h"

WasapiMixer::~WasapiMixer() {
    Stop();
}

void WasapiMixer::LoadSoundbite(int index, const std::vector<float>& floatPcmData) {
    // load soundbites into memory
    if (index >= 0 && index < g_soundbiteCount) {
        wavBuffers[index] = floatPcmData;
        wavReadIndex.store(0);
    }
}

void WasapiMixer::TriggerSoundbite(int index) {
    // add soundbite index to wavBufferIndex and set variables to trigger reading
    if (index < 0 || index >= g_soundbiteCount) return;
    wavBufferIndex.store(index);
    wavReadIndex.store(0);
    isPlayingWav.store(true);
}

bool WasapiMixer::Start(const wchar_t* targetRenderDeviceName) {
    IMMDeviceEnumerator* enumerator = nullptr;
    CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL, IID_PPV_ARGS(&enumerator));
    if (!enumerator) return false;

    // 1. get system default capture device  
    IMMDevice* micDevice = nullptr;
    enumerator->GetDefaultAudioEndpoint(eCapture, eConsole, &micDevice);
    if (!micDevice) {
        std::cerr << "Error: Could not get default Microphone endpoint.\n";
        enumerator->Release();
        return false;
    }

    // 2. get render device (must be vb-cable's device: "CABLE Output")
    IMMDeviceCollection* collection = nullptr;
    IMMDevice* renderDevice = nullptr;
    enumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &collection);

    if (collection) {
        UINT count = 0;
        collection->GetCount(&count);
        for (UINT i = 0; i < count; i++) {
            IMMDevice* dev = nullptr;
            if (FAILED(collection->Item(i, &dev)) || !dev) continue;

            IPropertyStore* props = nullptr;
            if (SUCCEEDED(dev->OpenPropertyStore(STGM_READ, &props)) && props) {
                PROPVARIANT varName;
                PropVariantInit(&varName);
                if (SUCCEEDED(props->GetValue(PKEY_Device_FriendlyName, &varName))) {
                    if (varName.vt == VT_LPWSTR && varName.pwszVal != nullptr) {
                        if (wcsstr(varName.pwszVal, targetRenderDeviceName) != nullptr) {
                            renderDevice = dev;
                            PropVariantClear(&varName);
                            props->Release();
                            break;
                        }
                    }
                    PropVariantClear(&varName);
                }
                props->Release();
            }
            dev->Release();
        }
    }

    if (collection) collection->Release();
    enumerator->Release();

    if (!renderDevice) {
        std::wcerr << L"Error: Could not find device matching name: '" 
                << targetRenderDeviceName << L"'\n";
        micDevice->Release();
        return false;
    }

    // 3. activate capture client and get capture format
    micDevice->Activate(__uuidof(IAudioClient), CLSCTX_ALL, NULL, (void**)&captureAudioClient);
    micDevice->Release();
    if (!captureAudioClient) return false;

    WAVEFORMATEX* captureFormat = nullptr;
    captureAudioClient->GetMixFormat(&captureFormat);
    if (!captureFormat) {
        std::cerr << "Error: Capture GetMixFormat failed." << std::endl;
        return false;
    }

    captureChannels = captureFormat->nChannels;
    captureSampleRate = captureFormat->nSamplesPerSec;
    captureBlockAlign = captureFormat->nBlockAlign;
    captureBitsPerSample = captureFormat->wBitsPerSample;

    captureAudioClient->Initialize(
        AUDCLNT_SHAREMODE_SHARED, 
        0, 
        10000000, // 1 second buffer
        0, 
        captureFormat, 
        NULL
    );
    CoTaskMemFree(captureFormat); // free memory allocated by WASAPI
    captureFormat = nullptr;

    captureAudioClient->GetService(__uuidof(IAudioCaptureClient), (void**)&captureClient);

    // 4. activate render client and get render format
    renderDevice->Activate(__uuidof(IAudioClient), CLSCTX_ALL, NULL, (void**)&renderAudioClient);
    renderDevice->Release();
    if (!renderAudioClient) return false;

    WAVEFORMATEX* renderFormat = nullptr;
    renderAudioClient->GetMixFormat(&renderFormat);

    renderChannels = renderFormat->nChannels;
    renderSampleRate = renderFormat->nSamplesPerSec;
    renderBlockAlign = renderFormat->nBlockAlign;
    renderBitsPerSample = renderFormat->wBitsPerSample;

    renderAudioClient->Initialize(
        AUDCLNT_SHAREMODE_SHARED, 
        0, 
        10000000, // 1 second buffer
        0, 
        renderFormat, 
        NULL
    );
    CoTaskMemFree(renderFormat); // free memory allocated by WASAPI
    renderFormat = nullptr;

    renderAudioClient->GetService(__uuidof(IAudioRenderClient), (void**)&renderClient);

    // 5. start everything, return
    isRunning.store(true);
    captureAudioClient->Start();
    renderAudioClient->Start();
    workerThread = std::thread(&WasapiMixer::MixLoop, this);
    return true;
}

void WasapiMixer::Stop() {
    if (isRunning.load()) {
        isRunning.store(false);
        
        // wait for worker thread to finish
        if (workerThread.joinable()) workerThread.join();

        // stop audio streams
        if (captureAudioClient) captureAudioClient->Stop();
        if (renderAudioClient) renderAudioClient->Stop();

        // release COM interfaces to prevent leaks
        if (captureClient) { captureClient->Release(); captureClient = nullptr; }
        if (captureAudioClient) { captureAudioClient->Release(); captureAudioClient = nullptr; }
        if (renderClient) { renderClient->Release(); renderClient = nullptr; }
        if (renderAudioClient) { renderAudioClient->Release(); renderAudioClient = nullptr; }
    }
}

void WasapiMixer::MixLoop() {
    CoInitializeEx(NULL, COINIT_MULTITHREADED);

    BYTE* capData = nullptr;
    UINT32 capNumFrames = 0;
    DWORD flags = 0;
    BYTE* renData = nullptr;
    UINT32 bufferFrameCount = 0;
    UINT32 numPaddingFrames = 0;

    const int isStereo = (captureChannels >= 2) ? 1 : 0;
    const float effectiveMicGain = (isStereo) ? micGain.load(std::memory_order_relaxed) : (micGain.load(std::memory_order_relaxed) * .7071f);
    renderAudioClient->GetBufferSize(&bufferFrameCount);

    while (isRunning.load()) {
        UINT32 nextPacketSize = 0;
        captureClient->GetNextPacketSize(&nextPacketSize);

        while (nextPacketSize > 0) {
            // 1. get microphone input buffer
            captureClient->GetBuffer(&capData, &capNumFrames, &flags, NULL, NULL);

            // 2. get available space in output render buffer
            renderAudioClient->GetCurrentPadding(&numPaddingFrames);
            UINT32 availableRenderFrames = bufferFrameCount - numPaddingFrames;

            // wait 1 ms to avoid dropping packets on full buffer
            if (availableRenderFrames < capNumFrames) {
                captureClient->ReleaseBuffer(capNumFrames);
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                captureClient->GetNextPacketSize(&nextPacketSize);
                continue;
            }

            // 3. lock output render buffer
            renderClient->GetBuffer(capNumFrames, &renData);

            if (renData) {
                // cast raw byte buffers to float pointers
                const float* micSamples = reinterpret_cast<const float*>(capData);
                float* outSamples = reinterpret_cast<float*>(renData);

                // format configuration parameters
                const UINT32 capChannels = captureChannels; 
                const UINT32 renChannels = renderChannels; 

                float wGain = wavGain.load(std::memory_order_relaxed);

                bool mixingWav = isPlayingWav.load(std::memory_order_relaxed);
                int activeIndex = wavBufferIndex.load(std::memory_order_relaxed);
                size_t currentWavIdx = 0;

                if (mixingWav && activeIndex >= 0 && activeIndex < g_soundbiteCount) {
                    currentWavIdx = wavReadIndex.load(std::memory_order_relaxed);
                }

                const std::vector<float>* currentBuffer = nullptr;
                if (mixingWav && activeIndex >= 0 && activeIndex < g_soundbiteCount) {
                    currentBuffer = &wavBuffers[activeIndex];
                }

                for (UINT32 frame = 0; frame < capNumFrames; ++frame) {
                    // 1. extract mic samples 
                    // get exact byte offsets using channel counts
                    UINT32 micFrameOffset = frame * capChannels; 
                    UINT32 outFrameOffset = frame * renChannels;

                    // capture channels 0 & 1 for left & right
                    float micL = micSamples[micFrameOffset + 0] * effectiveMicGain;
                    float micR = micSamples[micFrameOffset + isStereo] * effectiveMicGain;

                    // 2. extract soundbite samples 
                    float wavL = 0.0f;
                    float wavR = 0.0f;

                    if (mixingWav && currentBuffer) {
                        if (currentWavIdx + 1 < currentBuffer->size()) {
                            wavL = (*currentBuffer)[currentWavIdx]     * wGain;
                            wavR = (*currentBuffer)[currentWavIdx + 1] * wGain;
                            currentWavIdx += 2; // move soundbite forward by 2 float samples (1 frame)
                        } else {
                            // end of soundbite
                            mixingWav = false;
                            isPlayingWav.store(false, std::memory_order_relaxed);
                            wavBufferIndex.store(-1, std::memory_order_relaxed);
                        }
                    }

                    // 3. mix and clip to prevent distortion
                    float finalL = micL + wavL;
                    float finalR = micR + wavR;

                    // hard clip between -1.0f and 1.0f
                    outSamples[outFrameOffset + 0] = (finalL > 1.0f) ? 1.0f : ((finalL < -1.0f) ? -1.0f : finalL);
                    outSamples[outFrameOffset + 1] = (finalR > 1.0f) ? 1.0f : ((finalR < -1.0f) ? -1.0f : finalR);
                }

                // write back updated wav read position
                if (mixingWav) {
                    wavReadIndex.store(currentWavIdx, std::memory_order_relaxed);
                }

                renderClient->ReleaseBuffer(capNumFrames, 0);
            }

            captureClient->ReleaseBuffer(capNumFrames);
            captureClient->GetNextPacketSize(&nextPacketSize);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    CoUninitialize();
}