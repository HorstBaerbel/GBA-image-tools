#pragma once

#include "audiostructs.h"
#include "audioformat.h"

#include <cstdint>
#include <memory>

namespace Audio
{

    class Resampler
    {
    public:
        /// @brief Audio resampler. Using FFmpeg. Accepts int16_t input samples only
        /// @param inChannelFormat Input channels. Only mono or stereo accepted
        /// @param inSampleRateHz Input sample rate
        /// @param outChannelFormat Output channels. Only mono or stereo accepted
        /// @param outSampleRateHz Output sample rate
        /// @param outSampleFormat Output sample format
        Resampler(ChannelFormat inChannelFormat, uint32_t inSampleRateHz,
                  ChannelFormat outChannelFormat, uint32_t outSampleRateHz, SampleFormat outSampleFormat);

        ~Resampler();

        /// @brief Get auto format information
        /// @return Resampler outpput audio format
        auto getOutputFormat() const -> FrameInfo;

        /// @brief Resample one frame of audio
        /// @param inFrame Input audio frame. Must be int16_t data
        /// @return Resampled audio frame
        auto resample(const Frame &inFrame) -> Frame;

    private:
        struct State;
        std::shared_ptr<State> m_state;
        ChannelFormat m_inChannelFormat = ChannelFormat::Unknown;
        uint32_t m_inSampleRateHz = 0;
        ChannelFormat m_outChannelFormat = ChannelFormat::Unknown;
        uint32_t m_outSampleRateHz = 0;
        SampleFormat m_outSampleFormat = SampleFormat::Unknown;
        uint8_t *m_outData[2] = {nullptr, nullptr}; // Stereo audio conversion output sample buffer
        uint32_t m_outDataNrOfSamples = 0;          // Audio conversion output sample buffer size
    };

}
