#pragma once

#include "audio/audiostructs.h"
#include "exception.h"

#include <cstdint>
#include <fstream>

namespace IO
{

    class WavWriter
    {
    public:
        /// @brief Constructor
        WavWriter() = default;

        /// @brief Destruktor. Calls close()
        virtual ~WavWriter();

        /// @brief Open RIFF / WAVE file for writing
        /// @param filePathOptional Path to output file
        /// @throws std::runtime_error when failing to open the file for writing
        /// @note Will overwrite the file
        auto open(const std::string &filePath) -> void;

        /// @brief Write audio data to a WAV file
        /// @param frame Audio frame data. First frame will determine the format. All other frames should match.
        /// @throws std::runtime_error when frame info does not match first frame
        auto writeFrame(const Audio::Frame &frame) -> void;

        /// @brief Close writer opened with open()
        auto close() -> void;

    private:
        struct WavHeader
        {
            // RIFF chunk descriptor
            const uint8_t RIFFChunkId[4] = {'R', 'I', 'F', 'F'};  // RIFF file header magic
            uint32_t riffChunkSize;                               // RIFF chunk size (File size - 8)
            const uint8_t FileFormatId[4] = {'W', 'A', 'V', 'E'}; // Format identifier: WAVE file
            // Format sub-chunk
            const uint8_t FMTChunkId[4] = {'f', 'm', 't', ' '}; // FMT chunk header
            const uint32_t FMTChunkSize = 16;                   // FMT chunk size - 8 -> 16
            uint16_t dataFormat = 0;                            // Audio data format:
                                                                // 1 = PCM
                                                                // 2 = Microsoft ADPCM
                                                                // 3 = IEEE 754 float
                                                                // 6 = 8-bit ITU-T G.711 A-law
                                                                // 7 = 8-bit ITU-T G.711 µ-law
                                                                // 17 = DVI/IMA ADPCM
            uint16_t nrOfChannels = 0;                          // Number of channels: 1 = Mono, 2 = Stereo
            uint32_t sampleRateHz = 0;                          // Sample rate in Hz
            uint32_t bytesPerSec = 0;                           // Bytes per second (blockAlignment * sampleRateHz)
            uint16_t blockAlign = 0;                            // Aligment of samples ((bitsPerSample + 7 ) / 8 * nrOfChannels), e.g. 2 = 16-bit mono, 4 = 16-bit stereo
            uint16_t bitsPerSample = 0;                         // Number of bits per sample
            // Data sub-chunk
            const uint8_t DATAChunkId[4] = {'d', 'a', 't', 'a'}; // DATA chunk header
            uint32_t dataSize;                                   // Sample data length in bytes (File size - sizeof(WavHeader))
            // Now follows interleaved (L0 R0 L1 R1 ...) sample data
        };

        Audio::FrameInfo m_info;
        Audio::SampleFormatInfo m_sampleInfo;
        Audio::ChannelFormatInfo m_channelInfo;
        WavHeader m_fileHeader;
        std::ofstream m_os;
    };

}
