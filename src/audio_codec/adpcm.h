#pragma once

#include "audio/audioformat.h"
#include "statistics/statistics.h"

#include <cstdint>
#include <vector>

namespace Audio
{
    class Adpcm
    {
    public:
        Adpcm(Audio::ChannelFormat channelFormat, uint32_t sampleRateHz);

        ~Adpcm();

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
