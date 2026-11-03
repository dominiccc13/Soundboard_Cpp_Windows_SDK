#pragma once
#include <cstdint>
#include <string>
#include <vector>

enum class AudioFileType { Unknown, Wav, Mp3 };

// defaults match the README: 48.000 kHz, 2 channels, 16 bit
struct AudioFormat {
    uint32_t sampleRate = 48000;
    uint16_t channels = 2;
    uint16_t bitsPerSample = 16;
};

// detect the file type from the header bytes (not the extension)
AudioFileType GetAudioFileType(const std::string& path);

// get the file type and the format (sample rate, channels, bit depth) the file is currently in
bool GetAudioInfo(const std::string& path, AudioFileType& type, AudioFormat& format);

// convert to interleaved 32 bit float audio data in the target format for WASAPI, returns empty on failure
std::vector<float> ConvertWav(const std::string& path, const AudioFormat& target = AudioFormat());
std::vector<float> ConvertMp3(const std::string& path, const AudioFormat& target = AudioFormat());

// picks the converter that matches the file type
std::vector<float> ConvertAudioFile(const std::string& path, const AudioFormat& target = AudioFormat());
