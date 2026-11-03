#define NOMINMAX
#include "SbWindow.h"
#include "Tray.h"
#include "WasapiMixer.h"
#include "ConvertAudio.h"
#include <iostream>
#include <fstream>

std::wstring StringToWString(const std::string &str) {
    if (str.empty()) return std::wstring();
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}

static std::string ExeDirUtf8() {
    wchar_t buf[MAX_PATH];
    DWORD n = GetModuleFileNameW(NULL, buf, MAX_PATH);
    if (n == 0 || n >= MAX_PATH) return std::string();
    std::wstring dir(buf, n);
    dir.resize(dir.find_last_of(L'\\') + 1);
    int len = WideCharToMultiByte(CP_UTF8, 0, dir.c_str(), (int)dir.size(), NULL, 0, NULL, NULL);
    std::string out(len, '\0');
    WideCharToMultiByte(CP_UTF8, 0, dir.c_str(), (int)dir.size(), &out[0], len, NULL, NULL);
    return out;
}

std::vector<float> LoadWavFile(const std::string& wavPath) {
    // open file
    // std::ifstream file(wavPath, std::ios::binary);
    std::ifstream file(StringToWString(wavPath), std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Main::LoadWavFile Error: Could not open file: " << wavPath << "\n";
        return {};
    }

    // declare file format vars
    char riffId[4], formatId[4];
    uint32_t chunkSize = 0;
    
    // declare audio vars
    std::vector<BYTE> rawAudioData;
    uint16_t numChannels = 2;
    char subchunkId[4];
    uint32_t subchunkSize = 0;
    
    // verify and retrieve riff or wave formatting data
    file.read(riffId, 4);
    file.read(reinterpret_cast<char*>(&chunkSize), 4);
    file.read(formatId, 4);
    if (memcmp(riffId, "RIFF", 4) != 0 || memcmp(formatId, "WAVE", 4) != 0) {
        file.close();
        std::vector<float> converted = ConvertAudioFile(wavPath);
        if (!converted.empty()) return converted;
        else {
            std::cerr << "Main::LoadWavFile Error: Failed to convert file." << "\n";
            return {};
        }
    }

    // get remaining file format data and file audio data & close file
    while (file.read(subchunkId, 4)) {
        if (!file.read(reinterpret_cast<char*>(&subchunkSize), 4)) break;

        if (memcmp(subchunkId, "fmt ", 4) == 0) {
            uint16_t audioFormat = 0;
            file.read(reinterpret_cast<char*>(&audioFormat), 2);
            file.read(reinterpret_cast<char*>(&numChannels), 2);
            
            // skip remaining bytes of fmt chunk body
            if (subchunkSize > 4) {
                file.seekg(subchunkSize - 4, std::ios::cur);
            }
        }
        else if (memcmp(subchunkId, "data", 4) == 0) {
            std::streampos pos = file.tellg();
            file.seekg(0, std::ios::end);
            std::streamoff remaining = file.tellg() - pos;
            file.seekg(pos);
            if (subchunkSize > static_cast<uint64_t>(remaining)) subchunkSize = static_cast<uint32_t>(remaining);
            rawAudioData.resize(subchunkSize);
            file.read(reinterpret_cast<char*>(rawAudioData.data()), subchunkSize);
            break;
        }
        else {
            file.seekg(subchunkSize, std::ios::cur);
        }
    }
    file.close();

    if (rawAudioData.empty()) {
        std::cerr << "Main::LoadWavFile Error: Data payload empty in: " << wavPath << "\n";
        return {};
    }

    // convert raw audio data (16 bit signed integer sample) to 32 bit pcm float audio data for WASAPI and return it
    std::vector<float> floatAudioData;
    size_t sampleCount = rawAudioData.size() / sizeof(int16_t);
    const int16_t* pcm16 = reinterpret_cast<const int16_t*>(rawAudioData.data());
    if (numChannels == 1) {
        // expand mono to stereo for WASAPI
        floatAudioData.reserve(sampleCount * 2);
        for (size_t i = 0; i < sampleCount; ++i) {
            float sample = pcm16[i] / 32768.0f;
            floatAudioData.push_back(sample); // left channel
            floatAudioData.push_back(sample); // right channel
        }
    } else {
        floatAudioData.reserve(sampleCount);
        for (size_t i = 0; i < sampleCount; ++i) {
            floatAudioData.push_back(pcm16[i] / 32768.0f);
        }
    }
    return floatAudioData;
}

char GetPressedAlphaNumericKey() {
    // check numbers 0 - 9
    for (int vk = '0'; vk <= '9'; ++vk) {
        if (GetAsyncKeyState(vk) & 0x8000) {
            return static_cast<char>(vk);
        }
    }

    // check letters a - z
    for (int vk = 'A'; vk <= 'Z'; ++vk) {
        if (GetAsyncKeyState(vk) & 0x8000) {
            return static_cast<char>(vk);
        }
    }

    return 0; // no letter or number key pressed
}

