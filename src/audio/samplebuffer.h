#pragma once

#include "audioformat.h"
#include "audiostructs.h"

#include <cstdint>
#include <memory>

namespace Audio
{

    /// @brief Mono or planar stereo sample buffer with specific format
    class SampleBuffer
    {
    public:
        /// @brief Create sample buffer with specific format
        /// @param channelFormat Input channels. Only mono or stereo accepted
        /// @param sampleRateHz Input sample rate in Hz
        /// @param sampleFormat Input sample format
        SampleBuffer(ChannelFormat channelFormat, uint32_t sampleRateHz, SampleFormat sampleFormat);

        ~SampleBuffer() = default;

        /// @brief Get the number of samples per channel currently in buffer
        /// @return Number of samples per channel currently in buffer
        auto nrOfSamplesPerChannel() const -> std::size_t;

        /// @brief Push samples to the end of the buffer(s)
        /// @param frame Audio samples
        /// @throws std::runtime_exception if frame does not have the same format as in the SampleBuffer constructor
        auto push_back(const Frame &frame) -> void;

        /// @brief Pop samples from the beginning of the buffer(s)
        /// @param nrOfSamplesPerChannel Number of samples per channel to return
        /// @return Audio samples
        auto pop_front(std::size_t nrOfSamplesPerChannel) -> Frame;

    private:
        ChannelFormat m_channelFormat = ChannelFormat::Unknown;
        uint32_t m_sampleRateHz = 0;
        SampleFormat m_sampleFormat = SampleFormat::Unknown;
        SampleData m_samples;
    };
}
