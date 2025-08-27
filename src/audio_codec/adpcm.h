#pragma once

#include "audio/audioformat.h"
#include "statistics/statistics.h"

#include <cstdint>
#include <vector>

namespace Audio
{
    class ADPCM
    {
    public:
        static constexpr int BITS_PER_SAMPLE = 4; // ADPCM bits per sample
        static constexpr int LOOKAHEAD = 3;       // Lookahead value for ADPCM encoder. The more the better quality, but slower encoding

        struct FrameHeader
        {
            uint16_t flags : 5 = 0;                            // Flags (currently unused)
            uint16_t nrOfChannels : 2 = 0;                     // PCM output channels [1,2]
            uint16_t pcmBitsPerSample : 6 = 16;                // PCM output sample bit depth in [1,32]
            uint16_t adpcmBitsPerSample : 3 = BITS_PER_SAMPLE; // ADPCM sample bit depth in [3,5]
            uint16_t uncompressedSize = 0;                     // Uncompressed size of PCM output data

            std::vector<uint8_t> toVector() const;
            static auto fromVector(const std::vector<uint8_t> &data) -> FrameHeader;
        } __attribute__((aligned(4), packed));

        ADPCM(Audio::ChannelFormat channelFormat, uint32_t sampleRateHz);

        ~ADPCM();

        /// @brief Compress to ADPCM format
        auto encode(const Audio::SampleData &samples, Statistics::Frame::SPtr statistics = nullptr) -> std::vector<uint8_t>;

        /// @brief Decompress from ADPCM format
        static auto decode(const std::vector<uint8_t> &data) -> Audio::SampleData;

    private:
        struct State;
        std::shared_ptr<State> m_state;
        uint32_t m_nrOfChannels = 0;
        uint32_t m_sampleRateHz = 0;
    };
}
