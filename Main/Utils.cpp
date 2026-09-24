#include "Utils.h"

std::wstring StringToWString(const std::string &str) {
    if (str.empty()) return std::wstring();
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}

std::vector<float> LoadWavFile(const std::string& wavPath) {
    // open file
    std::ifstream file(wavPath, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file: " << wavPath << "\n";
        return {};
    }

    // declare file format vars
    char riffId[4], formatId[4];
    uint32_t chunkSize = 0;
    
    // declare audio vars
    std::vector<BYTE> rawAudioData;
    uint16_t numChannels = 2; // default to stereo
    char subchunkId[4];
    uint32_t subchunkSize = 0;
    
    // verify and retrieve riff or wave formatting data
    file.read(riffId, 4);
    file.read(reinterpret_cast<char*>(&chunkSize), 4);
    file.read(formatId, 4);
    if (memcmp(riffId, "RIFF", 4) != 0 || memcmp(formatId, "WAVE", 4) != 0) {
        std::cerr << "Error: Invalid WAV format: " << wavPath << "\n";
        return {};
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
        std::cerr << "Error: Data payload empty in: " << wavPath << "\n";
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