
// raw2wav.cpp
// C++17 single-file tool: extract 48-sample audio blocks per record into a 48 kHz mono WAV.
//
// Usage:
//   g++ -O2 -std=c++17 -o raw2wav raw2wav.cpp
//   ./raw2wav input.raw output.wav
//
// Assumptions (little-endian):
//   Header: "OCTOPUS_HEEG" (12 bytes), uint32 ampCount, uint32 chPerAmp
//   Record: int64 offset, float eeg[ampCount*chPerAmp], float audio[48], int32 trigger

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <limits>
#include <algorithm>

namespace {

constexpr uint32_t WAV_SR = 48000;     // 48 kHz
constexpr uint16_t WAV_CH = 1;         // mono
constexpr uint16_t WAV_BPS = 16;       // 16-bit PCM
constexpr size_t   AUDIO_SAMPLES_PER_REC = 48;

template <typename T>
bool readExact(std::ifstream& ifs, T* out) {
    return static_cast<bool>(ifs.read(reinterpret_cast<char*>(out), sizeof(T)));
}

bool readExactN(std::ifstream& ifs, void* dst, std::streamsize n) {
    return static_cast<bool>(ifs.read(reinterpret_cast<char*>(dst), n));
}

inline int16_t floatToInt16(float x) {
    // Expect input roughly in [-1, 1]. Clip to int16 range.
    // If your floats are in a different scale, adjust the gain here.
    const float s = std::max(-1.0f, std::min(1.0f, x));
    const int v = static_cast<int>(std::lrintf(s * 32767.0f));
    // lrintf may return 32768 for s=1.0 due to rounding; clamp.
    return static_cast<int16_t>(std::max(-32768, std::min(32767, v)));
}

struct Header {
    char magic[12];        // "OCTOPUS_HEEG"
    uint32_t ampCount;     // number of amps
    uint32_t chPerAmp;     // channels per amp (e.g., 66)
};

#pragma pack(push,1)
struct WavHeader {
    // RIFF chunk
    char     riff[4];          // "RIFF"
    uint32_t riffSize;         // 4 + (8 + fmtSize) + (8 + dataSize)
    char     wave[4];          // "WAVE"
    // fmt subchunk
    char     fmt[4];           // "fmt "
    uint32_t fmtSize;          // 16 for PCM
    uint16_t audioFormat;      // 1 = PCM
    uint16_t numChannels;      // 1
    uint32_t sampleRate;       // 48000
    uint32_t byteRate;         // sampleRate * numChannels * bitsPerSample/8
    uint16_t blockAlign;       // numChannels * bitsPerSample/8
    uint16_t bitsPerSample;    // 16
    // data subchunk
    char     data[4];          // "data"
    uint32_t dataSize;         // numSamples * blockAlign
};
#pragma pack(pop)

// Write a placeholder WAV header; we’ll seek back and fix sizes at the end.
void writeWavHeader(std::ofstream& ofs, uint32_t sampleRate, uint16_t channels, uint16_t bitsPerSample, uint32_t dataSizeBytes) {
    WavHeader h{};
    std::memcpy(h.riff, "RIFF", 4);
    std::memcpy(h.wave, "WAVE", 4);
    std::memcpy(h.fmt,  "fmt ", 4);
    std::memcpy(h.data, "data", 4);

    h.fmtSize       = 16;
    h.audioFormat   = 1; // PCM
    h.numChannels   = channels;
    h.sampleRate    = sampleRate;
    h.bitsPerSample = bitsPerSample;
    h.blockAlign    = static_cast<uint16_t>((channels * bitsPerSample) / 8);
    h.byteRate      = sampleRate * h.blockAlign;
    h.dataSize      = dataSizeBytes;
    h.riffSize      = 4 + (8 + h.fmtSize) + (8 + h.dataSize);

    ofs.write(reinterpret_cast<const char*>(&h), sizeof(h));
}

void fixWavSizes(std::ofstream& ofs, uint32_t dataSizeBytes) {
    // Update RIFF size and data chunk size
    const uint32_t riffSize = 4 + (8 + 16) + (8 + dataSizeBytes);
    ofs.seekp(4, std::ios::beg);
    ofs.write(reinterpret_cast<const char*>(&riffSize), sizeof(riffSize));
    ofs.seekp(40, std::ios::beg);
    ofs.write(reinterpret_cast<const char*>(&dataSizeBytes), sizeof(dataSizeBytes));
    ofs.seekp(0, std::ios::end);
}

} // namespace

