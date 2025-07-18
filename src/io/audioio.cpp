#include "audioio.h"

#include "audio/audiohelpers.h"
#include "exception.h"

#include <filesystem>
#include <fstream>

namespace IO
{

    struct WavHeader
    {
        // RIFF chunk descriptor
        const uint8_t RIFFChunkId[4] = {'R', 'I', 'F', 'F'};  // RIFF file header magic
        uint32_t riffChunkSize;                               // RIFF chunk size (File size - 8)
        const uint8_t FileFormatId[4] = {'W', 'A', 'V', 'E'}; // Format identifier: WAVE file
        // Format sub-chunk
        const uint8_t FMTChunkId[4] = {'f', 'm', 't', 0}; // FMT chunk header
        const uint32_t FMTChunkSize = 16;                 // FMT chunk size - 8 -> 16
        uint16_t dataFormat = 0;                          // Audio data format:
                                                          // 1 = PCM
                                                          // 2 = Microsoft ADPCM
                                                          // 3 = IEEE 754 float
                                                          // 6 = 8-bit ITU-T G.711 A-law
                                                          // 7 = 8-bit ITU-T G.711 µ-law
                                                          // 17 = DVI/IMA ADPCM
        uint16_t nrOfChannels = 0;                        // Number of channels: 1 = Mono, 2 = Stereo
        uint32_t sampleRateHz = 0;                        // Sample rate in Hz
        uint32_t bytesPerSec = 0;                         // Bytes per second (blockAlignment * sampleRateHz)
        uint16_t blockAlign = 0;                          // Aligment of samples ((bitsPerSample + 7 ) / 8 * nrOfChannels), e.g. 2 = 16-bit mono, 4 = 16-bit stereo
        uint16_t bitsPerSample = 0;                       // Number of bits per sample
        // Data sub-chunk
        const uint8_t DATAChunkId[4] = {'d', 'a', 't', 'a'}; // DATA chunk header
        uint32_t dataSize;                                   // Sample data length in bytes (File size - sizeof(WavHeader))
        // Now follows interleaved (L0 R0 L1 R1 ...) sample data
    };

    auto writeAudio(const Audio::FrameInfo &info, const Audio::SampleData &samples, const std::string &folder, const std::string &fileName) -> void
    {
        static_assert(sizeof(WavHeader) == 44);
        REQUIRE(fileName.empty(), std::runtime_error, "Either fileName must contain a file name");
        REQUIRE(info.channelFormat != Audio::ChannelFormat::Unknown, std::runtime_error, "Bad audio channel format");
        REQUIRE(info.sampleRateHz > 0 && info.sampleRateHz <= 48000, std::runtime_error, "Bad audio sample rate " << info.sampleRateHz << " Hz");
        REQUIRE(info.sampleFormat != Audio::SampleFormat::Unknown, std::runtime_error, "Bad audio sample format");
        // get format information
        const auto &sampleFormatInfo = Audio::formatInfo(info.sampleFormat);
        const auto &channelFormatInfo = Audio::formatInfo(info.channelFormat);
        // get raw sample data
        std::vector<uint8_t> rawSampleData;
        switch (info.sampleFormat)
        {
        case Audio::SampleFormat::Signed8P:
            REQUIRE(std::holds_alternative<std::vector<int8_t>>(samples), std::runtime_error, "Sample data type does not match sample format " << sampleFormatInfo.id);
            rawSampleData = AudioHelpers::toRawInterleavedData(samples, channelFormatInfo.nrOfChannels);
            break;
        case Audio::SampleFormat::Unsigned8P:
            REQUIRE(std::holds_alternative<std::vector<uint8_t>>(samples), std::runtime_error, "Sample data type does not match sample format " << sampleFormatInfo.id);
            rawSampleData = AudioHelpers::toRawInterleavedData(samples, channelFormatInfo.nrOfChannels);
            break;
        case Audio::SampleFormat::Signed16P:
            REQUIRE(std::holds_alternative<std::vector<int16_t>>(samples), std::runtime_error, "Sample data type does not match sample format " << sampleFormatInfo.id);
            rawSampleData = AudioHelpers::toRawInterleavedData(samples, channelFormatInfo.nrOfChannels);
            break;
        case Audio::SampleFormat::Unsigned16P:
            REQUIRE(std::holds_alternative<std::vector<uint16_t>>(samples), std::runtime_error, "Sample data type does not match sample format " << sampleFormatInfo.id);
            rawSampleData = AudioHelpers::toRawInterleavedData(samples, channelFormatInfo.nrOfChannels);
            break;
        case Audio::SampleFormat::Float32P:
            REQUIRE(std::holds_alternative<std::vector<float>>(samples), std::runtime_error, "Sample data type does not match sample format " << sampleFormatInfo.id);
            rawSampleData = AudioHelpers::toRawInterleavedData(samples, channelFormatInfo.nrOfChannels);
            break;
        default:
            THROW(std::runtime_error, "Bad sample format");
        }
        // build RIFF / WAVE file header
        WavHeader wavHeader;
        wavHeader.riffChunkSize = static_cast<uint32_t>(rawSampleData.size() + sizeof(WavHeader) - 8);
        wavHeader.dataFormat = 1; // PCM data
        wavHeader.nrOfChannels = channelFormatInfo.nrOfChannels;
        wavHeader.sampleRateHz = info.sampleRateHz;
        wavHeader.bytesPerSec = ((sampleFormatInfo.bitsPerSample + 7) / 8) * channelFormatInfo.nrOfChannels * info.sampleRateHz;
        wavHeader.blockAlign = ((sampleFormatInfo.bitsPerSample + 7) / 8) * channelFormatInfo.nrOfChannels;
        wavHeader.bitsPerSample = sampleFormatInfo.bitsPerSample;
        wavHeader.dataSize = static_cast<uint32_t>(rawSampleData.size());
        // create paths if neccessary
        auto outName = fileName;
        auto outPath = std::filesystem::path(folder) / std::filesystem::path(outName).filename();
        if (!std::filesystem::exists(std::filesystem::path(folder)))
        {
            std::filesystem::create_directory(std::filesystem::path(folder));
        }
        // write to disk
        auto ofs = std::ofstream(outPath, std::ios::binary);
        ofs.write(reinterpret_cast<const char *>(&wavHeader), sizeof(wavHeader));
        ofs.write(reinterpret_cast<const char *>(rawSampleData.data()), rawSampleData.size());
    }

}
