#define NOMINMAX
#include "WasapiMixer.h"
#include "Utils.h"
#include "Tray.h"

int main() {
    // 1. Initialize and declare variables
    std::string soundbiteBasePath = "C:\\Users\\Public\\cpp_soundboard\\Resources\\Test_Soundbites\\";
    std::string soundbitePaths[g_soundbiteCount] = {
        "1.wav", "2.wav", "3.wav", "4.wav", "5.wav"
    };
    WasapiMixer mixer;
    int soundbiteIndex = -1;
    CoInitializeEx(NULL, COINIT_MULTITHREADED);

    // 2. Load soundbites into memory and populate global tray variables for soundbite names
    for (int i = 0; i < g_soundbiteCount; i++) {
        std::vector<float> soundbiteData = LoadWavFile(soundbiteBasePath + soundbitePaths[i]);
        mixer.LoadSoundbite(i, soundbiteData);  
    }

    // 3. Start mixer background thread (captures Mic and routes to VB-Cable)
    if (!mixer.Start(L"CABLE Input")) {
        std::cerr << "Failed to start WasapiMixer! Make sure VB-Cable is active.\n";
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
    std::thread trayThread(TrayLoop);

    // 5. Main event loop waits for hotkeys
    while (true) {
        if (!g_running.load()) {
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
                case 'Q': soundbiteIndex = -1; g_running.store(false); break;
                default: continue;
            }
            mixer.TriggerSoundbite(soundbiteIndex);
            std::this_thread::sleep_for(std::chrono::milliseconds(200)); // debounce keypress
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(15));
    }
}