int main(int argc, char** argv) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " input.raw output.wav\n";
        return 1;
    }
    const std::string inPath  = argv[1];
    const std::string outPath = argv[2];

    std::ifstream ifs(inPath, std::ios::binary);
    if (!ifs) {
        std::cerr << "Failed to open input: " << inPath << "\n";
        return 1;
    }

    Header hdr{};
    if (!readExact(ifs, &hdr)) {
        std::cerr << "Failed to read header\n";
        return 1;
    }
    if (std::memcmp(hdr.magic, "OCTOPUS_HEEG", 12) != 0) {
        std::cerr << "Bad magic (expected OCTOPUS_HEEG)\n";
        return 1;
    }
    const uint64_t ampCount = hdr.ampCount;
    const uint64_t chPerAmp = hdr.chPerAmp;
    if (ampCount == 0 || chPerAmp == 0) {
        std::cerr << "Invalid header: ampCount=" << ampCount << " chPerAmp=" << chPerAmp << "\n";
        return 1;
    }
    const uint64_t totalCh = ampCount * chPerAmp;

    std::cerr << "Header OK. ampCount=" << ampCount
              << " chPerAmp=" << chPerAmp
              << " totalCh=" << totalCh << "\n";

    // Compute record size
    const uint64_t recSize =
        sizeof(int64_t) +               // offset
        sizeof(float) * totalCh +       // EEG floats
        sizeof(float) * AUDIO_SAMPLES_PER_REC + // 48 audio floats
        sizeof(int32_t);                // trigger

    if (recSize > (1ull << 32)) {
        std::cerr << "Record size unexpectedly large: " << recSize << " bytes\n";
        return 1;
    }

    // Prepare WAV output with placeholder header.
    std::ofstream ofs(outPath, std::ios::binary);
    if (!ofs) {
        std::cerr << "Failed to open output: " << outPath << "\n";
        return 1;
    }
    writeWavHeader(ofs, WAV_SR, WAV_CH, WAV_BPS, /*dataSizeBytes=*/0);

    // Process records
    std::vector<char> recBuf(static_cast<size_t>(recSize));
    uint64_t recCount = 0;
    uint64_t audioSamplesWritten = 0;

    while (true) {
        if (!readExactN(ifs, recBuf.data(), static_cast<std::streamsize>(recSize))) {
            // EOF or partial tail; stop cleanly
            break;
        }
        const char* p = recBuf.data();

        // Parse offset (unused)
        // int64_t offset;
        // std::memcpy(&offset, p, sizeof(int64_t));
        p += sizeof(int64_t);

        // Skip EEG floats
        p += sizeof(float) * totalCh;

        // Read 48 audio floats
        const float* audioF = reinterpret_cast<const float*>(p);
        p += sizeof(float) * AUDIO_SAMPLES_PER_REC;

        // Trigger (unused here, but parsed to move pointer)
        // int32_t trig;
        // std::memcpy(&trig, p, sizeof(int32_t));

        // Convert and write 48 samples to WAV
        for (size_t i = 0; i < AUDIO_SAMPLES_PER_REC; ++i) {
            int16_t s = floatToInt16(audioF[i]);
            ofs.write(reinterpret_cast<const char*>(&s), sizeof(s));
        }
        audioSamplesWritten += AUDIO_SAMPLES_PER_REC;
        ++recCount;
    }

    // Fix WAV sizes
    const uint32_t dataBytes = static_cast<uint32_t>(audioSamplesWritten * (WAV_BPS/8) * WAV_CH);
    fixWavSizes(ofs, dataBytes);
    ofs.flush();

    std::cerr << "Done. Records read: " << recCount
              << " | Audio samples: " << audioSamplesWritten
              << " (" << (audioSamplesWritten / static_cast<double>(WAV_SR)) << " s)"
              << " -> " << outPath << "\n";
    return 0;
}
