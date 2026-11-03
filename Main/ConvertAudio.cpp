#define NOMINMAX
#include "ConvertAudio.h"
#include <windows.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <wrl/client.h>
#include <cstring>
#include <fstream>
#include <iostream>

using Microsoft::WRL::ComPtr;

namespace {

const uint16_t WAV_FORMAT_PCM = 1;
const uint16_t WAV_FORMAT_FLOAT = 3;
const uint16_t WAV_FORMAT_EXTENSIBLE = 0xFFFE;
const DWORD FIRST_AUDIO_STREAM = static_cast<DWORD>(MF_SOURCE_READER_FIRST_AUDIO_STREAM);

// starts media foundation for the lifetime of the object
struct MfSession {
    HRESULT hr;
    MfSession() { hr = MFStartup(MF_VERSION); }
    ~MfSession() { if (SUCCEEDED(hr)) MFShutdown(); }
};

std::wstring PathToWString(const std::string& str) {
    if (str.empty()) return std::wstring();
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}

// read the fmt chunk (and the data chunk if rawAudioData is given) from a wav file
bool ReadWav(const std::string& path, AudioFormat& format, uint16_t& audioFormat, std::vector<BYTE>* rawAudioData) {
    // open file
    // std::ifstream file(path, std::ios::binary);
    std::ifstream file(PathToWString(path), std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "ReadWav Error: Could not open file: " << path << "\n";
        return false;
    }

    // declare file format vars
    char riffId[4], formatId[4];
    uint32_t chunkSize = 0;
    char subchunkId[4];
    uint32_t subchunkSize = 0;
    bool foundFmt = false, foundData = false;

    // verify riff or wave formatting data
    file.read(riffId, 4);
    file.read(reinterpret_cast<char*>(&chunkSize), 4);
    file.read(formatId, 4);
    if (!file || memcmp(riffId, "RIFF", 4) != 0 || memcmp(formatId, "WAVE", 4) != 0) {
        std::cerr << "ReadWav Error: Invalid WAV format: " << path << "\n";
        return false;
    }

    // get file format data and file audio data
    while (file.read(subchunkId, 4)) {
        if (!file.read(reinterpret_cast<char*>(&subchunkSize), 4)) break;
        std::streampos chunkStart = file.tellg();

        if (memcmp(subchunkId, "fmt ", 4) == 0) {
            if (subchunkSize < 16) break;
            uint32_t byteRate = 0;
            uint16_t blockAlign = 0;
            file.read(reinterpret_cast<char*>(&audioFormat), 2);
            file.read(reinterpret_cast<char*>(&format.channels), 2);
            file.read(reinterpret_cast<char*>(&format.sampleRate), 4);
            file.read(reinterpret_cast<char*>(&byteRate), 4);
            file.read(reinterpret_cast<char*>(&blockAlign), 2);
            file.read(reinterpret_cast<char*>(&format.bitsPerSample), 2);

            // extensible wavs store the real format tag at the start of the sub format guid
            if (audioFormat == WAV_FORMAT_EXTENSIBLE && subchunkSize >= 40) {
                file.seekg(chunkStart + std::streamoff(24));
                file.read(reinterpret_cast<char*>(&audioFormat), 2);
            }
            foundFmt = true;
        }
        else if (memcmp(subchunkId, "data", 4) == 0 && rawAudioData) {
            // don't trust the header size, cap it at what is left in the file
            file.seekg(0, std::ios::end);
            std::streamoff remaining = file.tellg() - chunkStart;
            file.seekg(chunkStart);
            if (subchunkSize > static_cast<uint64_t>(remaining)) subchunkSize = static_cast<uint32_t>(remaining);
            rawAudioData->resize(subchunkSize);
            file.read(reinterpret_cast<char*>(rawAudioData->data()), subchunkSize);
            foundData = true;
        }

        if (foundFmt && (foundData || !rawAudioData)) break;
        if (!file) break;

        // chunks are padded to an even size
        file.seekg(chunkStart + std::streamoff(subchunkSize + (subchunkSize & 1)));
    }
    file.close();

    if (!foundFmt || (rawAudioData && !foundData)) {
        std::cerr << "ReadWav Error: Missing fmt or data chunk in: " << path << "\n";
        return false;
    }
    return true;
}

// convert raw pcm / float bytes to 32 bit float samples, returns empty if the format isn't supported
std::vector<float> DecodeWavSamples(const AudioFormat& format, uint16_t audioFormat, const std::vector<BYTE>& rawAudioData) {
    std::vector<float> samples;
    size_t bytesPerSample = format.bitsPerSample / 8;
    if (bytesPerSample == 0) return samples;
    size_t sampleCount = rawAudioData.size() / bytesPerSample;
    const BYTE* data = rawAudioData.data();
    samples.reserve(sampleCount);

    if (audioFormat == WAV_FORMAT_PCM) {
        for (size_t i = 0; i < sampleCount; ++i) {
            const BYTE* p = data + i * bytesPerSample;
            if (format.bitsPerSample == 8) {
                samples.push_back((p[0] - 128) / 128.0f); // 8 bit wav is unsigned
            } else if (format.bitsPerSample == 16) {
                int16_t v;
                memcpy(&v, p, 2);
                samples.push_back(v / 32768.0f);
            } else if (format.bitsPerSample == 24) {
                int32_t v = static_cast<int32_t>((uint32_t(p[0]) << 8) | (uint32_t(p[1]) << 16) | (uint32_t(p[2]) << 24));
                samples.push_back(v / 2147483648.0f);
            } else if (format.bitsPerSample == 32) {
                int32_t v;
                memcpy(&v, p, 4);
                samples.push_back(v / 2147483648.0f);
            } else {
                return {};
            }
        }
    }
    else if (audioFormat == WAV_FORMAT_FLOAT && format.bitsPerSample == 32) {
        samples.resize(sampleCount);
        memcpy(samples.data(), data, sampleCount * sizeof(float));
    }
    return samples;
}

} // namespace