int main() {
    // 1. Initialize and declare variables
    SetDefaultDllDirectories(LOAD_LIBRARY_SEARCH_APPLICATION_DIR | LOAD_LIBRARY_SEARCH_SYSTEM32);
    std::string soundbiteBasePath = ExeDirUtf8() + "Soundbites\\";
    std::string soundbitePaths[g_soundbiteCount] = {
        "1.wav", "2.wav", "3.wav", "4.mp3", "5.wav"
    };
    WasapiMixer mixer;
    int soundbiteIndex = -1;
    CoInitializeEx(NULL, COINIT_MULTITHREADED);

    // 2. Load soundbites into memory and populate global tray variables for soundbite names
    for (int i = 0; i < 5; i++) {
        mixer.LoadSoundbite(i, LoadWavFile(soundbiteBasePath + soundbitePaths[i]));  
    }

    // 3. Start mixer background thread (captures Mic and routes to VB-Cable)
    if (!mixer.Start(L"CABLE Input")) {
        std::cerr << "Main::Main Error: Failed to start WasapiMixer! Make sure VB-Cable is active.";
        CoUninitialize();
        system("pause");
        return -1;
    }

    // 4. Initialize tray loop and tray path names to populate tray menu with soundbite names
    g_soundbiteNames.resize(g_soundbiteCount);
    g_soundbiteKeys.resize(g_soundbiteCount);
    for (int i = 0; i < g_soundbiteCount; i++) {
        std::string soundbitePath = soundbitePaths[i].substr(0, soundbitePaths[i].length() - 4);
        std::wstring w_soundbitePath = StringToWString(soundbitePath);
        std::wstring w_fullSoundbitePath = g_soundbiteKeys[i] + w_soundbitePath;
        g_soundbiteNames[i] = w_fullSoundbitePath;
    }

    std::vector<SbEntry> sbEntries;
    std::string loadedPaths[g_soundbiteCount];
    for (int i = 0; i < g_soundbiteCount; i++) {
        if (soundbitePaths[i].empty()) continue;
        loadedPaths[i] = soundbiteBasePath + soundbitePaths[i];
        sbEntries.push_back({ loadedPaths[i], static_cast<char>(g_soundbiteKeys[i][0]), i });
    }
    SbWindowInit(sbEntries);
    g_OnSoundbitesChanged = [&]() {
        std::vector<SbEntry> current = SbGetEntries();
        for (int i = 0; i < g_soundbiteCount; i++) {
            std::string path;
            for (const SbEntry& e : current) if (e.slot == i) path = e.path;
            if (path == loadedPaths[i]) continue;
            mixer.LoadSoundbite(i, path.empty() ? std::vector<float>{} : LoadWavFile(path));
            loadedPaths[i] = path;
        }
    };
    g_OnTrayDoubleClick = SbWindowShow;
    std::thread trayThread(TrayLoop);

    // 5. Main event loop waits for hotkeys
    while (g_running.load()) {
        bool ctrl = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
        bool shift = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
        bool alt = (GetAsyncKeyState(VK_MENU) & 0x8000) != 0;

        if (ctrl && shift && alt) {
            char pressedKey = GetPressedAlphaNumericKey();
            if (pressedKey == '\0') { std::this_thread::sleep_for(std::chrono::milliseconds(15)); continue; }
            switch (pressedKey) {
                case '1': soundbiteIndex = 0; break;
                case '2': soundbiteIndex = 1; break;
                case '3': soundbiteIndex = 2; break;
                case '4': soundbiteIndex = 3; break;
                case '5': soundbiteIndex = 4; break;
                case 'Q': soundbiteIndex = 5; break;
                case 'W': soundbiteIndex = 6; break;
                case 'E': soundbiteIndex = 7; break;
                case 'R': soundbiteIndex = 8; break;
                case 'T': soundbiteIndex = 9; break;
                case 'A': soundbiteIndex = 10; break;
                case 'S': soundbiteIndex = 11; break;
                case 'D': soundbiteIndex = 12; break;
                case 'F': soundbiteIndex = 13; break;
                case 'G': soundbiteIndex = 14; break;
                case 'Z': soundbiteIndex = 15; break;
                case 'X': soundbiteIndex = 16; break;
                case 'C': soundbiteIndex = 17; break;
                case 'V': soundbiteIndex = 18; break;
                case 'B': soundbiteIndex = 19; break;
                case 'P': soundbiteIndex = -1; g_running.store(false); break;
                default:
                    soundbiteIndex = SbFindByHotkey(pressedKey);
                    if (soundbiteIndex < 0) continue;
                    break;
            }
            mixer.TriggerSoundbite(soundbiteIndex);
            std::this_thread::sleep_for(std::chrono::milliseconds(200)); // debounce keypress
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(15));
    }

    g_OnSoundbitesChanged = nullptr;
    g_OnTrayDoubleClick = nullptr;
    SbWindowShutdown();
    mixer.Stop();
    CoUninitialize();
    if (g_trayThreadId != 0) {
        PostThreadMessage(g_trayThreadId, WM_QUIT, 0, 0);
    }
    if (trayThread.joinable()) {
        trayThread.join();
    }
    return 0;
}