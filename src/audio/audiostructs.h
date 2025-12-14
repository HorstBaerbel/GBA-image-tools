#pragma once

#include "audioformat.h"

#include <cstdint>
#include <string>

namespace Audio
{

    /// @brief Audio data returned from media reader
    using RawData = std::vector<int16_t>;

    /// @brief Information about current / final audio data before compression / conversion to raw
    struct FrameInfo
    {
        uint32_t sampleRateHz = 0;
        ChannelFormat channelFormat = ChannelFormat::Unknown;
        SampleFormat sampleFormat = SampleFormat::Unknown;
        bool compressed = false;      // Flag if raw or compressed data is stored
        uint32_t maxMemoryNeeded = 0; // Max. intermediate memory needed to process the audio. 0 if it can be directly written to destination (single processing stage)
    };

    /// @brief Stores data for an audio frame
    struct Frame
    {
        uint32_t index = 0;       // Input frame index counter
        std::string fileName;     // Input file name
        FrameInfo info;           // Frame information
        SampleData data;          // Raw / compressed data of mono or (planar) stereo channels
        uint32_t nrOfSamples = 0; // Number of samples per channel in data
    };

}