AudioFileType GetAudioFileType(const std::string& path) {
    // std::ifstream file(path, std::ios::binary);
    std::ifstream file(PathToWString(path), std::ios::binary);
    if (!file.is_open()) return AudioFileType::Unknown;

    BYTE header[12] = { 0 };
    file.read(reinterpret_cast<char*>(header), sizeof(header));
    if (file.gcount() < 4) return AudioFileType::Unknown;

    // riff / wave header
    if (file.gcount() == 12 && memcmp(header, "RIFF", 4) == 0 && memcmp(header + 8, "WAVE", 4) == 0) {
        return AudioFileType::Wav;
    }
    // id3 tag or an mpeg frame sync (11 set bits)
    if (memcmp(header, "ID3", 3) == 0 || (header[0] == 0xFF && (header[1] & 0xE0) == 0xE0)) {
        return AudioFileType::Mp3;
    }
    return AudioFileType::Unknown;
}

bool GetAudioInfo(const std::string& path, AudioFileType& type, AudioFormat& format) {
    type = GetAudioFileType(path);

    if (type == AudioFileType::Wav) {
        uint16_t audioFormat = 0;
        return ReadWav(path, format, audioFormat, nullptr);
    }
    if (type == AudioFileType::Mp3) {
        MfSession mf;
        if (FAILED(mf.hr)) return false;

        // read the native (compressed) media type of the first audio stream
        ComPtr<IMFSourceReader> reader;
        if (FAILED(MFCreateSourceReaderFromURL(PathToWString(path).c_str(), NULL, &reader))) return false;
        ComPtr<IMFMediaType> nativeType;
        if (FAILED(reader->GetNativeMediaType(FIRST_AUDIO_STREAM, 0, &nativeType))) return false;

        UINT32 sampleRate = 0, channels = 0, bitsPerSample = 0;
        nativeType->GetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, &sampleRate);
        nativeType->GetUINT32(MF_MT_AUDIO_NUM_CHANNELS, &channels);
        nativeType->GetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, &bitsPerSample);
        format.sampleRate = sampleRate;
        format.channels = static_cast<uint16_t>(channels);
        format.bitsPerSample = bitsPerSample ? static_cast<uint16_t>(bitsPerSample) : 16; // mp3 is lossy, no real bit depth
        return sampleRate != 0 && channels != 0;
    }
    return false;
}

std::vector<float> ConvertWav(const std::string& path, const AudioFormat& target) {
    if (target.channels != 1 && target.channels != 2) {
        std::cerr << "ConvertWav Error: Only mono or stereo output is supported.\n";
        return {};
    }

    // read the wav and decode its samples to float
    AudioFormat source;
    uint16_t audioFormat = 0;
    std::vector<BYTE> rawAudioData;
    if (!ReadWav(path, source, audioFormat, &rawAudioData)) return {};
    std::vector<float> samples = DecodeWavSamples(source, audioFormat, rawAudioData);
    if (samples.empty() || source.channels == 0 || source.sampleRate == 0) {
        std::cerr << "ConvertWav Error: Unsupported WAV format: " << path << "\n";
        return {};
    }

    // change channel count: left and right are the first two channels, mono is duplicated
    size_t frameCount = samples.size() / source.channels;
    std::vector<float> converted;
    converted.reserve(frameCount * target.channels);
    for (size_t frame = 0; frame < frameCount; ++frame) {
        float left = samples[frame * source.channels];
        float right = (source.channels > 1) ? samples[frame * source.channels + 1] : left;
        if (target.channels == 1) {
            converted.push_back((left + right) * 0.5f);
        } else {
            converted.push_back(left);
            converted.push_back(right);
        }
    }
    if (source.sampleRate == target.sampleRate) return converted;

    // change sample rate with linear interpolation
    size_t outFrames = static_cast<size_t>(static_cast<uint64_t>(frameCount) * target.sampleRate / source.sampleRate);
    std::vector<float> resampled;
    resampled.reserve(outFrames * target.channels);
    for (size_t i = 0; i < outFrames; ++i) {
        double pos = static_cast<double>(i) * source.sampleRate / target.sampleRate;
        size_t index = static_cast<size_t>(pos);
        size_t next = (index + 1 < frameCount) ? index + 1 : index;
        float frac = static_cast<float>(pos - index);
        for (uint16_t c = 0; c < target.channels; ++c) {
            float a = converted[index * target.channels + c];
            float b = converted[next * target.channels + c];
            resampled.push_back(a + (b - a) * frac);
        }
    }
    return resampled;
}

std::vector<float> ConvertMp3(const std::string& path, const AudioFormat& target) {
    if (target.bitsPerSample != 16) {
        std::cerr << "ConvertMp3 Error: Only 16 bit output is supported.\n";
        return {};
    }
    MfSession mf;
    if (FAILED(mf.hr)) {
        std::cerr << "ConvertMp3 Error: Media Foundation failed to start.\n";
        return {};
    }

    ComPtr<IMFSourceReader> reader;
    if (FAILED(MFCreateSourceReaderFromURL(PathToWString(path).c_str(), NULL, &reader))) {
        std::cerr << "ConvertMp3 Error: Could not open file: " << path << "\n";
        return {};
    }

    // ask the reader to decode to pcm in the target format (it resamples and remixes channels for us)
    ComPtr<IMFMediaType> pcmType;
    MFCreateMediaType(&pcmType);
    UINT32 blockAlign = target.channels * (target.bitsPerSample / 8);
    pcmType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
    pcmType->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM);
    pcmType->SetUINT32(MF_MT_AUDIO_NUM_CHANNELS, target.channels);
    pcmType->SetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, target.sampleRate);
    pcmType->SetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, target.bitsPerSample);
    pcmType->SetUINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT, blockAlign);
    pcmType->SetUINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, blockAlign * target.sampleRate);
    pcmType->SetUINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, TRUE);
    if (FAILED(reader->SetStreamSelection(FIRST_AUDIO_STREAM, TRUE)) ||
        FAILED(reader->SetCurrentMediaType(FIRST_AUDIO_STREAM, NULL, pcmType.Get()))) {
        std::cerr << "ConvertMp3 Error: Could not decode MP3: " << path << "\n";
        return {};
    }

    // read decoded samples until end of stream and convert 16 bit pcm to 32 bit float
    std::vector<float> floatAudioData;
    while (true) {
        DWORD flags = 0;
        ComPtr<IMFSample> sample;
        if (FAILED(reader->ReadSample(FIRST_AUDIO_STREAM, 0, NULL, &flags, NULL, &sample))) break;
        if (flags & MF_SOURCE_READERF_ENDOFSTREAM) break;
        if (!sample) continue;

        ComPtr<IMFMediaBuffer> buffer;
        if (FAILED(sample->ConvertToContiguousBuffer(&buffer))) break;
        BYTE* data = nullptr;
        DWORD length = 0;
        if (FAILED(buffer->Lock(&data, NULL, &length))) break;
        const int16_t* pcm16 = reinterpret_cast<const int16_t*>(data);
        size_t sampleCount = length / sizeof(int16_t);
        for (size_t i = 0; i < sampleCount; ++i) {
            floatAudioData.push_back(pcm16[i] / 32768.0f);
        }
        buffer->Unlock();
    }

    if (floatAudioData.empty()) {
        std::cerr << " ConvertMp3 Error: Data payload empty in: " << path << "\n";
    }
    return floatAudioData;
}

std::vector<float> ConvertAudioFile(const std::string& path, const AudioFormat& target) {
    AudioFileType type = AudioFileType::Unknown;
    AudioFormat source;
    if (!GetAudioInfo(path, type, source)) {
        std::cerr << "ConvertAudioFile Error: Unsupported audio file: " << path << "\n";
        return {};
    }

    switch (type) {
        case AudioFileType::Wav: return ConvertWav(path, target);
        case AudioFileType::Mp3: return ConvertMp3(path, target);
        default: return {};
    }
}
